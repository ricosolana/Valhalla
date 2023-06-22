#pragma once

#include <vector>
#include <functional>

#include "Vector.h"
#include "ZDO.h"
#include "Peer.h"
#include "PrefabManager.h"
#include "ZoneManager.h"

class IZDOManager {
	friend class INetManager;
	friend class IValhalla;
	friend class ZDO;
	friend class VHTest;

#if VH_IS_ON(VH_STANDARD_PREFABS)
	// Predicate for whether a zdo is a prefab with or without given flags
	//	prefabHash: if 0, then prefabHash check is skipped
	static bool PREFAB_CHECK_FUNCTION(std::reference_wrapper<const ZDO> zdo, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& prefab = zdo.get()->GetPrefab();

		return prefab.AllFlagsAbsent(flagsAbsent)
			&& (prefabHash == 0 || prefab.m_hash == prefabHash)
			&& prefab.AllFlagsPresent(flagsPresent);
	}
#endif // VH_STANDARD_PREFABS

	using ZDOContainer = ankerl::unordered_dense::segmented_set<ZDOBase*>;
	using ZDO_map = ankerl::unordered_dense::segmented_map<ZDOID, std::unique_ptr<ZDOBase>>;
	using ZDO_iterator = ZDO_map::iterator;

private:
	// Contains ZDOs according to Zone
	//	takes up around 5MB; could be around 72 bytes with map
	//	TODO use map for esp32 (and lower initial memory usage)
	//	esp only has 5Mb of ram, and this exceeds that
#if VH_IS_ON(VH_PLATFORM_ESP32)
	std::unordered_map<ZoneID, ZDOContainer> m_objectsBySector;
#else // !VH_PLATFORM_ESP32
	std::array<ZDOContainer, (IZoneManager::WORLD_RADIUS_IN_ZONES* IZoneManager::WORLD_RADIUS_IN_ZONES * 2 * 2)> m_objectsBySector;
#endif // VH_PLATFORM_ESP32

#if VH_IS_ON(VH_STANDARD_PREFABS)
	// Contains ZDOs according to prefab
	// TODO this specifically exists for the Lua API, remove when MODS_DISABLED and for esp32
	UNORDERED_MAP_t<HASH_t, ankerl::unordered_dense::segmented_set<ZDO*>> m_objectsByPrefab;
#endif // VH_STANDARD_PREFABS

	// Responsible for managing ZDOs lifetimes
	//	A segmented map is used instead of a 
	ZDO_map m_objectsByID;
	
	// Container of retired ZDOs
	//	TODO benchmark storage here vs keeping ZDOIDs in m_objectsByID map (but setting values to null to diffreenciate between alive/dead)
	ankerl::unordered_dense::segmented_set<ZDOID> m_erasedZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<ZDOID> m_destroySendList;

	BYTES_t m_temp;

	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

	//static const std::function<bool(const ZDO&, HASH_t, Prefab::FLAG_t, Prefab::FLAG_t)> PREFAB_FUNCTION;

private:
	// Called when an authenticated peer joins (internal)
	void OnNewPeer(Peer& peer);
	// Called when an authenticated peer leaves (internal)
	void OnPeerQuit(Peer& peer);

	
	// Retrieve a zone container for storing zdos
	//	if createIfNotExists, container will be created if it does not exist
	//decltype(m_objectsBySector)::iterator _GetZDOZoneContainer(ZoneID zone, bool createIfNotExists);
	template<bool createIfNotExists = false>
	[[nodiscard]] ZDOContainer* _GetZDOContainer(ZoneID zone) {
#if VH_IS_ON(VH_PLATFORM_ESP32)
		if constexpr (createIfNotExists) {
			return m_objectsBySector[zone];
		}
		else {
			auto&& find = m_objectsBySector.find(zone);
			if (find != m_objectsBySector.end()) {
				return find->second;
			}
		}
#else
		int num = SectorToIndex(zone);
		if (num != -1) {
			return &m_objectsBySector[num];
		}
#endif
		return nullptr;
	}

