#include "NetScene.h"
#include "ZoneSystem.h"
#include "NetSyncManager.h"
#include "GameObject.h"



// defined publics
void NetScene::Init() {
	LOG(INFO) << "m_prefabs:";
	for (auto &&gameObject : m_prefabs) {
		LOG(INFO) << gameObject->name << ", " << VUtils::String::GetStableHashCode(gameObject->name);
		m_namedPrefabs[VUtils::String::GetStableHashCode(gameObject->name)] = gameObject;
	}
	//LOG(INFO) << "m_nonNetViewPrefabs:";
	//for (auto&& gameObject2 : m_nonNetViewPrefabs)
	//{
	//	LOG(INFO) << gameObject2.name + ", " + gameObject2.name.GetStableHashCode();
	//	m_namedPrefabs[VUtils::String::GetStableHashCode(gameObject2.name)] = gameObject2;
	//}
	//ZDOMan instance = ZDOMan.instance;
	//m_onZDODestroyed = (Action<ZDO>)Delegate.Combine(instance.m_onZDODestroyed, new Action<ZDO>(this.OnZDODestroyed));
	//NetRouteManager::Register("SpawnObject", RPC_SpawnObject);
}

void NetScene::Shutdown() {
	assert(false);
	for (auto&& keyValuePair : m_instances) {
		if (keyValuePair.first) {
			keyValuePair.first->Invalidate();
			//keyValuePair.first->ResetZDO();
			//UnityEngine.Object.Destroy(keyValuePair.Value.gameObject);
		}
	}
	m_instances.clear();
}

void NetScene::AddInstance(ZDO *zdo, ZNetView *nview) {
	m_instances[zdo] = nview;
}

//
//void NetScene::Destroy(GameObject go) {
//	assert(false);
//	//ZNetView *component = go.GetComponent<ZNetView>();
//	//if (component && component.GetZDO()) {
//	//	auto zdo = component.GetZDO();
//	//	component->ResetZDO();
//	//	m_instances.Remove(zdo);
//	//	if (zdo->Local()) {
//	//		ZDOMan.instance.DestroyZDO(zdo);
//	//	}
//	//}
//	//UnityEngine.Object.Destroy(go);
//}

//GameObject GetPrefab(int hash) {
//	GameObject result;
//	if (m_namedPrefabs.TryGetValue(hash, out result))
//		return result;
//	return nullptr;
//}
//
//GameObject GetPrefab(string name) {
//	int stableHashCode = name.GetStableHashCode();
//	return this.GetPrefab(stableHashCode);
//}

//HASH_t GetPrefabHash(GameObject go) {
//	return go.name.GetStableHashCode();
//}

// client only
//bool IsAreaReady(Vector3 point) {
//	auto zone = ZoneSystem::WorldToZonePos(point);
//	if (!ZoneSystem::IsZoneLoaded(zone))
//        return false;
//	m_tempCurrentObjects.clear();
//    NetSyncManager::FindSectorObjects(zone, 1, 0, m_tempCurrentObjects);
//	for (auto zdo : m_tempCurrentObjects) {
//		if (IsPrefabZDOValid(zdo) && !FindInstance(zdo))
//			return false;
//	}
//	return true;
//}

ZNetView* NetScene::FindInstance(ZDO *zdo) {
    auto&& find = m_instances.find(zdo);
    if (find != m_instances.end())
        return find->second;
	return nullptr;
}

bool NetScene::HaveInstance(ZDO *zdo) {
	return m_instances.contains(zdo);
}

//GameObject FindInstance(const ZDOID &id) {
//	auto zdo = NetSyncManager::GetZDO(id);
//	if (zdo) {
//		auto znetView = FindInstance(zdo);
//		if (znetView)
//			return znetView.gameObject;
//	}
//	return nullptr;
//}

//void NetScene::Update() {
//	float deltaTime = Valhalla()->Delta();
//	//m_createDestroyTimer += deltaTime;
//	//if (m_createDestroyTimer >= 0.033333335f) {
//	//	m_createDestroyTimer = 0;
//	//	//CreateDestroyObjects();
//	//}
//}

// relocated to ZoneSystem
//bool NetScene::InActiveArea(const Vector2i &zone, const Vector3 &refPoint) {
//	return InActiveArea(zone, 
//		ZoneSystem::WorldToZonePos(refPoint));
//}
//
//bool NetScene::InActiveArea(const Vector2i& zone, const Vector2i& refCenterZone) {
//	int num = ZoneSystem::ACTIVE_AREA - 1;
//	return zone.x >= refCenterZone.x - num
//        && zone.x <= refCenterZone.x + num
//        && zone.y <= refCenterZone.y + num
//        && zone.y >= refCenterZone.y - num;
//}

