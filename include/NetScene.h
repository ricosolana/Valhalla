#pragma once

#include "ZDO.h"
#include "NetObject.h"

class NetScene {

	static constexpr int m_maxCreatedPerFrame = 10;
	static constexpr int m_maxDestroyedPerFrame = 20;
	static constexpr float m_createDestroyFps = 30;

	// Holds ready to instantiate prefabs as template-like objects
	// 

	// also seemingly unused by server
	std::vector<std::unique_ptr<GameObject>> m_prefabs;
	robin_hood::unordered_map<HASH_t, GameObject> m_namedPrefabs;

	// 
	robin_hood::unordered_map<ZDO*, ZNetView*> m_instances; // comparer: new ZDOComparer();

	std::vector<ZDO*> m_tempCurrentObjects;
	std::vector<ZDO*> m_tempCurrentObjects2; // redundant?
	std::vector<ZDO*> m_tempCurrentDistantObjects;
	std::vector<ZNetView*> m_tempRemoved;

	robin_hood::unordered_set<ZDO*> m_tempActiveZDOs; //new ZDOComparer();

	float m_createDestroyTimer;

	// forward declared privates

	void Init();

	void Shutdown();

	//bool IsPrefabZDOValid(ZDO *zdo);
	//GameObject CreateObject(ZDO* zdo);
	//void CreateObjects(std::vector<ZDO*>& currentNearObjects, std::vector<ZDO*>& currentDistantObjects);
	//void CreateObjectsSorted(std::vector<ZDO*> currentNearObjects, int maxCreatedPerFrame);
	//static int ZDOCompare(ZDO *x, ZDO *y);
	//void CreateDistantObjects(std::vector<ZDO*> objects, int maxCreatedPerFrame);
	void OnZDODestroyed(ZDO* zdo);
	//void RemoveObjects(std::vector<ZDO*>& currentNearObjects, std::vector<ZDO*>& currentDistantObjects);
	//void CreateDestroyObjects();
	//void RPC_SpawnObject(OWNER_t spawner, Vector3 pos, Quaternion rot, HASH_t prefabHash);

public:
	void AddInstance(NetSync* zdo, ZNetView* nview);

	// Uncertain client? uses:
	//	TeleportAbility.Setup
	//	Tameable.RPC_UnSummon
	//	ZNetView.Destroy()...
	//		SE_Demister.RemoveEffects()
	//		StatusEffect.RemoveStartEffects()	
	// through some rather heavy testing, 
	// im fairly certain that the usages
	// of Destroy are completely client managed
	// I could be wrong, however the patterns
	// of usage clearly point to heavy use
	// by the client	
	//void Destroy(GameObject go);

	//GameObject GetPrefab(int hash);

	// im pretty sure this is client only too
	// only Turret.GetAmmoItem is questionable
	//		Turret.UpdateRotation is performed on both client/server?
	//GameObject GetPrefab(const std::string &name);

	// Also bonfire only (when throwing surtling)
	//int GetPrefabHash(GameObject go);

	// client only
	//bool IsAreaReady(const Vector3& point);

	// Fish
	//	Unity.OnCollision...only client handles physics though...
	//	Unspecific: FishingFloat.RPC_Nibble?
	//	Just test everything

	// Additionally
	//	A large majority of ZNetScene methods are 
	//		primarily client-only
	// Within Tameable
	
	// Unable to perform any exact usage tests,
	//	unless deepest breakpoint reached

	// Seriously, usages of both of these are mixed
	//	Usages are quite sparse in server
	ZNetView* FindInstance(NetSync* zdo);
	bool HaveInstance(NetSync* zdo);
	//GameObject FindInstance(const ZDOID &id);

	//void Update();

	// TODO: move to ZoneSystem
	//	this doesnt actually test whether an activearea was loaded,
	//	does simple comparison only
	static bool InActiveArea(const Vector2i& zone, const Vector3& areaPoint);

	// TODO: move to ZoneSystem
	//	looks only at points
	static bool InActiveArea(const Vector2i& zone, const Vector2i& areaZone);

	// Used by client only
	// TODO: move to ZoneSystem
	//bool OutsideActiveArea(const Vector3 &point);

	// TODO: move to ZoneSystem
	//bool OutsideActiveArea(const Vector3 &point, const Vector3 &refPoint);

	// also client only really uses this (UpdateTTL, used by..)
	//bool HaveInstanceInSector(const Vector2i &sector);

	// client connect panel only
	//int NrOfInstances();

	// used only by bonfire surtling throw in
	//void SpawnObject(const Vector3 &pos, const Quaternion &rot, GameObject prefab);

	//List<string> GetPrefabNames();

	//List<GameObject> m_prefabs = new List<GameObject>();

	//List<GameObject> m_nonNetViewPrefabs = new List<GameObject>();

};
