#include "NetScene.h"

namespace NetScene {

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

	void RPC_SpawnObject(UUID_t spawner, Vector3 pos, Quaternion rot, HASH_t prefabHash);



	// defined publics
	void Init() {
		ZLog.Log("m_prefabs:");
		for (auto &&gameObject : m_prefabs) {
			ZLog.Log(gameObject.name + ", " + gameObject.name.GetStableHashCode());
			this.m_namedPrefabs.Add(gameObject.name.GetStableHashCode(), gameObject);
		}
		ZLog.Log("m_nonNetViewPrefabs:");
		foreach(GameObject gameObject2 in this.m_nonNetViewPrefabs)
		{
			ZLog.Log(gameObject2.name + ", " + gameObject2.name.GetStableHashCode());
			this.m_namedPrefabs.Add(gameObject2.name.GetStableHashCode(), gameObject2);
		}
		ZDOMan instance = ZDOMan.instance;
		instance.m_onZDODestroyed = (Action<ZDO>)Delegate.Combine(instance.m_onZDODestroyed, new Action<ZDO>(this.OnZDODestroyed));
		ZRoutedRpc.instance.Register<Vector3, Quaternion, int>("SpawnObject", new Action<long, Vector3, Quaternion, int>(this.RPC_SpawnObject));
	}

	void Shutdown() {
		foreach(KeyValuePair<ZDO, ZNetView> keyValuePair in this.m_instances)
		{
			if (keyValuePair.Value)
			{
				keyValuePair.Value.ResetZDO();
				UnityEngine.Object.Destroy(keyValuePair.Value.gameObject);
			}
		}
		this.m_instances.Clear();
		base.enabled = false;
	}

	void AddInstance(ZDO zdo, ZNetView nview) {
		this.m_instances[zdo] = nview;
	}

	void Destroy(GameObject go) {
		ZNetView component = go.GetComponent<ZNetView>();
		if (component && component.GetZDO() != null)
		{
			ZDO zdo = component.GetZDO();
			component.ResetZDO();
			this.m_instances.Remove(zdo);
			if (zdo.IsOwner())
			{
				ZDOMan.instance.DestroyZDO(zdo);
			}
		}
		UnityEngine.Object.Destroy(go);
	}

	GameObject GetPrefab(int hash) {
		GameObject result;
		if (this.m_namedPrefabs.TryGetValue(hash, out result))
		{
			return result;
		}
		return null;
	}

	GameObject GetPrefab(string name) {
		int stableHashCode = name.GetStableHashCode();
		return this.GetPrefab(stableHashCode);
	}

	int GetPrefabHash(GameObject go) {
		return go.name.GetStableHashCode();
	}

	bool IsAreaReady(Vector3 point)
	{
		Vector2i zone = ZoneSystem.instance.GetZone(point);
		if (!ZoneSystem.instance.IsZoneLoaded(zone))
		{
			return false;
		}
		this.m_tempCurrentObjects.Clear();
		ZDOMan.instance.FindSectorObjects(zone, 1, 0, this.m_tempCurrentObjects, null);
		foreach(ZDO zdo in this.m_tempCurrentObjects)
		{
			if (this.IsPrefabZDOValid(zdo) && !this.FindInstance(zdo))
			{
				return false;
			}
		}
		return true;
	}

	ZNetView FindInstance(ZDO zdo)
	{
		ZNetView result;
		if (this.m_instances.TryGetValue(zdo, out result))
		{
			return result;
		}
		return null;
	}

	bool HaveInstance(ZDO zdo)
	{
		return this.m_instances.ContainsKey(zdo);
	}

	GameObject FindInstance(ZDOID id)
	{
		ZDO zdo = ZDOMan.instance.GetZDO(id);
		if (zdo != null)
		{
			ZNetView znetView = this.FindInstance(zdo);
			if (znetView)
			{
				return znetView.gameObject;
			}
		}
		return null;
	}

	void Update()
	{
		float deltaTime = Time.deltaTime;
		this.m_createDestroyTimer += deltaTime;
		if (this.m_createDestroyTimer >= 0.033333335f)
		{
			this.m_createDestroyTimer = 0f;
			this.CreateDestroyObjects();
		}
	}

	bool InActiveArea(Vector2i zone, Vector3 refPoint)
	{
		Vector2i zone2 = ZoneSystem.instance.GetZone(refPoint);
		return this.InActiveArea(zone, zone2);
	}

	bool InActiveArea(Vector2i zone, Vector2i refCenterZone)
	{
		int num = ZoneSystem.instance.m_activeArea - 1;
		return zone.x >= refCenterZone.x - num && zone.x <= refCenterZone.x + num && zone.y <= refCenterZone.y + num && zone.y >= refCenterZone.y - num;
	}

	bool OutsideActiveArea(Vector3 point)
	{
		return this.OutsideActiveArea(point, ZNet.instance.GetReferencePosition());
	}

