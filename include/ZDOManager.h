#pragma once

#include <vector>

#include "Vector.h"
#include "ZDO.h"

// Forward declaration
class NetPeer;
class ZDOPeer;

class IZDOManager {
	static constexpr int SECTOR_WIDTH = 512; // The width of world in zones (the actual world is smaller than this at 315)
	static constexpr int MAX_DEAD_OBJECTS = 100000;

private:
	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

	// Responsible for managing ZDOs lifetimes
	robin_hood::unordered_map<NetID, std::unique_ptr<ZDO>> m_objectsByID;

	// Contains ZDOs according to Zone
	std::array<robin_hood::unordered_set<NetSync*>, (SECTOR_WIDTH * SECTOR_WIDTH)> m_objectsBySector;

	// Primarily used in RPC_ZDOData
	robin_hood::unordered_map<NetID, int64_t> m_deadZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<NetID> m_destroySendList;

private:
	void Init();
	void OnNewPeer(NetPeer* peer);
	void OnPeerQuit(NetPeer* peer);

	bool IsInPeerActiveArea(const Vector2i& sector, OWNER_t id) const;
	void ReleaseNearbyZDOS(const Vector3& refPosition, OWNER_t id);
	void HandleDestroyedZDO(const NetID& uid);
	void SendAllZDOs(ZDOPeer* peer);
	bool SendZDOs(ZDOPeer* peer, bool flush);
	void RPC_ZDOData(NetRpc* rpc, NetPackage pkg);
	void CreateSyncList(ZDOPeer* peer, std::vector<NetSync*>& toSync);
	void AddForceSendZDOs(ZDOPeer* peer, std::vector<NetSync*>& syncList);

	NetSync* CreateNewZDO(const Vector3& position);
	NetSync* CreateNewZDO(const NetID& uid, const Vector3& position);

	void ServerSortSendZDOS(std::vector<NetSync*>& objects, const Vector3& refPos, ZDOPeer* peer);

	int SectorToIndex(const Vector2i& s);
	void FindObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void FindDistantObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void RemoveOrphanNonPersistentZDOS();
	bool IsPeerConnected(OWNER_t uid);

public:
	// Used when saving the world from disk
	void Save(NetPackage& pkg);

	// Used when loading the world from disk
	void Load(NetPackage& reader, int version);

	void CapDeadZDOList();

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