	// Insert a ZDO into zone (internal)
	void _AddZDOToZone(ZDO& zdo);
	// Remove a zdo from a zone (internal)
	void _RemoveFromSector(ZDO& zdo);
	// Relay a ZDO sector change to clients (internal)
	void _InvalidateZDOZone(ZDO& zdo);

	void AssignOrReleaseZDOs(Peer& peer);
	//void SmartAssignZDOs();

	// Frees a ZDO from memory by a valid iterator
	[[maybe_unused]] ZDO_iterator _EraseZDO(ZDO_iterator itr);
	void EraseZDO(ZDOID zdoid) {
		auto&& find = m_objectsByID.find(zdoid);
		if (find != m_objectsByID.end())
			_EraseZDO(find);
	}

	// Destroys a ZDO globally
	//	The ZDO is freed from memory
	//	Returns an iterator to the next ZDO
	[[maybe_unused]] ZDO_iterator _DestroyZDO(ZDO_iterator itr) {
		m_destroySendList.push_back(itr->first);
		return _EraseZDO(itr);
	}

	void SendAllZDOs(Peer& peer) {
		while (SendZDOs(peer, true));
	}
	[[maybe_unused]] bool SendZDOs(Peer& peer, bool flush);
	[[nodiscard]] std::list<std::pair<ZDO, float>> CreateSyncList(Peer& peer);

	[[nodiscard]] ZDO _Instantiate(Vector3f position);
	[[nodiscard]] ZDO _Instantiate(ZDOID uid, Vector3f position);
		
	// Get a ZDO by id
	//	The ZDO will be created if its ID does not exist
	//	Returns the ZDO and a bool if newly created
	[[nodiscard]] std::pair<ZDO_iterator, bool> _GetOrInstantiate(ZDOID id, Vector3f def);

#if !VH_IS_ON(VH_PLATFORM_ESP32)
	// Performs a coordinate to pitch conversion
	[[nodiscard]] int SectorToIndex(ZoneID zone) const {
		if (zone.x * zone.x + zone.y * zone.y >= IZoneManager::WORLD_RADIUS_IN_ZONES * IZoneManager::WORLD_RADIUS_IN_ZONES)
			return -1;

		int x = zone.x + IZoneManager::WORLD_RADIUS_IN_ZONES;
		int y = zone.y + IZoneManager::WORLD_RADIUS_IN_ZONES;
		if (x < 0 || y < 0
			|| x >= IZoneManager::WORLD_DIAMETER_IN_ZONES || y >= IZoneManager::WORLD_DIAMETER_IN_ZONES) {
			return -1;
		}

		assert(x >= 0 && y >= 0 && x < IZoneManager::WORLD_DIAMETER_IN_ZONES && y < IZoneManager::WORLD_DIAMETER_IN_ZONES && "sector exceeds world radius");

		return y * IZoneManager::WORLD_DIAMETER_IN_ZONES + x;
}
#endif

public:
	void Init();
	void Update();

	// Used when saving the world from disk
	void Save(DataWriter& writer);

	// Used when loading the world from disk
	void Load(DataReader& reader, int version);

#if VH_IS_ON(VH_STANDARD_PREFABS)
	[[maybe_discard]] ZDO Instantiate(const Prefab& prefab, Vector3f pos);
	[[maybe_discard]] ZDO Instantiate(HASH_t hash, Vector3f pos, const Prefab** outPrefab);
#endif // VH_STANDARD_PREFABS
	[[maybe_discard]] ZDO Instantiate(HASH_t hash, Vector3f pos) {
#if VH_IS_ON(VH_STANDARD_PREFABS)
		return Instantiate(hash, pos, nullptr);
#else // !VH_STANDARD_PREFABS
		auto&& zdo = Instantiate(pos);
		zdo.get()._SetPrefabHash(hash);
		return zdo;
#endif // VH_STANDARD_PREFABS
	}
	[[maybe_discard]] ZDO Instantiate(const ZDO& zdo);

