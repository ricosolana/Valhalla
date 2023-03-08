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
	robin_hood::unordered_map<ZDOID, std::unique_ptr<ZDO>> m_objectsByID;

	// Contains ZDOs according to Zone
	std::array<robin_hood::unordered_set<ZDO*>, 
		(IZoneManager::WORLD_RADIUS_IN_ZONES * IZoneManager::WORLD_RADIUS_IN_ZONES * 2 * 2)> m_objectsBySector; // takes up around 5MB; could be around 72 bytes with map

	// Contains ZDOs according to prefab
	robin_hood::unordered_map<HASH_t, robin_hood::unordered_set<ZDO*>> m_objectsByPrefab;

	// Primarily used in RPC_ZDOData
	//robin_hood::unordered_map<ZDOID, TICKS_t> m_deadZDOs;
	robin_hood::unordered_set<ZDOID> m_deadZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<ZDOID> m_destroySendList;

	//robin_hood::unordered_map<ZDOID, TICKS_t> m_terrainRevisions;

private:
	// Called when an authenticated peer joins (internal)
	void OnNewPeer(Peer& peer);
	// Called when an authenticated peer leaves (internal)
	void OnPeerQuit(Peer& peer);

	// Insert a ZDO into zone (internal)
	bool AddToSector(ZDO& zdo);
	// Remove a zdo from a zone (internal)
	void RemoveFromSector(ZDO& zdo);
	// Relay a ZDO sector change to clients (internal)
	void InvalidateSector(ZDO& zdo);

	void AssignOrReleaseZDOs(Peer& peer);
	//void SmartAssignZDOs();

	void EraseZDO(const ZDOID& uid);
	void SendAllZDOs(Peer& peer);
	bool SendZDOs(Peer& peer, bool flush);
	std::list<std::reference_wrapper<ZDO>> CreateSyncList(Peer& peer);

	ZDO& AddZDO(const Vector3& position);
	ZDO& AddZDO(const ZDOID& uid, const Vector3& position);
		
	// Performs a coordinate to pitch conversion
	int SectorToIndex(const ZoneID& zone) const;

