#include "NetScene.h"
#include "ZoneSystem.h"
#include "NetSyncManager.h"

namespace NetScene {
    static constexpr int m_maxCreatedPerFrame = 10;
    static constexpr int m_maxDestroyedPerFrame = 20;
    static constexpr float m_createDestroyFps = 30;

    robin_hood::unordered_map<HASH_t, GameObject> m_namedPrefabs;

    robin_hood::unordered_map<ZDO*, ZNetView*> m_instances; // comparer: new ZDOComparer();

    std::vector<ZDO*> m_tempCurrentObjects;
    std::vector<ZDO*> m_tempCurrentObjects2; // redundant?
    std::vector<ZDO*> m_tempCurrentDistantObjects;
    std::vector<ZNetView*> m_tempRemoved;

    robin_hood::unordered_set<ZDO*> m_tempActiveZDOs; //new ZDOComparer();

    float m_createDestroyTimer;

	// forward declared privates

	bool IsPrefabZDOValid(ZDO *zdo);
	GameObject CreateObject(ZDO *zdo);
	bool InLoadingScreen();
	void CreateObjects(std::vector<ZDO*> currentNearObjects, std::vector<ZDO*> &currentDistantObjects);
	void CreateObjectsSorted(std::vector<ZDO*> currentNearObjects, int maxCreatedPerFrame, int &created);
	static int ZDOCompare(ZDO *x, ZDO *y);
	void CreateDistantObjects(std::vector<ZDO*> objects, int maxCreatedPerFrame, int &created);
	void OnZDODestroyed(ZDO *zdo);
	void RemoveObjects(std::vector<ZDO*> &currentNearObjects, std::vector<ZDO*> &currentDistantObjects);
	void CreateDestroyObjects();
	void RPC_SpawnObject(OWNER_t spawner, Vector3 pos, Quaternion rot, HASH_t prefabHash);



	// defined publics
	void Init() {
		LOG(INFO) << "m_prefabs:";
		for (auto &&gameObject : m_prefabs) {
			LOG(INFO) << gameObject.name + ", " + gameObject.name.GetStableHashCode();
			m_namedPrefabs.Add(gameObject.name.GetStableHashCode(), gameObject);
		}
		ZLog.Log("m_nonNetViewPrefabs:");
		foreach(GameObject gameObject2 in this.m_nonNetViewPrefabs)
		{
			ZLog.Log(gameObject2.name + ", " + gameObject2.name.GetStableHashCode());
			this.m_namedPrefabs.Add(gameObject2.name.GetStableHashCode(), gameObject2);
		}
		ZDOMan instance = ZDOMan.instance;
		m_onZDODestroyed = (Action<ZDO>)Delegate.Combine(instance.m_onZDODestroyed, new Action<ZDO>(this.OnZDODestroyed));
		NetRouteManager::Register("SpawnObject", RPC_SpawnObject);
	}

	void Shutdown() {
		for (auto&& keyValuePair : m_instances) {
			if (keyValuePair.first) {
				keyValuePair.first->ResetZDO();
				UnityEngine.Object.Destroy(keyValuePair.Value.gameObject);
			}
		}
		m_instances.Clear();
		base.enabled = false;
	}

	void AddInstance(ZDO *zdo, ZNetView nview) {
		m_instances[zdo] = nview;
	}

	void Destroy(GameObject go) {
		ZNetView *component = go.GetComponent<ZNetView>();
		if (component && component.GetZDO()) {
			auto zdo = component.GetZDO();
			component->ResetZDO();
			m_instances.Remove(zdo);
			if (zdo->Local()) {
				ZDOMan.instance.DestroyZDO(zdo);
			}
		}
		UnityEngine.Object.Destroy(go);
	}

	GameObject GetPrefab(int hash) {
		GameObject result;
		if (m_namedPrefabs.TryGetValue(hash, out result))
			return result;
		return nullptr;
	}

	GameObject GetPrefab(string name) {
		int stableHashCode = name.GetStableHashCode();
		return this.GetPrefab(stableHashCode);
	}

	HASH_t GetPrefabHash(GameObject go) {
		return go.name.GetStableHashCode();
	}

	bool IsAreaReady(Vector3 point) {
		auto zone = ZoneSystem::GetZoneCoords(point);
		if (!ZoneSystem::IsZoneLoaded(zone))
            return false;
		m_tempCurrentObjects.clear();
        NetSyncManager::FindSectorObjects(zone, 1, 0, m_tempCurrentObjects);
		for (auto zdo : m_tempCurrentObjects) {
			if (IsPrefabZDOValid(zdo) && !FindInstance(zdo))
				return false;
		}
		return true;
	}

	ZNetView *FindInstance(ZDO *zdo) {
        auto&& find = m_instances.find(zdo);
        if (find != m_instances.end())
            return find.second;
		return nullptr;
	}