	// Get a ZDO by id
	//	TODO use optional<reference>
	[[nodiscard]] ZDO* GetZDO(ZDOID id);

	// Get all ZDOs strictly within a zone
	void GetZDOs_Zone(ZoneID zone, std::list<ZDO>& out);
	// Get all ZDOs strictly within neighboring zones
	void GetZDOs_NeighborZones(ZoneID zone, std::list<ZDO>& out);
	// Get all ZDOs strictly within distant zones
	void GetZDOs_DistantZones(ZoneID zone, std::list<ZDO>& out);
	// Get all ZDOs strictly within a zone, its neighboring zones, and its distant zones
	void GetZDOs_ActiveZones(ZoneID zone, std::list<ZDO>& out, std::list<ZDO>& outDistant);
	// Get all ZDOs strictly within a zone that are distant flagged
	void GetZDOs_Distant(ZoneID sector, std::list<ZDO>& objects);
	

	// Get a capped number of ZDOs within a radius matching an optional predicate
	[[nodiscard]] std::list<ZDO> SomeZDOs(Vector3f pos, float radius, size_t max, const std::function<bool(const ZDO&)>& pred);
	// Get a capped number of ZDOs within a radius
	[[nodiscard]] std::list<ZDO> SomeZDOs(Vector3f pos, float radius, size_t max) {
		return SomeZDOs(pos, radius, max, nullptr);
	}
#if VH_IS_ON(VH_STANDARD_PREFABS)
	// Get a capped number of ZDOs with prefab and/or flag
	//	*Note: Prefab or Flag must be non-zero for anything to be returned
	[[nodiscard]] std::list<ZDO> SomeZDOs(Vector3f pos, float radius, size_t max, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(pos, radius, max, 
			[&](const ZDO& zdo) {
				return PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
			}
		);
	}
#endif

	// Get a capped number of ZDOs within a zone matching an optional predicate
	[[nodiscard]] std::list<ZDO> SomeZDOs(ZoneID zone, size_t max, const std::function<bool(const ZDO&)>& pred);
	// Get a capped number of ZDOs within a zone
	[[nodiscard]] std::list<ZDO> SomeZDOs(ZoneID zone, size_t max) {
		return SomeZDOs(zone, max, nullptr);
	}
#if VH_IS_ON(VH_STANDARD_PREFABS)
	// Get a capped number of ZDOs within a radius in zone with prefab and/or flag
	[[nodiscard]] std::list<ZDO> SomeZDOs(ZoneID zone, size_t max, Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, max, [&](const ZDO& zdo) {
			return zdo->Position().SqDistance(pos) <= sqRadius
				&& PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
		});
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	[[nodiscard]] std::list<ZDO> SomeZDOs(ZoneID zone, size_t max, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, max, Vector3f::Zero(), std::numeric_limits<float>::max(), prefab, flagsPresent, flagsAbsent);
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	[[nodiscard]] std::list<ZDO> SomeZDOs(ZoneID zone, size_t max, Vector3f pos, float radius) {
		return SomeZDOs(zone, max, pos, radius, 0, Prefab::Flag::NONE, Prefab::Flag::NONE);
	}


	// Get all ZDOs with prefab
	//	This method is optimized assuming VH_STANDARD_PREFABS is on
	[[nodiscard]] std::list<ZDO> GetZDOs(HASH_t prefab);
	
	// Get all ZDOs fulfilling a given predicate
	//	Try to avoid using this method too frequently (it iterates all ZDOs in the world, which is *very* slow)
	[[nodiscard]] std::list<ZDO> GetZDOs(const std::function<bool(const ZDO&)>& pred);

	// Get all ZDOs matching the given prefab flags
	//	Try to avoid using this method too frequently (it iterates all ZDOs in the world, which is *very* slow)
	[[nodiscard]] std::list<ZDO> GetZDOs(Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return GetZDOs([&](const ZDO& zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, 0, flagsPresent, flagsAbsent);
		});
	}
