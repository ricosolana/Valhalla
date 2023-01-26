#pragma once

#include <vector>

#include "Vector.h"
#include "ZDO.h"
#include "Peer.h"
#include "PrefabManager.h"

class IZDOManager {
	friend class IPrefabManager;

	static constexpr int WIDTH_IN_ZONES = 512; // The width of world in zones (the actual world is smaller than this at 315)
	static constexpr int MAX_DEAD_OBJECTS = 100000;

private:
	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

	// Responsible for managing ZDOs lifetimes
	robin_hood::unordered_map<NetID, std::unique_ptr<ZDO>> m_objectsByID;

	// Contains ZDOs according to Zone
	std::array<robin_hood::unordered_set<ZDO*>, (WIDTH_IN_ZONES* WIDTH_IN_ZONES)> m_objectsBySector;

	// Primarily used in RPC_ZDOData
	robin_hood::unordered_map<NetID, TICKS_t> m_deadZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<NetID> m_destroySendList;

private:
	void OnNewPeer(Peer* peer);
	void OnPeerQuit(Peer* peer);

	void ReleaseNearbyZDOS(const Vector3& refPosition, Peer* peer);
	void HandleDestroyedZDO(const NetID& uid);
	void SendAllZDOs(Peer* peer);
	bool SendZDOs(Peer* peer, bool flush);
	void CreateSyncList(Peer* peer, std::vector<ZDO*>& toSync);
	void AddForceSendZDOs(Peer* peer, std::vector<ZDO*>& syncList);

	ZDO* CreateZDO(const Vector3& position);
	ZDO* CreateZDO(const NetID& uid, const Vector3& position);

	void ServerSortSendZDOS(std::vector<ZDO*>& objects, Peer* peer);

	static int SectorToIndex(const Vector2i& s);
	void FindObjects(const Vector2i& sector, std::vector<ZDO*>& objects);
	void FindDistantObjects(const Vector2i& sector, std::vector<ZDO*>& objects);
	void RemoveOrphanNonPersistentZDOS();
	bool IsPeerConnected(OWNER_t uid);

public:
	void Init();

	// Used when saving the world from disk
	void Save(NetPackage& pkg);

	// Used when loading the world from disk
	void Load(NetPackage& reader, int version);

	void CapDeadZDOList();

	// Sector Coords -> Sector Pitch
	// Returns -1 on invalid sector
	void AddToSector(ZDO* zdo, const Vector2i& sector);

	// used by zdo to self invalidate its sector
	void ZDOSectorInvalidated(ZDO* zdo);

	void RemoveFromSector(ZDO* zdo, const Vector2i& sector);

	ZDO* GetZDO(const NetID& id);

	// called when registering joining peer

	void Update();

	void MarkDestroyZDO(ZDO* zdo);

	void FindSectorObjects(const Vector2i& sector, int area, int distantArea,
		std::vector<ZDO*>& sectorObjects, std::vector<ZDO*>* distantSectorObjects = nullptr);

	void FindSectorObjects(const Vector2i& sector, int area, std::vector<ZDO*>& sectorObjects);

	//long GetMyID();

	void GetAllZDOsWithPrefab(const std::string& prefab, std::vector<ZDO*> zdos);

	// Used to get portals incrementally in a coroutine
	// basically, the coroutine thread is frozen in place
	// its not real multithreading, but is confusing for no reason
	// this can be refactored to have clearer intent
	//bool GetAllZDOsWithPrefabIterative(const std::string& prefab, std::vector<ZDO*> &zdos, int& index);

	void ForceSendZDO(const NetID& id);
};

IZDOManager* ZDOManager();