	bool HaveInstance(ZDO *zdo) {
		return m_instances.contains(zdo);
	}

	GameObject FindInstance(const ZDOID &id) {
		auto zdo = NetSyncManager::GetZDO(id);
		if (zdo) {
			auto znetView = FindInstance(zdo);
			if (znetView)
				return znetView.gameObject;
		}
		return nullptr;
	}

	void Update() {
		float deltaTime = Valhalla()->Delta();
		m_createDestroyTimer += deltaTime;
		if (m_createDestroyTimer >= 0.033333335f) {
			m_createDestroyTimer = 0;
			CreateDestroyObjects();
		}
	}

	bool InActiveArea(Vector2i zone, Vector3 refPoint) {
		Vector2i zone2 = ZoneSystem::GetZoneCoords(refPoint);
		return InActiveArea(zone, zone2);
	}

	bool InActiveArea(Vector2i zone, Vector2i refCenterZone) {
		int num = ZoneSystem::ACTIVE_AREA - 1;
		return zone.x >= refCenterZone.x - num
            && zone.x <= refCenterZone.x + num
            && zone.y <= refCenterZone.y + num
            && zone.y >= refCenterZone.y - num;
	}

	bool OutsideActiveArea(Vector3 point) {
		return OutsideActiveArea(point, NetManager::GetReferencePosition());
	}

	bool OutsideActiveArea(Vector3 point, Vector3 refPoint) {
		auto zone = ZoneSystem::GetZoneCoords(refPoint);
		auto zone2 = ZoneSystem::GetZoneCoords(point);
		return zone2.x <= zone.x - ZoneSystem::ACTIVE_AREA
            || zone2.x >= zone.x + ZoneSystem::ACTIVE_AREA
            || zone2.y >= zone.y + ZoneSystem::ACTIVE_AREA
            || zone2.y <= zone.y - ZoneSystem::ACTIVE_AREA;
	}

	bool HaveInstanceInSector(Vector2i sector) {
		for (auto&& keyValuePair : m_instances) {
			if (keyValuePair.first
                && !keyValuePair.first->Distant()
                && ZoneSystem::GetZoneCoords(keyValuePair.first.transform.position) == sector) {
				return true;
			}
		}
		return false;
	}

	int NrOfInstances() {
		return m_instances.size();
	}

	void SpawnObject(const Vector3 &pos, const Quaternion &rot, GameObject prefab) {
		HASH_t prefabHash = GetPrefabHash(prefab);
		NetRouteManager::Invoke(NetRouteManager::EVERYBODY, "SpawnObject", pos, rot, prefabHash);
	}

	std::vector<std::string> GetPrefabNames() {
		std::vector<std::string> list;
		for (auto&& keyValuePair : m_namedPrefabs) {
			list.push_back(keyValuePair.Value.name);
		}
		return list;
	}



	// defined privates

	bool IsPrefabZDOValid(ZDO *zdo) {
		HASH_t prefab = zdo->PrefabHash();
		return prefab && GetPrefab(prefab);
	}

	GameObject CreateObject(ZDO* zdo) {
		HASH_t prefab = zdo->PrefabHash();
		if (prefab == 0)
			return nullptr;

		GameObject prefab2 = GetPrefab(prefab);
		if (!prefab2)
			return nullptr;

		auto position = zdo->Position();
		auto rotation = zdo->Rotation();

        // The below portion is a hackish way to construct the object with args
		//ZNetView.m_useInitZDO = true;
		//ZNetView.m_initZDO = zdo;
		GameObject result = UnityEngine.Object.Instantiate<GameObject>(prefab2, position, rotation);
        // TODO fix the above ^^^
        // a direct ZNetView constructor can be utilized instead of the above
		//if (ZNetView.m_initZDO != null) {
		//	string str = "ZDO ";
		//	ZDOID uid = zdo.m_uid;
		//	ZLog.LogWarning(str + uid.ToString() + " not used when creating object " + prefab2.name);
		//	ZNetView.m_initZDO = null;
		//}
		//ZNetView.m_useInitZDO = false;
		return result;
	}

	//bool InLoadingScreen() {
	//	return Player.m_localPlayer == null || Player.m_localPlayer.IsTeleporting();
	//}

	void CreateObjects(std::vector<ZDO*> &currentNearObjects, std::vector<ZDO*> &currentDistantObjects) {
		int maxCreatedPerFrame = 10;
        // TODO client only
		if (InLoadingScreen()) {
			maxCreatedPerFrame = 100;
		}
		int frameCount = Time.frameCount;
		for (auto&& pair : m_instances) {
            auto zdo = pair.first;
			zdo.m_tempCreateEarmark = frameCount;
		}
		int num = 0;
		CreateObjectsSorted(currentNearObjects, maxCreatedPerFrame, ref num);
		CreateDistantObjects(currentDistantObjects, maxCreatedPerFrame, ref num);
	}

