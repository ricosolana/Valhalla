#pragma once

#include <vector>

#include "Vector.h"
#include "ZDO.h"

// Forward declaration
class NetPeer;
class ZDOPeer;

class IZDOManager {

private:
	void OnNewPeer(NetPeer* peer);
	void OnPeerQuit(NetPeer* peer);

	void Init();	

	bool IsInPeerActiveArea(const Vector2i& sector, OWNER_t id);
	void ReleaseNearbyZDOS(const Vector3& refPosition, OWNER_t id);
	void HandleDestroyedZDO(const NetID& uid);
	void SendAllZDOs(ZDOPeer* peer);
	bool SendZDOs(ZDOPeer* peer, bool flush);
	void RPC_ZDOData(NetRpc* rpc, NetPackage pkg);
	void CreateSyncList(ZDOPeer* peer, std::vector<NetSync*>& toSync);
	void AddForceSendZDOs(ZDOPeer* peer, std::vector<NetSync*>& syncList);

	void ServerSortSendZDOS(std::vector<NetSync*>& objects, const Vector3& refPos, ZDOPeer* peer);

	int SectorToIndex(const Vector2i& s);
	void FindObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void FindDistantObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void RemoveOrphanNonPersistentZDOS();
	bool IsPeerConnected(OWNER_t uid);

	static constexpr int SECTOR_WIDTH = 512; // The width of world in zones (the actual world is smaller than this at 315)
	static constexpr int MAX_DEAD_OBJECTS = 100000;

	//std::list<std::unique_ptr<SyncPeer>> m_peers; // Peer lifetimes



	// so ensure with a bunch of asserts of something that all ZDO external references are removed once the zdo is popped from here
	robin_hood::unordered_map<NetID, std::unique_ptr<ZDO>> m_objectsByID;    // primary lifetime container



	std::array<robin_hood::unordered_set<NetSync*>, SECTOR_WIDTH* SECTOR_WIDTH> m_objectsBySector;   // a bunch of objects
	// TODO this might be essentially never used in game
	robin_hood::unordered_map<Vector2i, robin_hood::unordered_set<NetSync*>> m_objectsByOutsideSector;

	//constexpr static int s00 = sizeof(m_objectsBySector);
	//constexpr static int s01 = sizeof(robin_hood::unordered_map<Vector2i, robin_hood::unordered_set<NetSync*>, HashUtils::Hasher>);

	robin_hood::unordered_map<NetID, int64_t> m_deadZDOs;
	//std::vector<NetSync*> tempSectorObjects;
	std::vector<NetID> m_destroySendList;

	// Increments indefinitely for new ZDO id
	uint32_t m_nextUid = 1;

public:
	// Used when saving the world from disk
	void Save(NetPackage& pkg);

	// Used when loading the world from disk
	void Load(NetPackage& reader, int version);

	void CapDeadZDOList();

	NetSync* CreateNewZDO(const Vector3& position);

	NetSync* CreateNewZDO(const NetID& uid, const Vector3& position);

	// Sector Coords -> Sector Pitch
	// Returns -1 on invalid sector
	void AddToSector(NetSync* zdo, const Vector2i& sector);

	// used by zdo to self invalidate its sector
	void ZDOSectorInvalidated(NetSync* zdo);

	void RemoveFromSector(NetSync* zdo, const Vector2i& sector);

	NetSync* GetZDO(const NetID& id);

	// called when registering joining peer

	void Update();

	void MarkDestroyZDO(NetSync* zdo);

	void FindSectorObjects(const Vector2i& sector, int area, int distantArea,
		std::vector<NetSync*>& sectorObjects, std::vector<NetSync*>* distantSectorObjects = nullptr);

	void FindSectorObjects(const Vector2i& sector, int area, std::vector<NetSync*>& sectorObjects);

	//long GetMyID();

	void GetAllZDOsWithPrefab(const std::string& prefab, std::vector<NetSync*> zdos);

	// Used to get portals incrementally in a coroutine
	// basically, the coroutine thread is frozen in place
	// its not real multithreading, but is confusing for no reason
	// this can be refactored to have clearer intent
	bool GetAllZDOsWithPrefabIterative(const std::string& prefab, std::vector<NetSync*> &zdos, int& index);

	void ForceSendZDO(const NetID& id);
};

IZDOManager* ZDOManager();
