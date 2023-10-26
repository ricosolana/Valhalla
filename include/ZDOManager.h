#pragma once

#include <vector>
#include <functional>
#include <range/v3/all.hpp>

#include "Vector.h"
#include "ZDO.h"
#include "Peer.h"
#include "PrefabManager.h"
#include "ZoneManager.h"

class IZDOManager {
	friend class ZDOCollector;
	friend class INetManager;
	friend class IValhalla;
	friend class ZDO;
	friend class VHTest;

	// these are just here for easy access
	//using ZDOContainer = ZDO::set;
	//using ZDO_map = ZDO::map;
	//using ZDO_iterator = ZDO::ZDO_iterator;

private:
	// Contains ZDOs according to Zone
	//	takes up around 5MB; could be around 72 bytes with map
	std::array<ZDO::ref_container, (IZoneManager::WORLD_RADIUS_IN_ZONES* IZoneManager::WORLD_RADIUS_IN_ZONES * 2 * 2)> m_objectsBySector;

	// Contains ZDOs according to prefab
	//	TODO is this necessary?
	UNORDERED_MAP_t<HASH_t, ZDO::ref_container> m_objectsByPrefab;

	// Responsible for managing ZDOs lifetimes
	//	A segmented map is used instead of a vector map
	ZDO::container m_objectsByID;
	
	// Container of retired ZDOs
	//	TODO benchmark storage here vs keeping ZDOIDs in m_objectsByID map (but setting values to null to diffreenciate between alive/dead)
	ZDO::id_container m_erasedZDOs;

	// Contains recently destroyed ZDOs to be sent
	std::vector<ZDOID> m_destroySendList;

	std::vector<ZDO> m_zdoInsertQueue; // TODO create all zdos before sendZdos in Update to sync
	std::vector<ZDOID> m_zdoEraseQueue; // TODO use zdoid

	BYTES_t m_temp;

	// Increments over the course of the game as ZDOs are created
	uint32_t m_nextUid = 1;

private:
	// Called when an authenticated peer joins (internal)
	void OnNewPeer(Peer& peer);
	// Called when an authenticated peer leaves (internal)
	void OnPeerQuit(Peer& peer);

	
	// Retrieve a zone container for storing zdos
	[[nodiscard]] ZDO::ref_container* _GetZDOContainer(ZoneID zone) {
		int num = SectorToIndex(zone);
		if (num != -1) {
			return &m_objectsBySector[num];
		}
		return nullptr;
	}

	// Insert a ZDO into zone (internal)
	void _AddZDOToZone(ZDO::unsafe_value zdo);
	// Remove a zdo from a zone (internal)
	void _RemoveFromSector(ZDO::unsafe_value zdo);
	// Relay a ZDO zone change to clients (internal)
	void _InvalidateZDOZone(ZDO::unsafe_value zdo);

	void AssignOrReleaseZDOs(Peer& peer);
	//void SmartAssignZDOs();

	// Frees a ZDO from memory by a valid iterator
	[[maybe_unused]] ZDO::container::iterator _EraseZDO(ZDO::container::iterator itr);
	void EraseZDO(ZDOID zdoid) {
		//m_zdoEraseQueue.push_back()

		auto&& find = m_objectsByID.find(zdoid);
		if (find != m_objectsByID.end())
			_EraseZDO(find);
	}

	// Destroys a ZDO globally
	// The ZDO is freed from memory
	// Returns an iterator to the next ZDO
	[[maybe_unused]] ZDO::container::iterator _DestroyZDO(ZDO::container::iterator itr) {
		m_destroySendList.push_back((*itr)->GetID());
		return _EraseZDO(itr);
	}

	void SendAllZDOs(Peer& peer) {
		while (SendZDOs(peer, true));
	}
	[[maybe_unused]] bool SendZDOs(Peer& peer, bool flush);
	[[nodiscard]] std::list<std::pair<ZDO::unsafe_value, float>> CreateSyncList(Peer& peer);



	// Instantiate a ZDO by id if it does not exist
	// Returns the ZDO or the previously mapped ZDO
	[[nodiscard]] std::pair<ZDO::container::iterator, bool> _Instantiate(ZDOID zdoid) noexcept;

	// Instantiate a ZDO by id if it does not exist
	// Returns the ZDO or the previously mapped ZDO
	[[nodiscard]] std::pair<ZDO::container::iterator, bool> _Instantiate(ZDOID zdoid, Vector3f position) noexcept;