	bool OutsideActiveArea(Vector3 point, Vector3 refPoint)
	{
		Vector2i zone = ZoneSystem.instance.GetZone(refPoint);
		Vector2i zone2 = ZoneSystem.instance.GetZone(point);
		return zone2.x <= zone.x - ZoneSystem.instance.m_activeArea || zone2.x >= zone.x + ZoneSystem.instance.m_activeArea || zone2.y >= zone.y + ZoneSystem.instance.m_activeArea || zone2.y <= zone.y - ZoneSystem.instance.m_activeArea;
	}

	bool HaveInstanceInSector(Vector2i sector)
	{
		foreach(KeyValuePair<ZDO, ZNetView> keyValuePair in this.m_instances)
		{
			if (keyValuePair.Value && !keyValuePair.Value.m_distant && ZoneSystem.instance.GetZone(keyValuePair.Value.transform.position) == sector)
			{
				return true;
			}
		}
		return false;
	}

	int NrOfInstances()
	{
		return this.m_instances.Count;
	}

	void SpawnObject(Vector3 pos, Quaternion rot, GameObject prefab)
	{
		int prefabHash = this.GetPrefabHash(prefab);
		ZRoutedRpc.instance.InvokeRoutedRPC(ZRoutedRpc.Everybody, "SpawnObject", new object[]
			{
				pos,
				rot,
				prefabHash
			});
	}

	List<string> GetPrefabNames()
	{
		List<string> list = new List<string>();
		foreach(KeyValuePair<int, GameObject> keyValuePair in this.m_namedPrefabs)
		{
			list.Add(keyValuePair.Value.name);
		}
		return list;
	}





	// defined privates

	bool IsPrefabZDOValid(ZDO zdo) {
		int prefab = zdo.GetPrefab();
		return prefab != 0 && !(this.GetPrefab(prefab) == null);
	}

	GameObject CreateObject(ZDO zdo) {
		int prefab = zdo.GetPrefab();
		if (prefab == 0)
		{
			return null;
		}
		GameObject prefab2 = this.GetPrefab(prefab);
		if (prefab2 == null)
		{
			return null;
		}
		Vector3 position = zdo.GetPosition();
		Quaternion rotation = zdo.GetRotation();
		ZNetView.m_useInitZDO = true;
		ZNetView.m_initZDO = zdo;
		GameObject result = UnityEngine.Object.Instantiate<GameObject>(prefab2, position, rotation);
		if (ZNetView.m_initZDO != null)
		{
			string str = "ZDO ";
			ZDOID uid = zdo.m_uid;
			ZLog.LogWarning(str + uid.ToString() + " not used when creating object " + prefab2.name);
			ZNetView.m_initZDO = null;
		}
		ZNetView.m_useInitZDO = false;
		return result;
	}

	private bool InLoadingScreen()
	{
		return Player.m_localPlayer == null || Player.m_localPlayer.IsTeleporting();
	}

	private void CreateObjects(List<ZDO> currentNearObjects, List<ZDO> currentDistantObjects)
	{
		int maxCreatedPerFrame = 10;
		if (this.InLoadingScreen())
		{
			maxCreatedPerFrame = 100;
		}
		int frameCount = Time.frameCount;
		foreach(ZDO zdo in this.m_instances.Keys)
		{
			zdo.m_tempCreateEarmark = frameCount;
		}
		int num = 0;
		this.CreateObjectsSorted(currentNearObjects, maxCreatedPerFrame, ref num);
		this.CreateDistantObjects(currentDistantObjects, maxCreatedPerFrame, ref num);
	}

	private void CreateObjectsSorted(List<ZDO> currentNearObjects, int maxCreatedPerFrame, ref int created)
	{
		if (!ZoneSystem.instance.IsActiveAreaLoaded())
		{
			return;
		}
		this.m_tempCurrentObjects2.Clear();
		int frameCount = Time.frameCount;
		Vector3 referencePosition = ZNet.instance.GetReferencePosition();
		foreach(ZDO zdo in currentNearObjects)
		{
			if (zdo.m_tempCreateEarmark != frameCount)
			{
				zdo.m_tempSortValue = Utils.DistanceSqr(referencePosition, zdo.GetPosition());
				this.m_tempCurrentObjects2.Add(zdo);
			}
		}
		int num = Mathf.Max(this.m_tempCurrentObjects2.Count / 100, maxCreatedPerFrame);
		this.m_tempCurrentObjects2.Sort(new Comparison<ZDO>(ZNetScene.ZDOCompare));
		foreach(ZDO zdo2 in this.m_tempCurrentObjects2)
		{
			if (this.CreateObject(zdo2) != null)
			{
				created++;
				if (created > num)
				{
					break;
				}
			}
			else if (ZNet.instance.IsServer())
			{
				zdo2.SetOwner(ZDOMan.instance.GetMyID());
				string str = "Destroyed invalid predab ZDO:";
				ZDOID uid = zdo2.m_uid;
				ZLog.Log(str + uid.ToString());
				ZDOMan.instance.DestroyZDO(zdo2);
			}
		}
	}

	private static int ZDOCompare(ZDO x, ZDO y)
	{
		if (x.m_type == y.m_type)
		{
			return Utils.CompareFloats(x.m_tempSortValue, y.m_tempSortValue);
		}
		int type = (int)y.m_type;
		return type.CompareTo((int)x.m_type);
	}