//bool OutsideActiveArea(Vector3 point) {
//	return OutsideActiveArea(point, NetManager::GetReferencePosition());
//}
//
//bool OutsideActiveArea(Vector3 point, Vector3 refPoint) {
//	auto zone = ZoneSystem::GetZoneCoords(refPoint);
//	auto zone2 = ZoneSystem::GetZoneCoords(point);
//	return zone2.x <= zone.x - ZoneSystem::ACTIVE_AREA
//        || zone2.x >= zone.x + ZoneSystem::ACTIVE_AREA
//        || zone2.y >= zone.y + ZoneSystem::ACTIVE_AREA
//        || zone2.y <= zone.y - ZoneSystem::ACTIVE_AREA;
//}

//bool HaveInstanceInSector(const Vector2i& sector) {
//	for (auto&& keyValuePair : m_instances) {
//		if (keyValuePair.first
//            && !keyValuePair.first->Distant()
//            && ZoneSystem::WorldToZonePos(keyValuePair.->first.transform.position) == sector) {
//			return true;
//		}
//	}
//	return false;
//}

//int NrOfInstances() {
//	return m_instances.size();
//}

//void SpawnObject(const Vector3 &pos, const Quaternion &rot, GameObject prefab) {
//	HASH_t prefabHash = GetPrefabHash(prefab);
//	NetRouteManager::Invoke(NetRouteManager::EVERYBODY, "SpawnObject", pos, rot, prefabHash);
//}

// client terminal command only
//std::vector<std::string> GetPrefabNames() {
//	std::vector<std::string> list;
//	for (auto&& keyValuePair : m_namedPrefabs) {
//		list.push_back(keyValuePair.Value.name);
//	}
//	return list;
//}



// defined privates

// client only
//bool IsPrefabZDOValid(ZDO *zdo) {
//	HASH_t prefab = zdo->PrefabHash();
//	return prefab && GetPrefab(prefab);
//}

//GameObject NetScene::CreateObject(ZDO* zdo) {
//	HASH_t prefab = zdo->PrefabHash();
//	if (prefab == 0)
//		return nullptr;
//
//	GameObject prefab2 = GetPrefab(prefab);
//	if (!prefab2)
//		return nullptr;
//
//	auto position = zdo->Position();
//	auto rotation = zdo->Rotation();
//
//    // The below portion is a hackish way to construct the object with args
//	//ZNetView.m_useInitZDO = true;
//	//ZNetView.m_initZDO = zdo;
//	GameObject result = UnityEngine.Object.Instantiate<GameObject>(prefab2, position, rotation);
//    // TODO fix the above ^^^
//    // a direct ZNetView constructor can be utilized instead of the above
//	//if (ZNetView.m_initZDO != null) {
//	//	string str = "ZDO ";
//	//	ZDOID uid = zdo.m_uid;
//	//	ZLog.LogWarning(str + uid.ToString() + " not used when creating object " + prefab2.name);
//	//	ZNetView.m_initZDO = null;
//	//}
//	//ZNetView.m_useInitZDO = false;
//	return result;
//}

//void NetScene::CreateObjects(std::vector<ZDO*> &currentNearObjects, std::vector<ZDO*> &currentDistantObjects) {
//	int maxCreatedPerFrame = 100;
//
//	int frameCount = Time.frameCount;
//	for (auto&& pair : m_instances) {
//        auto zdo = pair.first;
//		zdo.m_tempCreateEarmark = frameCount;
//	}
//	CreateObjectsSorted(currentNearObjects, maxCreatedPerFrame);
//	CreateDistantObjects(currentDistantObjects, maxCreatedPerFrame);
//}

// essentially client only
// !ZoneSystem.instance.IsActiveAreaLoaded()) should always return false on server
//void CreateObjectsSorted(std::vector<ZDO*> &currentNearObjects, int maxCreatedPerFrame) {
//	m_tempCurrentObjects2.clear();
//	int frameCount = Time.frameCount;
//
//	Vector3 referencePosition = NetManager::GetReferencePosition();
//	for (auto zdo : currentNearObjects)
//	{
//		if (zdo->m_tempCreateEarmark != frameCount) {
//			zdo.m_tempSortValue = VUtils.DistanceSqr(referencePosition, zdo.GetPosition());
//			m_tempCurrentObjects2.Add(zdo);
//		}
//	}
//	int num = std::max(m_tempCurrentObjects2.size() / 100, maxCreatedPerFrame);
//	m_tempCurrentObjects2.Sort(new Comparison<ZDO>(ZNetScene.ZDOCompare));
//	for (auto zdo2 : m_tempCurrentObjects2) {
//		if (CreateObject(zdo2)) {
//			created++;
//			if (created > num) {
//				break;
//			}
//		}
//		else {
//            //LOG(INFO) << "Destroyed invalid prefab ZDO " << zdo2->ID();
//            zdo2->SetLocal();
//			NetSyncManager::DestroyZDO(zdo2);
//		}
//	}
//}

