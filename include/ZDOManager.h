#pragma once

#include <vector>

#include "Vector.h"
#include "ZDO.h"
#include "Peer.h"
#include "PrefabManager.h"
#include "ZoneManager.h"

class IZDOManager {
	friend class IPrefabManager;
	friend class INetManager;
	friend class ZDO;

	//friend void AddToSector(ZDO* zdo);
	//friend void RemoveFromSector(ZDO* zdo);
	
	//static constexpr int WIDTH_IN_ZONES = 512; // The width of world in zones (the actual world is smaller than this at 315)
	static constexpr int MAX_DEAD_OBJECTS = 100000;

private:
	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

	// Responsible for managing ZDOs lifetimes
	robin_hood::unordered_map<NetID, std::unique_ptr<ZDO>> m_objectsByID;

	// Contains ZDOs according to Zone
	std::array<robin_hood::unordered_set<ZDO*>, 
		(IZoneManager::WORLD_RADIUS_IN_ZONES * IZoneManager::WORLD_RADIUS_IN_ZONES * 2 * 2)> m_objectsBySector; // takes up around 5MB; could be around 72 bytes with map

	// Contains ZDOs according to prefab
	robin_hood::unordered_map<HASH_t, robin_hood::unordered_set<ZDO*>> m_objectsByPrefab;

	// Primarily used in RPC_ZDOData
	robin_hood::unordered_map<NetID, TICKS_t> m_deadZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<NetID> m_destroySendList;

private:
	// Called when an authenticated peer joins (internal)
	void OnNewPeer(Peer* peer);
	// Called when an authenticated peer leaves (internal)
	void OnPeerQuit(Peer* peer);

	// Insert a ZDO into zone (internal)
	bool AddToSector(ZDO* zdo);
	// Remove a zdo from a zone (internal)
	void RemoveFromSector(ZDO* zdo);
	// Relay a ZDO sector change to clients (internal)
	void InvalidateSector(ZDO* zdo);

	void AssignOrReleaseZDOs(Peer* peer);
	//void SmartAssignZDOs();

	void EraseZDO(const NetID& uid);
	void SendAllZDOs(Peer* peer);
	bool SendZDOs(Peer* peer, bool flush);
	std::list<ZDO*> CreateSyncList(Peer* peer);

	ZDO* AddZDO(const Vector3& position);
	ZDO* AddZDO(const NetID& uid, const Vector3& position);
		
	// Performs a coordinate to pitch conversion
	int SectorToIndex(const ZoneID& s) const;

public:
	void Init();
	void Update();

	// Used when saving the world from disk
	void Save(DataWriter& writer);

	// Used when loading the world from disk
	void Load(DataReader& reader, int version);

	//decltype(m_objectsByID)::const_iterator GetZDOItr(const NetID& id, const Vector3& def);

	// Get a ZDO by id
	//	The ZDO will be created if its ID does not exist
	//	Returns the ZDO and a bool if newly created
	std::pair<ZDO*, bool> GetOrCreateZDO(const NetID& id, const Vector3& def);

	// Get a ZDO by id
	ZDO* GetZDO(const NetID& id);

	// Get all ZDOs strictly within a zone
	void GetZDOs_Zone(const ZoneID& zone, std::list<ZDO*>& out);
	// Get all ZDOs strictly within neighboring zones
	void GetZDOs_NeighborZones(const ZoneID& zone, std::list<ZDO*>& out);
	// Get all ZDOs strictly within distant zones
	void GetZDOs_DistantZones(const ZoneID& zone, std::list<ZDO*>& out);
	// Get all ZDOs strictly within a zone, its neighboring zones, and its distant zones
	void GetZDOs_ActiveZones(const ZoneID& zone, std::list<ZDO*>& out, std::list<ZDO*>& outDistant);

	// Get all ZDOs strictly within a zone that are distant flagged
	void GetZDOs_Distant(const ZoneID& sector, std::list<ZDO*>& objects);

	// Get all ZDOs strictly by prefab
	std::list<ZDO*> GetZDOs_Prefab(HASH_t prefabHash);
	// Get all ZDOs strictly within a radius
	std::list<ZDO*> GetZDOs_Radius(const Vector3& pos, float radius);
	// Gets all ZDOs strictly within a sqradius
	//std::list<ZDO*> GetZDOs_SqRadius(const Vector3& pos, float sqradius);
	
	// Get all ZDOs strictly within a radius by prefab
	std::list<ZDO*> GetZDOs_PrefabRadius(const Vector3& pos, float radius, HASH_t prefabHash);

	ZDO* AnyZDO_PrefabRadius(const Vector3& pos, float radius, HASH_t prefabHash);

	void ForceSendZDO(const NetID& id);
};

IZDOManager* ZDOManager();