	private void CreateDistantObjects(List<ZDO> objects, int maxCreatedPerFrame, ref int created)
	{
		if (created > maxCreatedPerFrame)
		{
			return;
		}
		int frameCount = Time.frameCount;
		foreach(ZDO zdo in objects)
		{
			if (zdo.m_tempCreateEarmark != frameCount)
			{
				if (this.CreateObject(zdo) != null)
				{
					created++;
					if (created > maxCreatedPerFrame)
					{
						break;
					}
				}
				else if (ZNet.instance.IsServer())
				{
					zdo.SetOwner(ZDOMan.instance.GetMyID());
					string str = "Destroyed invalid predab ZDO:";
					ZDOID uid = zdo.m_uid;
					ZLog.Log(str + uid.ToString() + "  prefab hash:" + zdo.GetPrefab().ToString());
					ZDOMan.instance.DestroyZDO(zdo);
				}
			}
		}
	}

	private void OnZDODestroyed(ZDO zdo)
	{
		ZNetView znetView;
		if (this.m_instances.TryGetValue(zdo, out znetView))
		{
			znetView.ResetZDO();
			UnityEngine.Object.Destroy(znetView.gameObject);
			this.m_instances.Remove(zdo);
		}
	}

	private void RemoveObjects(List<ZDO> currentNearObjects, List<ZDO> currentDistantObjects)
	{
		int frameCount = Time.frameCount;
		foreach(ZDO zdo in currentNearObjects)
		{
			zdo.m_tempRemoveEarmark = frameCount;
		}
		foreach(ZDO zdo2 in currentDistantObjects)
		{
			zdo2.m_tempRemoveEarmark = frameCount;
		}
		this.m_tempRemoved.Clear();
		foreach(ZNetView znetView in this.m_instances.Values)
		{
			if (znetView.GetZDO().m_tempRemoveEarmark != frameCount)
			{
				this.m_tempRemoved.Add(znetView);
			}
		}
		for (int i = 0; i < this.m_tempRemoved.Count; i++)
		{
			ZNetView znetView2 = this.m_tempRemoved[i];
			ZDO zdo3 = znetView2.GetZDO();
			znetView2.ResetZDO();
			UnityEngine.Object.Destroy(znetView2.gameObject);
			if (!zdo3.m_persistent && zdo3.IsOwner())
			{
				ZDOMan.instance.DestroyZDO(zdo3);
			}
			this.m_instances.Remove(zdo3);
		}
	}

	private void CreateDestroyObjects()
	{
		Vector2i zone = ZoneSystem.instance.GetZone(ZNet.instance.GetReferencePosition());
		this.m_tempCurrentObjects.Clear();
		this.m_tempCurrentDistantObjects.Clear();
		ZDOMan.instance.FindSectorObjects(zone, ZoneSystem.instance.m_activeArea, ZoneSystem.instance.m_activeDistantArea, this.m_tempCurrentObjects, this.m_tempCurrentDistantObjects);
		this.CreateObjects(this.m_tempCurrentObjects, this.m_tempCurrentDistantObjects);
		this.RemoveObjects(this.m_tempCurrentObjects, this.m_tempCurrentDistantObjects);
	}

	void RPC_SpawnObject(long spawner, Vector3 pos, Quaternion rot, int prefabHash)
	{
		GameObject prefab = this.GetPrefab(prefabHash);
		if (prefab == null)
		{
			ZLog.Log("Missing prefab " + prefabHash.ToString());
			return;
		}
		UnityEngine.Object.Instantiate<GameObject>(prefab, pos, rot);
	}

	// Token: 0x04000D30 RID: 3376
	private const int m_maxCreatedPerFrame = 10;

	// Token: 0x04000D31 RID: 3377
	private const int m_maxDestroyedPerFrame = 20;

	// Token: 0x04000D32 RID: 3378
	private const float m_createDestroyFps = 30f;


	// Token: 0x04000D35 RID: 3381
	private Dictionary<int, GameObject> m_namedPrefabs = new Dictionary<int, GameObject>();

	// Token: 0x04000D36 RID: 3382
	private Dictionary<ZDO, ZNetView> m_instances = new Dictionary<ZDO, ZNetView>(new ZDOComparer());

	// Token: 0x04000D37 RID: 3383
	private List<ZDO> m_tempCurrentObjects = new List<ZDO>();

	// Token: 0x04000D38 RID: 3384
	private List<ZDO> m_tempCurrentObjects2 = new List<ZDO>();

	// Token: 0x04000D39 RID: 3385
	private List<ZDO> m_tempCurrentDistantObjects = new List<ZDO>();

	// Token: 0x04000D3A RID: 3386
	private List<ZNetView> m_tempRemoved = new List<ZNetView>();

	// Token: 0x04000D3B RID: 3387
	private HashSet<ZDO> m_tempActiveZDOs = new HashSet<ZDO>(new ZDOComparer());

	// Token: 0x04000D3C RID: 3388
	private float m_createDestroyTimer;

}