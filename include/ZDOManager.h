#pragma once

#include <vector>
#include <functional>

#include "Vector.h"
#include "ZDO.h"
#include "Peer.h"

class IZDOManager {
	friend class INetManager;
	friend class IValhalla;
	friend class ZDO;
	friend class IZoneManager;

private:
	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

	// Responsible for managing ZDOs lifetimes
	//	If a value is nullptr, the zdo is considered dead (this is to save memory)
	UNORDERED_MAP_t<ZDOID, std::unique_ptr<ZDO>> m_objectsByID;

	UNORDERED_MAP_t<ZoneID, UNORDERED_SET_t<ZDO*>> m_objectsByZone;

	// Contains recently destroyed ZDOs to be sent
	std::vector<ZDOID> m_destroySendList;

	BYTES_t m_temp;

private:
	// Called when an authenticated peer joins (internal)
	void OnNewPeer(Peer& peer);
	// Called when an authenticated peer leaves (internal)
	void OnPeerQuit(Peer& peer);

	// Insert a ZDO into zone (internal)
	bool AddZDOToZone(ZDO& zdo);
	// Remove a zdo from a zone (internal)
	void RemoveFromSector(ZDO& zdo);
	// Relay a ZDO sector change to clients (internal)
	void InvalidateZDOZone(ZDO& zdo);

	void AssignOrReleaseZDOs(Peer& peer);
	//void SmartAssignZDOs();

	decltype(m_objectsByID)::iterator DestroyZDO(decltype(m_objectsByID)::iterator itr);

	// Performs an unchecked erasure
	//	Does not check whether the iterator is at end of container
	decltype(m_objectsByID)::iterator EraseZDO(decltype(m_objectsByID)::iterator itr);
	void EraseZDO(ZDOID uid);
	void SendAllZDOs(Peer& peer);
	bool SendZDOs(Peer& peer, bool flush);
	std::list<std::pair<std::reference_wrapper<ZDO>, float>> CreateSyncList(Peer& peer);
			
	// Get a ZDO by id
	//	The ZDO will be created if its ID does not exist
	//	Returns the ZDO and a bool if newly created
	//	Returns key: null if the zdoid is none 
	//	Returns key: null if the zdo is null (zdo is destroyed)
	std::pair<decltype(IZDOManager::m_objectsByID)::iterator, bool> GetOrInstantiate(ZDOID id, Vector3f def);

	// Performs a coordinate to pitch conversion
	int SectorToIndex(ZoneID zone) const;

public:
	void Init();
	void Update();

	// Used when saving the world from disk
	void Save(DataWriter& writer);

	// Used when loading the world from disk
	void Load(DataReader& reader, int version);

	// Get a ZDO by id
	//	TODO use optional<reference>
	ZDO* GetZDO(ZDOID id);

	// Get all ZDOs strictly within a zone
	void GetZDOs_Zone(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within neighboring zones
	void GetZDOs_NeighborZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within distant zones
	void GetZDOs_DistantZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within a zone, its neighboring zones, and its distant zones
	void GetZDOs_ActiveZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out, std::list<std::reference_wrapper<ZDO>>& outDistant);
	// Get all ZDOs strictly within a zone that are distant flagged
	void GetZDOs_Distant(ZoneID sector, std::list<std::reference_wrapper<ZDO>>& objects);
	


	void ForceSendZDO(ZDOID id);

	// Erases a ZDO on clients and server
	//	Warning: the target ZDO is freed from memory and will become no longer accessible
	//void DestroyZDO(ZDO& zdo);
	void DestroyZDO(ZDOID zdoid);
	void DestroyZDO(const ZDO& zdo) {
		DestroyZDO(zdo.ID());
	}

	size_t GetSumZDOMembers();
	float GetMeanZDOMembers();
	float GetStDevZDOMembers();
	size_t GetTotalZDOAlloc();

	size_t GetCountEmptyZDOs();
};

// Manager class for everything related to networked object synchronization
IZDOManager* ZDOManager();