#endif

	// Get all ZDOs within a radius matching an optional predicate
	[[nodiscard]] std::list<ZDO> GetZDOs(Vector3f pos, float radius, const std::function<bool(const ZDO&)>& pred) {
		return SomeZDOs(pos, radius, -1, pred);
	}
	// Get all ZDOs within a radius
	[[nodiscard]] std::list<ZDO> GetZDOs(Vector3f pos, float radius) {
		return SomeZDOs(pos, radius, -1, nullptr);
	}

#if VH_IS_ON(VH_STANDARD_PREFABS)
	// Get all ZDOs within a radius with prefab and/or flag
	[[nodiscard]] std::list<ZDO> GetZDOs(Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(pos, radius, -1, prefab, flagsPresent, flagsAbsent);
	}
#endif


	// Get all ZDOs within a zone matching an optional predicate
	[[nodiscard]] std::list<ZDO> GetZDOs(ZoneID zone, const std::function<bool(const ZDO&)>& pred) {
		return SomeZDOs(zone, -1, pred);
	}
	// Get all ZDOs within a zone
	[[nodiscard]] std::list<ZDO> GetZDOs(ZoneID zone) {
		return SomeZDOs(zone, -1, nullptr);
	}
#if VH_IS_ON(VH_STANDARD_PREFABS)
	// Get all ZDOs within a zone of prefab and/or flag
	[[nodiscard]] std::list<ZDO> GetZDOs(ZoneID zone, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, -1, [&](const ZDO& zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
		});
	}
	// Get all ZDOs within a radius in zone
	[[nodiscard]] std::list<ZDO> GetZDOs(ZoneID zone, Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, -1, [&](const ZDO& zdo) {
			return zdo->Position().SqDistance(pos) <= sqRadius
				&& PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
		});
	}
	// Get all ZDOs within a radius in zone
	[[nodiscard]] std::list<ZDO> GetZDOs(ZoneID zone, Vector3f pos, float radius) {
		return GetZDOs(zone, pos, radius, 0, Prefab::Flag::NONE, Prefab::Flag::NONE);
	}


	// Get any ZDO within a radius with prefab and/or flag
	[[nodiscard]] ZDO AnyZDO(Vector3f pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(pos, radius, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return nullptr;
		return &zdos.front().get();
	}
	// Get any ZDO within a zone with prefab and/or flag
	[[nodiscard]] ZDO AnyZDO(ZoneID zone, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(zone, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return nullptr;
		return &zdos.front().get();
	}
#endif


	// Get the nearest ZDO within a radius matching an optional predicate
	[[nodiscard]] ZDO* NearestZDO(Vector3f pos, float radius, const std::function<bool(const ZDO&)>& pred);
#if VH_IS_ON(VH_STANDARD_PREFABS)
	// Get the nearest ZDO within a radius with prefab and/or flag
	[[nodiscard]] ZDO* NearestZDO(Vector3f pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return NearestZDO(pos, radius, [&](const ZDO& zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, prefabHash, flagsPresent, flagsAbsent);
		});
	}
#endif



	void ForceSendZDO(ZDOID id);

	// Erases a ZDO on clients and server
	//	Warning: the target ZDO is freed from memory and will become no longer accessible
	void DestroyZDO(ZDOID zdoid) {
		m_destroySendList.push_back(zdoid);
		EraseZDO(zdoid);
	}
	void DestroyZDO(const ZDO& zdo) {
		DestroyZDO(zdo.ID());
	}



	[[nodiscard]] size_t GetSumZDOMembers();
	[[nodiscard]] float GetMeanZDOMembers();
	[[nodiscard]] float GetStDevZDOMembers();
	[[nodiscard]] size_t GetTotalZDOAlloc();

	[[nodiscard]] size_t GetCountEmptyZDOs();
};

// Manager class for everything related to networked object synchronization
IZDOManager* ZDOManager();