	// Instantiate a ZDO with the next available ID
	[[nodiscard]] ZDO::unsafe_value _Instantiate(Vector3f position) noexcept;
	
	// Instantiate a ZDO with the specified id
	// Throws if the ZDO exists
	[[nodiscard]] ZDO::unsafe_value _TryInstantiate(ZDOID uid, Vector3f position);
		
	[[nodiscard]] ZDO::unsafe_value _TryInstantiate(ZDOID zdoid) {
		auto&& insert = _Instantiate(zdoid);

		// if inserted, then set pos
		if (!insert.second) {
			throw std::runtime_error("zdo already exists");
		}

		return ZDO::make_unsafe_value(insert.first);
	}



	// Get a ZDO by id
	// UBF/crash if the ZDO is not found
	//[[nodiscard]] ZDO::unsafe_value _GetZDO(ZDOID id) noexcept {
	//	auto&& find = m_objectsByID.find(id);
	//	if (find != m_objectsByID.end())
	//		return ZDO::ref(find);
	//		//return ZDO(*find);
	//	std::unreachable();
	//}

	// Get a ZDO by id
	// The ZDO will be created if its ID does not exist
	// Returns the ZDO and a bool if newly created
	//[[nodiscard]] std::pair<ZDO_iterator, bool> _GetOrInstantiate(ZDOID id, Vector3f def);

	// Returns
	ZDO::unsafe_optional _GetZDOMatch(ZDOID v) {
		auto&& find = m_objectsByID.find(v);
		if (find != m_objectsByID.end())
			return ZDO::make_unsafe_optional(find);
		return ZDO::unsafe_nullopt;
	}

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

public:
	void Init();
	void Update();

	// Used when saving the world from disk
	void Save(DataWriter& writer);

	// Used when loading the world from disk
	void Load(DataReader& reader, int version);

	[[maybe_unused]] ZDO::unsafe_value Instantiate(const Prefab& prefab, Vector3f pos);
	[[maybe_unused]] ZDO::unsafe_value Instantiate(HASH_t hash, Vector3f pos, const Prefab** outPrefab);

	[[maybe_unused]] ZDO::unsafe_value Instantiate(HASH_t hash, Vector3f pos) {
		return Instantiate(hash, pos, nullptr);
	}
	// TODO either correctly implement or?
	//	intended to instantiate an object based on another
	//[[maybe_unused]] ZDO Instantiate(const ZDO& zdo);

	// Get a ZDO by id
	//	TODO use optional<reference>
	[[nodiscard]] ZDO::unsafe_optional GetZDO(ZDOID id);

	// Get all ZDOs strictly within a zone
	void GetZDOs_Zone(ZoneID zone, std::list<ZDO::unsafe_value>& out);
	// Get all ZDOs strictly within neighboring zones
	void GetZDOs_NeighborZones(ZoneID zone, std::list<ZDO::unsafe_value>& out);
	// Get all ZDOs strictly within distant zones
	void GetZDOs_DistantZones(ZoneID zone, std::list<ZDO::unsafe_value>& out);
	// Get all ZDOs strictly within a zone, its neighboring zones, and its distant zones
	void GetZDOs_ActiveZones(ZoneID zone, std::list<ZDO::unsafe_value>& out, std::list<ZDO::unsafe_value>& outDistant);
	// Get all ZDOs strictly within a zone that are distant flagged
	void GetZDOs_Distant(ZoneID sector, std::list<ZDO::unsafe_value>& objects);
	



	void ForceSendZDO(ZDOID id);

	// Erases a ZDO on clients and server
	//	Warning: the target ZDO is freed from memory and will become no longer accessible
	void DestroyZDO(ZDOID zdoid) {
		m_destroySendList.push_back(zdoid);
		EraseZDO(zdoid);
	}
	void DestroyZDO(ZDO::unsafe_value zdo) {
		DestroyZDO(zdo->GetID());
	}



	[[nodiscard]] size_t GetSumZDOMembers();
	[[nodiscard]] float GetMeanZDOMembers();
	[[nodiscard]] float GetStDevZDOMembers();
	[[nodiscard]] size_t GetTotalZDOAlloc();

	[[nodiscard]] size_t GetCountEmptyZDOs();
};

// Manager class for everything related to networked object synchronization
IZDOManager* ZDOManager();