	void CreateObjectsSorted(std::vector<ZDO> &currentNearObjects, int maxCreatedPerFrame, ref int created) {
		if (!ZoneSystem::IsActiveAreaLoaded())
			return;

		m_tempCurrentObjects2.clear();
		int frameCount = Time.frameCount;
        // TODO client only?
		Vector3 referencePosition = NetManager::GetReferencePosition();
		for (auto zdo : currentNearObjects)
		{
			if (zdo->m_tempCreateEarmark != frameCount) {
				zdo.m_tempSortValue = Utils.DistanceSqr(referencePosition, zdo.GetPosition());
				m_tempCurrentObjects2.Add(zdo);
			}
		}
		int num = Mathf.Max(m_tempCurrentObjects2.Count / 100, maxCreatedPerFrame);
		m_tempCurrentObjects2.Sort(new Comparison<ZDO>(ZNetScene.ZDOCompare));
		for (auto zdo2 : m_tempCurrentObjects2) {
			if (CreateObject(zdo2)) {
				created++;
				if (created > num) {
					break;
				}
			}
			else {
                //LOG(INFO) << "Destroyed invalid prefab ZDO " << zdo2->ID();
                zdo2->SetLocal();
				NetSyncManager::DestroyZDO(zdo2);
			}
		}
	}

    // TODO make sort lambda runnable
	static int ZDOCompare(ZDO *x, ZDO *y) {
		if (x->Type() == y->Type()) {
			return Utils.CompareFloats(x.m_tempSortValue, y.m_tempSortValue);
		}
		int type = (int)y->Type();
		return type.CompareTo((int)x->Type());
	}

	void CreateDistantObjects(std::vector<ZDO*> objects, int maxCreatedPerFrame, ref int created) {
		if (created > maxCreatedPerFrame)
			return;

		int frameCount = Time.frameCount;
		for (auto zdo : objects) {
			if (zdo.m_tempCreateEarmark != frameCount) {
				if (CreateObject(zdo)) {
					created++;
					if (created > maxCreatedPerFrame)
						break;
				}
				else {
                    //LOG(INFO) << "Destroyed invalid prefab ZDO " << zdo->ID();
					zdo->SetLocal();
					NetSyncManager::DestroyZDO(zdo);
				}
			}
		}
	}

	void OnZDODestroyed(ZDO *zdo) {
        auto&& find = m_instances.find(zdo);
        if (find != m_instances.end()) {
            auto&& znetView = find->second;
			znetView->ResetNetSync();
			UnityEngine.Object.Destroy(znetView.gameObject);
			m_instances.erase(find);
		}
	}

	void RemoveObjects(std::vector<ZDO*> &currentNearObjects, std::vector<ZDO*> &currentDistantObjects) {
		int frameCount = Time.frameCount;

		for (auto zdo : currentNearObjects)
			zdo.m_tempRemoveEarmark = frameCount;

		for (ZDO zdo2 in currentDistantObjects)
			zdo2.m_tempRemoveEarmark = frameCount;

		m_tempRemoved.clear();
        for (auto&& pair : m_instances) {
            auto&& znetView = pair.second;
			if (znetView->GetNetSync().m_tempRemoveEarmark != frameCount)
				m_tempRemoved.push_back(znetView);
		}

		for (int i = 0; i < m_tempRemoved.size(); i++) {
			auto znetView2 = m_tempRemoved[i];
			auto zdo3 = znetView2->GetNetSync();
			znetView2->ResetNetSync();
			UnityEngine.Object.Destroy(znetView2.gameObject);
			if (!zdo3->Persists() && zdo3->Local()) {
				NetSyncManager::DestroyZDO(zdo3);
			}
			m_instances.erase(zdo3);
		}
	}

	void CreateDestroyObjects() {
        // TODO client
		auto zone = ZoneSystem::GetZoneCoords(NetManager::GetReferencePosition());
		m_tempCurrentObjects.clear();
		m_tempCurrentDistantObjects.clear();
		NetSyncManager::FindSectorObjects(zone, ZoneSystem::ACTIVE_AREA, ZoneSystem::ACTIVE_DISTANT_AREA, m_tempCurrentObjects, m_tempCurrentDistantObjects);
		CreateObjects(m_tempCurrentObjects, m_tempCurrentDistantObjects);
		RemoveObjects(m_tempCurrentObjects, m_tempCurrentDistantObjects);
	}

	void RPC_SpawnObject(long spawner, Vector3 pos, Quaternion rot, int prefabHash) {
		GameObject prefab = GetPrefab(prefabHash);
		if (!prefab) {
			LOG(INFO) << "Missing prefab " << prefabHash;
			return;
		}
		UnityEngine.Object.Instantiate<GameObject>(prefab, pos, rot);
	}
}