public:
	void Init();
	void Update();

	// Used when saving the world from disk
	void Save(DataWriter& writer);

	// Used when loading the world from disk
	void Load(DataReader& reader, int version);

	// Get a ZDO by id
	//	The ZDO will be created if its ID does not exist
	//	Returns the ZDO and a bool if newly created
	std::pair<ZDO&, bool> GetOrCreateZDO(const ZDOID& id, const Vector3& def);

	// Get a ZDO by id
	//	Returns null if ZDO not found
	ZDO* GetZDO(const ZDOID& id);

	// Get all ZDOs strictly within a zone
	void GetZDOs_Zone(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within neighboring zones
	void GetZDOs_NeighborZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within distant zones
	void GetZDOs_DistantZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out);
	// Get all ZDOs strictly within a zone, its neighboring zones, and its distant zones
	void GetZDOs_ActiveZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out, std::list<std::reference_wrapper<ZDO>>& outDistant);
	// Get all ZDOs strictly within a zone that are distant flagged
	void GetZDOs_Distant(const ZoneID& sector, std::list<std::reference_wrapper<ZDO>>& objects);
		


	// Get a capped number of ZDOs within a radius matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const Vector3& pos, float radius, size_t max, const std::function<bool(const ZDO&)>& pred);
	// Get a capped number of ZDOs within a radius
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const Vector3& pos, float radius, size_t max) {
		return SomeZDOs(pos, radius, max, nullptr);
	}
	// Get a capped number of ZDOs with prefab and/or flag
	//	*Note: Prefab or Flag must be non-zero for anything to be returned
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const Vector3& pos, float radius, size_t max, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(pos, radius, max, [&](const ZDO& zdo) {
			auto&& prefab = zdo.GetPrefab();
			return prefab->FlagsAbsent(flagsAbsent) &&
				(prefab->m_hash == prefabHash 
					|| (std::to_underlying(flagsPresent) && prefab->FlagsPresent(flagsPresent)));
		});
	}


	// Get a capped number of ZDOs within a zone matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const ZoneID& zone, size_t max, const std::function<bool(const ZDO&)>& pred);
	// Get a capped number of ZDOs within a zone
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const ZoneID& zone, size_t max) {
		return SomeZDOs(zone, max, nullptr);
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const ZoneID& zone, size_t max, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, max, [&](const ZDO& zdo) {
			auto&& prefab = zdo.GetPrefab();
			return prefab->FlagsAbsent(flagsAbsent) &&
				(prefab->m_hash == prefabHash
					|| (std::to_underlying(flagsPresent) && prefab->FlagsPresent(flagsPresent)));
		});
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const ZoneID& zone, size_t max, const Vector3& pos, float radius) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, max, [&](const ZDO& zdo) {
			return zdo.Position().SqDistance(pos) <= sqRadius;
		});
	}
	// Get a capped number of ZDOs within a radius in zone with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> SomeZDOs(const ZoneID& zone, size_t max, const Vector3& pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, max, [&](const ZDO& zdo) {
			if (zdo.Position().SqDistance(pos) <= sqRadius) {
				auto&& prefab = zdo.GetPrefab();
				return prefab->FlagsAbsent(flagsAbsent) &&
					(prefab->m_hash == prefabHash
						|| (std::to_underlying(flagsPresent) && prefab->FlagsPresent(flagsPresent)));
			}
			return false;
		});
	}


	// Get all ZDOs with prefab
	std::list<std::reference_wrapper<ZDO>> GetZDOs(HASH_t prefab);


	// Get all ZDOs within a radius matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const Vector3& pos, float radius, const std::function<bool(const ZDO&)>& pred) {
		return SomeZDOs(pos, radius, -1, pred);
	}
	// Get all ZDOs within a radius
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const Vector3& pos, float radius) {
		return SomeZDOs(pos, radius, -1, nullptr);
	}
	// Get all ZDOs within a radius with prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const Vector3& pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(pos, radius, -1, prefabHash, flagsPresent, flagsAbsent);
	}


	// Get all ZDOs within a zone matching an optional predicate
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const ZoneID& zone, const std::function<bool(const ZDO&)>& pred) {
		return SomeZDOs(zone, -1, pred);
	}
	// Get all ZDOs within a zone
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const ZoneID& zone) {
		return SomeZDOs(zone, -1, nullptr);
	}
	// Get all ZDOs within a zone of prefab and/or flag
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const ZoneID& zone, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, -1, [&](const ZDO& zdo) {
			auto&& prefab = zdo.GetPrefab();
			return prefab->FlagsAbsent(flagsAbsent) &&
				(prefab->m_hash == prefabHash
					|| (std::to_underlying(flagsPresent) && prefab->FlagsPresent(flagsPresent)));
		});
	}
	// Get all ZDOs within a radius in zone
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const ZoneID& zone, const Vector3& pos, float radius) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, -1, [&](const ZDO& zdo) {
			return zdo.Position().SqDistance(pos) <= sqRadius;
		});
	}
	// Get all ZDOs within a radius in zone
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const ZoneID& zone, const Vector3& pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, -1, [&](const ZDO& zdo) {
			if (zdo.Position().SqDistance(pos) <= sqRadius) {
				auto&& prefab = zdo.GetPrefab();
				return prefab->FlagsAbsent(flagsAbsent) &&
					(prefab->m_hash == prefabHash
						|| (std::to_underlying(flagsPresent) && prefab->FlagsPresent(flagsPresent)));
			}
			return false;
		});
	}

	/*
	std::list<std::reference_wrapper<ZDO>> GetZDOs(const ZoneID& zone, float minHeight, float maxHeight) {
		return GetZDOs(zone, [=](const ZDO& zdo) {
			auto&& h = zdo.Position().y;
			return h >= minHeight && h <= maxHeight;
		});
	}

	std::list<std::reference_wrapper<ZDO>> GetZDOs(const ZoneID& zone, float minHeight, float maxHeight, Prefab::Flag flag) {
		return GetZDOs(zone, [=](const ZDO& zdo) {
			if (!zdo.GetPrefab()->FlagsPresent(flag))
				return false;

			auto&& h = zdo.Position().y;
			return h >= minHeight && h <= maxHeight;
		});
	}*/


	// Get any ZDO within a radius with prefab and/or flag
	ZDO* AnyZDO(const Vector3& pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(pos, radius, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return nullptr;
		return &zdos.front().get();
	}
	// Get any ZDO within a zone with prefab and/or flag
	ZDO* AnyZDO(const ZoneID& zone, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(zone, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return nullptr;
		return &zdos.front().get();
	}


	// Get the nearest ZDO within a radius matching an optional predicate
	ZDO* NearestZDO(const Vector3& pos, float radius, const std::function<bool(const ZDO&)>& pred);
	// Get the nearest ZDO within a radius with prefab and/or flag
	ZDO* NearestZDO(const Vector3& pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return NearestZDO(pos, radius, [&](const ZDO& zdo) {
			auto&& prefab = zdo.GetPrefab();
			return prefab->FlagsAbsent(flagsAbsent) &&
				(prefab->m_hash == prefabHash
					|| (std::to_underlying(flagsPresent) && prefab->FlagsPresent(flagsPresent)));
		});
	}



	void ForceSendZDO(const ZDOID& id);
	void DestroyZDO(ZDO& zdo, bool immediate);

	// Destroy a ZDO with default client erase behaviour
	//	ZDO is still kept around on the server
	void DestroyZDO(ZDO& zdo) {
		this->DestroyZDO(zdo, false);
	}
};

// Manager class for everything related to networked object synchronization
IZDOManager* ZDOManager();