// TODO make sort lambda runnable
//static int ZDOCompare(ZDO *x, ZDO *y) {
//	if (x->Type() == y->Type()) {
//		return VUtils.CompareFloats(x.m_tempSortValue, y.m_tempSortValue);
//	}
//	int type = (int)y->Type();
//	return type.CompareTo((int)x->Type());
//}

//void NetScene::CreateDistantObjects(std::vector<ZDO*> objects, int maxCreatedPerFrame) {
//	if (created > maxCreatedPerFrame)
//		return;
//
//	int frameCount = Time.frameCount;
//	for (auto zdo : objects) {
//		if (zdo.m_tempCreateEarmark != frameCount) {
//			if (CreateObject(zdo)) {
//				created++;
//				if (created > maxCreatedPerFrame)
//					break;
//			}
//			else {
//                LOG(INFO) << "Destroyed invalid prefab ZDO " << zdo->ID().m_id << " " << zdo->ID().m_uuid;
//				zdo->SetLocal();
//				NetSyncManager::MarkDestroyZDO(zdo);
//			}
//		}
//	}
//}

void NetScene::OnZDODestroyed(ZDO *zdo) {
	assert(false);
    auto&& find = m_instances.find(zdo);
    if (find != m_instances.end()) {
        auto&& znetView = find->second;
		znetView->ResetNetSync();
		//UnityEngine.Object.Destroy(znetView.gameObject);
		m_instances.erase(find);
	}
}

// useless too
//void NetScene::RemoveObjects(std::vector<ZDO*> &currentNearObjects, std::vector<ZDO*> &currentDistantObjects) {
//	int frameCount = Time.frameCount;
//
//	for (auto zdo : currentNearObjects)
//		zdo.m_tempRemoveEarmark = frameCount;
//
//	for (ZDO zdo2 in currentDistantObjects)
//		zdo2.m_tempRemoveEarmark = frameCount;
//
//	m_tempRemoved.clear();
//    for (auto&& pair : m_instances) {
//        auto&& znetView = pair.second;
//		if (znetView->GetNetSync().m_tempRemoveEarmark != frameCount)
//			m_tempRemoved.push_back(znetView);
//	}
//
//	for (int i = 0; i < m_tempRemoved.size(); i++) {
//		auto znetView2 = m_tempRemoved[i];
//		auto zdo3 = znetView2->GetNetSync();
//		znetView2->ResetNetSync();
//		UnityEngine.Object.Destroy(znetView2.gameObject);
//		if (!zdo3->Persists() && zdo3->Local()) {
//			NetSyncManager::DestroyZDO(zdo3);
//		}
//		m_instances.erase(zdo3);
//	}
//}

// through debugging with dnspy, it appears to be client only
// the ONLY objects the server gathers are ones that it just created for this... _zoneCtrl,
// which are in a far unused corner of the map 
// will always create and destroy 25 useless _zoneCtrl
//void NetScene::CreateDestroyObjects() {
//    // TODO client
//	auto zone = ZoneSystem::GetZoneCoords(NetManager::GetReferencePosition());
//	m_tempCurrentObjects.clear();
//	m_tempCurrentDistantObjects.clear();
//	NetSyncManager::FindSectorObjects(zone, ZoneSystem::ACTIVE_AREA, ZoneSystem::ACTIVE_DISTANT_AREA, m_tempCurrentObjects, m_tempCurrentDistantObjects);
//	CreateObjects(m_tempCurrentObjects, m_tempCurrentDistantObjects);
//	RemoveObjects(m_tempCurrentObjects, m_tempCurrentDistantObjects);
//}

//void NetScene::RPC_SpawnObject(OWNER_t spawner, Vector3 pos, Quaternion rot, HASH_t prefabHash) {
//	GameObject prefab = GetPrefab(prefabHash);
//	if (!prefab) {
//		LOG(INFO) << "Missing prefab " << prefabHash;
//		return;
//	}
//	UnityEngine.Object.Instantiate<GameObject>(prefab, pos, rot);
//}
