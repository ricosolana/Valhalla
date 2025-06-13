
#include <cassert>
#include <quill/LogMacros.h>
#include <stdexcept>

#include <range/v3/all.hpp>
#include <utility>

#include "ZDOManager.h"
#include "NetManager.h"
#include "VUtils.h"
#include "ValhallaServer.h"
#include "Hashes.h"
#include "ZoneManager.h"
#include "RouteManager.h"

auto ZDO_MANAGER = std::make_unique<IZDOManager>();
IZDOManager* ZDOManager() {
	return ZDO_MANAGER.get();
}



void IZDOManager::Init() {
	m_nextUid = 1;

	LOG_INFO(VH_LOGGER, "Initializing ZDOManager");

	RouteManager()->Register(avledet::util::hashes::Routed::DestroyZDO, 
		[this](Peer*, DataReader reader) {
			// TODO constraint check
			reader.read([this](ZDOID zdoid) {
				EraseZDO(zdoid);
			});
		}
	);

	//auto&& insert = ZDOManager()->m_objectsByID.begin()->second->
		//m_members.insert({0, ZDO::Ord()});
	//insert.first->second.Get
	RouteManager()->Register(avledet::util::hashes::Routed::C2S_RequestZDO, 
		[this](Peer* peer, ZDOID id) {
			peer->ForceSendZDO(id);
		}
	);
}

void IZDOManager::Update() {
	ZoneScoped;

	if (VUtils::run_periodic_now<struct periodic_zdo_stats>(3min)) {
		LOG_INFO(VH_LOGGER, "Currently {} zdos (~{:0.02f}MB -> ~{:0.02f}MB)", 
			m_objectsByID.size(), 
			//(this->GetTotalZDOAlloc() / 1000000.f),
			ZDO::GetTotalAlloc(false) / 1000000.f,
			ZDO::GetTotalAlloc(true) / 1000000.f
		);
	}

	//assert(std::accumulate(m_objectsByPrefab.begin(), m_objectsByPrefab.end(), (std::size_t)0,
	//	[](std::size_t value, const decltype(m_objectsByPrefab)::value_type& v) -> std::size_t {
	//		return value + v.second.size();
	//	}
	//) == m_objectsByID.size());

	// TODO requires testing
	//	link portals if mode enabled
#if VH_IS_ON(VH_PORTAL_LINKING)
	if (VUtils::run_periodic<struct link_portals>(1s)) {
		auto&& portals = GetZDOs(avledet::util::hashes::Object::portal_wood);

		// TODO use the optimized Lua ported code for linking portals
		//	not the exact code but the way the algo works

		auto&& FindRandomUnconnectedPortal = [&](ZDOID skip, std::string_view tag) -> ZDO::unsafe_optional {
			std::vector<ZDO::unsafe_value> list;
			for (auto&& zdo : portals) {
				if (zdo->GetID() != skip
					&& zdo->GetString(avledet::util::hashes::ZDO::TeleportWorld::TAG) == tag
					&& !zdo->GetConnectionZDOID(ZDOConnector::Type::Portal))
				{
					list.push_back(zdo);
				}
			}

			if (list.empty()) {
				return ZDO::unsafe_nullopt;
			}

			return ZDO::make_unsafe_optional(list[VUtils::Random::State().range(0, list.size())]);
		};

		for (auto&& zdo : portals) {
			auto&& connectionZDOID = zdo->GetConnectionZDOID(ZDOConnector::Type::Portal);
			auto&& string = zdo->GetString(avledet::util::hashes::ZDO::TeleportWorld::TAG);
			if (connectionZDOID) {
				auto&& zdo2 = GetZDO(connectionZDOID);
				if (!zdo2 || zdo2->GetString(avledet::util::hashes::ZDO::TeleportWorld::TAG) != string)
				{
					zdo->SetLocal();
					zdo->SetConnection(ZDOConnector::Type::Portal, ZDOID::NONE);
					ForceSendZDO(zdo->GetID());
				}
			}
		}
		
		for (auto&& zdo3 : portals) {
			if (!zdo3->GetConnectionZDOID(ZDOConnector::Type::Portal)) {
				auto&& string2 = zdo3->GetString(avledet::util::hashes::ZDO::TeleportWorld::TAG);
				auto&& zdo4 = FindRandomUnconnectedPortal(zdo3->GetID(), string2);
				if (zdo4) {
					zdo3->SetLocal();
					zdo4->SetLocal();
					zdo3->SetConnection(ZDOConnector::Type::Portal, zdo4->GetID());
					zdo4->SetConnection(ZDOConnector::Type::Portal, zdo3->GetID());
					ForceSendZDO(zdo3->GetID());
					ForceSendZDO(zdo4->GetID());
					//zdo3.Apply();
				}
			}
		}
	}
#endif



	auto&& peers = NetManager()->GetPeers();
	
	if (VUtils::run_periodic<struct zdos_release_assign>(VH_SETTINGS.zdoAssignInterval)) {
		for (auto&& peer : peers) {
			if (!peer->IsGated()) 
			{
				AssignOrReleaseZDOs(*peer);
			}
		}
	}

	if (VUtils::run_periodic<struct periodic_send_zdos>(VH_SETTINGS.zdoSendInterval)) {
		for (auto&& peer : peers) {
			SendZDOs(*peer, false);
		}
	}	

	if (!m_destroySendList.empty()) {

		// TODO make a member variable?
		//	think about emulated zdo containers (like replaying actions to specific peers)
		//	this is a functionality I might be planning on into the future
		DataWriter writer; //.write(m_destroySendList);
		writer.write(m_destroySendList);

		m_destroySendList.clear();

		RouteManager()->InvokeAll(avledet::util::hashes::Routed::DestroyZDO, std::move(writer.get_buf()));
	}
}



void IZDOManager::_AddZDOToZone(ZDO::unsafe_value zdo) {
	auto&& container = _GetZDOContainer(zdo->GetZone());

	assert(!container.get().contains(zdo));

	auto&& insert = container.get().insert(zdo);

	assert(insert.second); //ensure newly inserted

	//LOG_INFO(VH_LOGGER, "zdo added to zone: {} {}", zdo->GetID(), zdo->GetZone());
}

void IZDOManager::_RemoveFromSector(ZDO::unsafe_value zdo) {
	if (auto&& container = _FindZDOContainer(zdo->GetZone())) {
		auto&& erase = container->erase(zdo);

		// ensure zdo was actually erased
		assert(erase);

		//LOG_INFO(VH_LOGGER, "zdo removed from zone: {} {}", zdo->GetID(), zdo->GetZone());
	} else {
		//TODO otherwise, then poll the sector map
		assert(false);
	}
}

void IZDOManager::_InvalidateZDOZone(ZDO::unsafe_value zdo) {
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->ZDOSectorInvalidated(zdo);
	}
}



void IZDOManager::Save(DataWriter& writer) {
	//pkg.write(Valhalla()->ID());
	writer.write((std::int64_t)0);
	writer.write(m_nextUid);
	
	{
		// Write zdos (persistent)
		const auto start = writer.get_pos();

		std::int32_t count = 0;
		writer.write(count);

		for (auto&& zdo : m_objectsByID) {
			if (zdo->IsPersistent()) {
				zdo->Pack(writer, false);
				count++;
			}
		}

		const auto end = writer.get_pos();
		writer.set_pos(start);
		writer.write(count);
		writer.set_pos(end);
	}
}



void IZDOManager::Load(DataReader& reader, int version) {
	reader.read<std::int64_t>(); // server id
	reader.read<std::uint32_t>(); // next uid
	
	auto count = reader.read<std::uint32_t>();
	for (decltype(count) i = 0; i < count; i++) {
		auto&& insert = _Instantiate(
			version < 31 ? reader.read<ZDOID>() : ZDOID(0, ZDOManager()->m_nextUid++)
		);
		
		//auto&& zdo = ZDO(*insert.first);
		//auto&& zdo = insert
		auto&& zdo = ZDO::make_unsafe_value(insert.first);

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
		if (version < 31) {
			auto zdoReader = DataReader(reader.read<std::vector<char>>());

			zdo->Load31Pre(zdoReader, version);
		}
		else 
#endif // VH_LEGACY_WORLD_LOADING
		{
			zdo->Unpack(reader, version);
		}

		_AddZDOToZone(zdo);

		//auto&& prefab = zdo.GetPrefab();

		m_objectsByPrefab[zdo->GetPrefabHash()].insert(zdo);

		//assert(std::accumulate(m_objectsByPrefab.begin(), m_objectsByPrefab.end(), (std::size_t)0,
		//	[](std::size_t value, const decltype(m_objectsByPrefab)::value_type& v) -> std::size_t {
		//		return value + v.second.size();
		//	}
		//) == m_objectsByID.size());

#if VH_IS_ON(VH_DUNGEON_REGENERATION)
		if (prefab.AllFlagsPresent(Prefab::Flag::DUNGEON)) {
			// Only add real sky dungeon
			if (zdo->GetPosition().y > 4000)
				DungeonManager()->m_dungeonInstances.push_back(zdo->GetID());
		}
#endif // VH_DUNGEON_REGENERATION
		//m_objectsByID[zdo->GetID()] = std::move(zdo);
	}

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
	if (version < 31) {
		auto deadCount = reader.read<std::int32_t>();
		for (decltype(deadCount) j = 0; j < deadCount; j++) {
			reader.read<std::int64_t>();
			reader.read<std::uint32_t>();
			reader.read<std::int64_t>();
		}

		// Owners, Terrains, and Seeds have already been converted

		// convert portals
		for (auto&& zdo : GetZDOs(avledet::util::hashes::Object::portal_wood)) {			
			auto&& string = zdo->GetString(avledet::util::hashes::ZDO::TeleportWorld::TAG);
			ZDOID zdoid; zdo->Extract("target", zdoid);
			if (zdoid && !string.empty()) {
				auto&& zdo2 = GetZDO(zdoid);
				if (zdo2) {
					auto&& string2 = zdo2->GetString(avledet::util::hashes::ZDO::TeleportWorld::TAG);
					ZDOID zdoid2; zdo2->Extract("target", zdoid2);
					if (string == string2
						&& zdoid == zdo2->GetID()
						&& zdoid2 == zdo->GetID()) 
					{
						zdo->SetLocal();
						zdo2->SetLocal();
						zdo->SetConnection(ZDOConnector::Type::Portal, zdo2->GetID());
						zdo2->SetConnection(ZDOConnector::Type::Portal, zdo->GetID());
						//zdo.Apply();
					}
				}
			}
		}
		
		// convert spawners
		for (auto&& zdo : GetZDOs(Prefab::Flag::CREATURE_SPAWNER, Prefab::Flag::NONE)) {
			zdo->SetLocal();
			ZDOID zdoid; zdo->Extract("spawn_id", zdoid);
			auto&& zdo2 = GetZDO(zdoid);
			zdo->SetConnection(ZDOConnector::Type::Spawned, zdo2 ? zdo2->GetID() : ZDOID::NONE);
			//zdo.Apply();
		}

		// convert sync transforms
		for (auto&& zdo : GetZDOs(Prefab::Flag::SYNCED_TRANSFORM, Prefab::Flag::NONE)) {
			zdo->SetLocal();
			ZDOID zdoid; zdo->Extract("parentID", zdoid);
			auto&& zdo2 = GetZDO(zdoid);
			if (zdo2) {
				zdo->SetConnection(ZDOConnector::Type::Spawned, zdo2->GetID());
				//zdo.Apply();
			}
			else {
				//zdo.m_pack.Set<ZDO::FLAGS_PACK_INDEX>(
					// zero out connector bit
					//zdo.m_pack.Get<ZDO::FLAGS_PACK_INDEX>() & static_cast<std::uint32_t>(~ZDO::LocalFlag::Member_Connection)
				//);
			}
		}
	}
#endif // VH_LEGACY_WORLD_LOADING

	LOG_INFO(VH_LOGGER, "Loaded {} zdos", m_objectsByID.size());
}



[[nodiscard]] std::pair<ZDO::container::iterator, bool> IZDOManager::_Instantiate(ZDOID zdoid) noexcept {
	//https://jguegant.github.io/blogs/tech/performing-try-emplace.html
	auto&& insert = m_objectsByID.insert(std::make_unique<ZDO>(zdoid));
	//LOG_INFO(VH_LOGGER, "zdo instantiated: {} {}", insert.second, zdoid);

	return insert;
}

std::pair<ZDO::container::iterator, bool> IZDOManager::_Instantiate(ZDOID zdoid, Vector3f position) noexcept {
	auto&& insert = _Instantiate(zdoid);

	// if inserted, then set pos
	if (insert.second) {
		//auto&& zdo = ZDO(*insert.first);
		auto&& zdo = ZDO::make_unsafe_value(insert.first);

		// Set zone and position of ZDO
		zdo->_SetPosition(position);
		_AddZDOToZone(zdo);
	}

	return insert;
}

ZDO::unsafe_value IZDOManager::_Instantiate(Vector3f position) noexcept {
	//ZDOID zdoid = ZDOID(VH_ID, 0);
	ZDOID zdoid = ZDOID(0, 0);
	for(;;) {
		zdoid.set_id(m_nextUid++);
		auto&& insert = _Instantiate(zdoid, position);
		if (insert.second)
			return ZDO::make_unsafe_value(insert.first);
	}
	std::unreachable();
}

//unused
//ZDO::unsafe_value IZDOManager::_TryInstantiate(ZDOID uid, Vector3f position) {
//	// See version #2
//	// ...returns a pair object whose first element is an iterator 
//	//		pointing either to the newly inserted element in the 
//	//		container or to the element whose key is equivalent...
//	// https://cplusplus.com/reference/unordered_map/unordered_map/insert/
//
//	auto&& insert = _Instantiate(uid, position);
//	if (insert.second)
//		return ZDO::make_unsafe_value(insert.first);
//
//	throw std::runtime_error("zdo already exists");
//}



ZDO::unsafe_optional IZDOManager::GetZDO(ZDOID id) {
	if (id) {
		auto&& find = m_objectsByID.find(id);
		if (find != m_objectsByID.end())
			return ZDO::make_unsafe_optional(find);
	}
	return ZDO::unsafe_nullopt;
}


/*
std::pair<IZDOManager::ZDO_iterator, bool> IZDOManager::_GetOrInstantiate(ZDOID id, Vector3f def) {
	auto&& insert = _Instantiate(def, id);
	if (!insert.second)
		return insert;
	
	auto&& insert = m_objectsByID.insert({ id, nullptr });
	
	if (!insert.second) // if new insert failed, return it
		return insert;

	auto&& pair = insert.first;

	auto&& zdo = pair->second;

	zdo = std::make_unique<ZDO>(id, def);
	return insert;
}*/



ZDO::unsafe_value IZDOManager::Instantiate(const Prefab& prefab, Vector3f pos) {
	auto&& zdo = _Instantiate(pos);
	//zdo.get().m_encoded.SetPrefabIndex(PrefabManager()->RequirePrefabIndexByHash(prefab.m_hash));
	//zdo.get().m_pack.Set<ZDO::PREFAB_PACK_INDEX>(PrefabManager()->RequirePrefabIndexByHash(prefab.m_hash));
	zdo->_SetPrefabHash(prefab.m_hash);
	if (prefab.AllFlagsPresent(Prefab::Flag::SYNC_INITIAL_SCALE)) {
		zdo->SetLocalScale(prefab.m_localScale, false);
	}

	return zdo;
}

//unused
//ZDO::unsafe_value IZDOManager::Instantiate(avledet::util::Hash hash, Vector3f pos, const Prefab** outPrefab) {
//	//auto&& zdo = Instantiate()
//	auto&& prefab = PrefabManager()->RequirePrefabByHash(hash);
//	if (outPrefab) *outPrefab = &prefab;
//	
//	return Instantiate(prefab, pos);
//}

/*
ZDO::reference IZDOManager::Instantiate(const ZDO& zdo) {
	assert(false);

	auto&& copy = _Instantiate(zdo.GetPosition());
	
	//copy.get().m_encoded = zdo.m_encoded;
	//copy.get().m_pack = zdo.m_pack;
	copy.get().m_rotation = zdo.m_rotation;

	return copy;
}*/



void IZDOManager::AssignOrReleaseZDOs(Peer& peer) {
	ZoneScoped;

	auto&& zone = IZoneManager::WorldToZonePos(peer.m_pos);

	std::list<ZDO::unsafe_value> m_tempNearObjects;
	GetZDOs_Zone(zone, m_tempNearObjects); // get zdos: zone, nearby
	GetZDOs_NeighborZones(zone, m_tempNearObjects); // get zdos: zone, nearby

	for (auto&& zdo : m_tempNearObjects) {
		if (zdo->IsPersistent()) {
			if (zdo->IsOwner(peer.GetUserID())) {
				// If peer no longer in area of zdo, unclaim zdo
				if (!ZoneManager()->ZonesOverlap(zdo->GetZone(), zone)) {
					zdo->Disown();
				}
			}
			else {
				// If ZDO no longer has owner, or the owner went far away,
				//  Then assign this new peer as owner 
				if (!(zdo->HasOwner() && ZoneManager()->IsPeerNearby(zdo->GetZone(), zdo->Owner()))
					&& ZoneManager()->ZonesOverlap(zdo->GetZone(), zone)) {
					
					zdo->SetOwner(peer.GetUserID());
				}
			}
		}
	}

	if (VH_SETTINGS.TEST_zdoAssignAlgorithm == AssignAlgorithm::DYNAMIC_RADIUS) {

		float minSqDist = std::numeric_limits<float>::max();
		Vector3f closestPos;

		// get the distance to the closest peer
		for (auto&& otherPeer : NetManager()->GetPeers()) {
			if (otherPeer == &peer)
				continue;

			if (!ZoneManager()->IsPeerNearby(IZoneManager::WorldToZonePos(otherPeer->m_pos), peer.GetUserID()))
				continue;

			float sqDist = otherPeer->m_pos.sq_distance_to(peer.m_pos);
			if (sqDist < minSqDist) {
				minSqDist = sqDist;
				closestPos = otherPeer->m_pos;

				if (minSqDist <= 12 * 12) {
					break;
				}
			}
		}

		if (minSqDist != std::numeric_limits<float>::max() 
			&& minSqDist > 12 * 12) {
			// Get zdos immediate to this peer
			auto zdos = GetZDOs(peer.m_pos,
				std::sqrt(minSqDist) * 0.5f - 2.f);

			// Basically reassign zdos from another owner to me instead
			for (auto&& zdo : zdos) {
				if (zdo->IsPersistent()
					&& zdo->GetPosition().sq_distance_to(closestPos) > 12 * 12 // Ensure the ZDO is far from the other player
					) {
					zdo->SetOwner(peer.GetUserID());
				}
			}
		}
	}

}

ZDO::container::iterator IZDOManager::_EraseZDO(ZDO::container::iterator itr) {
	assert(itr != m_objectsByID.end());

	auto&& zdo = ZDO::make_unsafe_value(itr);
	auto&& zdoid = zdo->GetID();

	// update local next only if im the user who created the zdo
	if (zdoid.get_user_id() == VH_ID) {
		this->m_nextUid = std::max(this->m_nextUid, zdoid.get_id() + 1);
	}

	//VLOG(2) << "Destroying zdo (" << zdo->GetPrefab().m_name << ")";

	_RemoveFromSector(zdo);
	{
		auto&& pfind = m_objectsByPrefab.find(zdo->GetPrefabHash());
		if (pfind != m_objectsByPrefab.end()) pfind->second.erase(zdoid);
	}

	// cleans up some zdos
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->m_zdos.erase(zdoid);
	}

	m_erasedZDOs.insert(zdoid);

	//erase members
	ZDO::m_floats.erase(zdoid);
	ZDO::m_vec3.erase(zdoid);
	ZDO::m_quats.erase(zdoid);
	ZDO::m_ints.erase(zdoid);
	ZDO::m_longs.erase(zdoid);
	ZDO::m_strings.erase(zdoid);
	ZDO::m_byteArrays.erase(zdoid);
	ZDO::ZDO_OWNERS.erase(zdoid);
	ZDO::ZDO_TARGETED_CONNECTORS.erase(zdoid);
	
	// erase members and connectors
	//assert(false); //TODO erase from all maps the entry for zdoid
	//ZDO::ZDO_MEMBERS.erase(zdoid);
	////ZDO::ZDO_CONNECTORS.erase(zdo->GetID());
	
	ZDO::ZDO_TARGETED_CONNECTORS.erase(zdoid);

	//LOG_INFO(VH_LOGGER, "zdo erased: {}", zdoid);

	return m_objectsByID.erase(itr);
}

void IZDOManager::GetZDOs_ActiveZones(ZoneID zone, std::list<ZDO::unsafe_value>& out, std::list<ZDO::unsafe_value>& outDistant) {
	// Add ZDOs from immediate sector
	GetZDOs_Zone(zone, out);

	// Add ZDOs from nearby zones
	GetZDOs_NeighborZones(zone, out);

	// Add ZDOs from distant zones
	GetZDOs_DistantZones(zone, outDistant);
}

void IZDOManager::GetZDOs_NeighborZones(ZoneID zone, std::list<ZDO::unsafe_value>& sectorObjects) {
	for (auto z = zone.y - IZoneManager::NEAR_ZRADIUS; z <= zone.y + IZoneManager::NEAR_ZRADIUS; z++) {
		for (auto x = zone.x - IZoneManager::NEAR_ZRADIUS; x <= zone.x + IZoneManager::NEAR_ZRADIUS; x++) {
			auto current = ZoneID(x, z);
			// Skip the center zone
			if (current == zone)
				continue;

			GetZDOs_Zone(current, sectorObjects);
		}
	}
}

void IZDOManager::GetZDOs_DistantZones(ZoneID zone, std::list<ZDO::unsafe_value>& out) {
	for (std::int16_t r = IZoneManager::NEAR_ZRADIUS + 1; 
		r <= IZoneManager::NEAR_ZRADIUS + IZoneManager::DISTANT_ZRADIUS; 
		r++) {
		for (std::int16_t x = zone.x - r; x <= zone.x + r; x++) {
			GetZDOs_Distant(ZoneID(x, zone.y - r), out);
			GetZDOs_Distant(ZoneID(x, zone.y + r), out);
		}
		for (std::int16_t y = zone.y - r + 1; y <= zone.y + r - 1; y++) {
			GetZDOs_Distant(ZoneID(zone.x - r, y), out);
			GetZDOs_Distant(ZoneID(zone.x + r, y), out);
		}
	}
}

std::list<std::pair<ZDO::unsafe_value, float>> IZDOManager::CreateSyncList(Peer& peer) {
	auto zone = IZoneManager::WorldToZonePos(peer.m_pos);

	// Gather all updated ZDO's
	std::list<ZDO::unsafe_value> zoneZDOs;
	std::list<ZDO::unsafe_value> distantZDOs;
	GetZDOs_ActiveZones(zone, zoneZDOs, distantZDOs);

	std::list<std::pair<ZDO::unsafe_value, float>> result;

	// Prepare client-side outdated ZDO's
	const auto time(Valhalla()->Time());
	for (auto&& zdo : zoneZDOs) {
		decltype(Peer::m_zdos)::iterator outItr;
		if (peer.IsOutdatedZDO(zdo, outItr)) {
			float weight = 150;
			if (outItr != peer.m_zdos.end())
				weight = std::min(time - outItr->second.second, 100.f) * 1.5f;

			result.push_back({ zdo, zdo->GetPosition().sq_distance_to(peer.m_pos) - weight * weight });
		}
	}

	// Prioritize ZDO's	
	result.sort([&](const std::pair<ZDO::unsafe_value, float>& first, const std::pair<ZDO::unsafe_value, float>& second) {

		// Sort in rough order of:
		//	flag -> type/priority -> distance ASC -> age ASC

		// https://www.reddit.com/r/valheim/comments/mga1iw/understanding_the_new_networking_mechanisms_from/

		auto&& a = first.first;
		auto&& b = second.first;

		bool flag = a->GetType() == avledet::util::ObjectType::PRIORITIZED && a->HasOwner() && !a->IsOwner(peer.GetUserID());
		bool flag2 = b->GetType() == avledet::util::ObjectType::PRIORITIZED && b->HasOwner() && !b->IsOwner(peer.GetUserID());

		if (flag == flag2) {
			if ((flag && flag2) || a->GetType() == b->GetType()) {
				return first.second < second.second;
			}
			else
				// > (shows large trees first)
				// <  (shows smallest, then small trees first)
				// >= (shows small trees and large trees simultaneously)

				// the problem with seemingly slowly perceived network speed is not with the actual network,
				// but with the ZDOManager being bottlenecked by the expensive HeightmapBuilder
				//	(if Heightmap is not ready, vegetation cannot be generated -> ZDOs cannot be sent)
				//	this only applies to newly generated areas
				return a->GetType() >= b->GetType();
		}
		else {
			return flag;
		}
	});

	// Add a minimum amount of ZDOs
	if (result.size() < 10) {
		for (auto&& zdo2 : distantZDOs) {
			if (peer.IsOutdatedZDO(zdo2)) {
				result.push_back({ zdo2, 0 });
			}
		}
	}

	// Add forcible send ZDOs
	for (auto&& itr = peer.m_forceSend.begin(); itr != peer.m_forceSend.end();) {
		auto&& zdoid = *itr;
		auto zdo = GetZDO(zdoid);
		if (zdo && peer.IsOutdatedZDO(zdo)) {
			result.push_front({ zdo, 0 });
			++itr;
		}
		else {
			itr = peer.m_forceSend.erase(itr);
		}
	}

	return result;
}

void IZDOManager::GetZDOs_Zone(ZoneID zone, std::list<ZDO::unsafe_value>& objects) {
	if (auto&& container = _FindZDOContainer(zone)) {
		objects.insert(objects.end(), container->begin(), container->end());
	}
}

void IZDOManager::GetZDOs_Distant(ZoneID zone, std::list<ZDO::unsafe_value>& objects) {
	if (auto&& container = _FindZDOContainer(zone)) {
		for (auto&& zdo : *container) {
			if (zdo->IsDistant()) {
				objects.push_back(zdo);
			}
		}
	}
}



std::list<ZDO::unsafe_value> IZDOManager::GetZDOs(avledet::util::Hash prefab) {
	std::list<ZDO::unsafe_value> out;
	auto&& find = m_objectsByPrefab.find(prefab);
	if (find != m_objectsByPrefab.end()) {
		auto&& container = find->second;
		out.insert(out.end(), container.begin(), container.end());
	}
	return out;
}

std::list<ZDO::unsafe_value> IZDOManager::GetZDOs(pred_t pred) {
	std::list<ZDO::unsafe_value> out;
	for (auto&& v : m_objectsByID) {
		auto&& zdo = ZDO::make_unsafe_value(v);
		if (!pred || pred(zdo)) {
			out.push_back(zdo);
		}
	}
	return out;
}



std::list<ZDO::unsafe_value> IZDOManager::SomeZDOs(Vector3f pos, float radius, std::size_t max, pred_t pred) {
	std::list<ZDO::unsafe_value> out;

	const float sqRadius = radius * radius;

	auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));

	for (auto z = minZone.y; z <= maxZone.y; z++) {
		for (auto x = minZone.x; x <= maxZone.x; x++) {
			if (auto&& container = _FindZDOContainer(ZoneID(x, z))) {
				for (auto&& zdo : *container) {
					if (zdo->GetPosition().sq_distance_to(pos) <= sqRadius
						&& (!pred || pred(zdo)))
					{
						if (max--)
							out.push_back(zdo);
						else
							return out;
					}
				}
			}
		}
	}

	return out;
}

std::list<ZDO::unsafe_value> IZDOManager::SomeZDOs(ZoneID zone, std::size_t max, pred_t pred) {
	std::list<ZDO::unsafe_value> out;

	if (auto&& container = _FindZDOContainer(zone)) {
		for (auto&& zdo : *container) {
			if (!pred || pred(zdo)) {
				if (max--)
					out.push_back(zdo);
				else
					return out;
			}
		}
	}

	return out;
}



ZDO::unsafe_optional IZDOManager::NearestZDO(Vector3f pos, float radius, pred_t pred) {
	float minSqDist = radius * radius;
	
	ZDO::unsafe_optional out;
	//float minSqDist = std::numeric_limits<float>::max();

	auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));

	for (auto z = minZone.y; z <= maxZone.y; z++) {
		for (auto x = minZone.x; x <= maxZone.x; x++) {
			if (auto&& container = _FindZDOContainer(ZoneID(x, z))) {
				for (auto&& zdo : *container) {
					float sqDist = zdo->GetPosition().sq_distance_to(pos);
					if (sqDist < minSqDist // Filter to closest ZDO
						&& (!pred || pred(zdo)))
					{
						out = zdo;
						minSqDist = sqDist;
					}
				}
			}
		}
	}

	return out;
}



// Global send
void IZDOManager::ForceSendZDO(ZDOID id) {
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->ForceSendZDO(id);
	}
}

bool IZDOManager::SendZDOs(Peer& peer, bool flush) {
	ZoneScoped;

	auto sendQueueSize = peer.m_socket->get_send_queue_size();

	// flushing forces a packet send
	const auto threshold = VH_SETTINGS.zdoMaxCongestion;
	if (!flush && sendQueueSize > threshold)
		return false;

	auto availableSpace = threshold - sendQueueSize;
	if (availableSpace < VH_SETTINGS.zdoMinCongestion)
		return false;

	auto syncList = CreateSyncList(peer);

	// continue only if there are updated/invalid ZDOs to send
	if (syncList.empty() && peer.m_invalidSector.empty())
		return false;

	// TODO a better optimization would be to use write a special
	//	preserializer that prepends packet data to the peer-buffer
	//	to avoid a few buffer allocs
	//	this only matters if performance is upmost concern, which it is because c :>

	peer.SubInvoke(avledet::util::hashes::Rpc::ZDOData, [&peer, &syncList, availableSpace](DataWriter& writer) {
		writer.write(peer.m_invalidSector);

		const auto time = Valhalla()->Time();

		for (auto&& itr = syncList.begin();
			itr != syncList.end() && writer.size() <= availableSpace;
			itr++) {

			// copy is intentional
			auto zdo = itr->first;

			peer.m_forceSend.erase(zdo->GetID());

			if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::SendingZDO, peer, zdo)) {
				continue;
			}

			writer.write(zdo->GetID());
			writer.write(zdo->GetOwnerRevision());
			writer.write(zdo->GetDataRevision());
			writer.write(zdo->Owner());
			writer.write(zdo->GetPosition());

			//assert(false); //TODO

			writer.write([zdo](DataWriter& writer) {
				zdo->Pack(writer, true);
			});

			peer.m_zdos[zdo->GetID()] = { zdo->GetRevision(), time};
		}
		writer.write(ZDOID::NONE); // null terminator
	});

	if (!peer.m_invalidSector.empty() || !syncList.empty()) {
		peer.m_invalidSector.clear();

		return true;
	}

	return false;
}

void IZDOManager::OnNewPeer(Peer& peer) {
	peer.Register(avledet::util::hashes::Rpc::ZDOData, [this](Peer* peer, DataReader reader) {
		ZoneScoped;

		// Only allow if normal mode
		if (peer->IsGated())
			return;

		//assert(false); //TODO
		reader.read([this](ZDOID zdoid) {
			if (auto zdo = GetZDO(zdoid))
				_InvalidateZDOZone(zdo);
			}
		);
		
		auto time = Valhalla()->Time();

		while (auto zdoid = reader.read<ZDOID>()) {
			auto ownerRev = reader.read<std::uint16_t>();	// owner revision
			auto dataRev = reader.read<std::uint32_t>();		// data revision
			auto owner = reader.read<std::int64_t>();		// owner
			auto pos = reader.read<Vector3f>();			// position

			auto des = DataReader(reader.read<std::vector<char>>());		// dont move this

			/*
			ZDO::Rev rev = { 
				.m_dataRev = dataRev, 
				.m_ownerRev = ownerRev, 
				.m_syncTime = time 
			};*/

			auto&& pair = this->_Instantiate(zdoid);

			//auto&& zdo = ZDO(*pair.first);
			auto &&zdo = ZDO::make_unsafe_value(pair.first);
			auto&& created = pair.second;

			assert(zdoid == zdo->GetID());

			if (!created) {
				// If the incoming data revision is at most older or equal to this revision, we do NOT need to deserialize
				//	(because the data will be the same, or at the worst case, it will be outdated)
				if (dataRev <= zdo->GetDataRevision()) {

					// If the owner has changed, keep a copy
					if (ownerRev > zdo->GetOwnerRevision()) {
						zdo->_SetOwner(owner);
						//zdo.SetOwnerRevision(ownerRev);
						zdo->GetRevision().SetOwnerRevision(ownerRev);
						peer->m_zdos[zdoid] = { 
							ZDO::Rev(dataRev, ownerRev),
							time 
						};
					}
					continue;
				}
			}
			else {
				assert((_FindZDOContainer(zdo->GetZone())) 
					? !_FindZDOContainer(zdo->GetZone())->contains(zdoid) : true);

				if (m_erasedZDOs.contains(zdoid)) {
					m_destroySendList.push_back(zdoid);

					m_objectsByID.erase(pair.first);
					continue;
				}
			}

			// Also used as restore point if this ZDO breaks during deserialization
			//ZDO copy(zdo);

			//try {
				zdo->_SetOwner(owner);
				zdo->GetRevision().SetDataRevision(dataRev);
				zdo->GetRevision().SetOwnerRevision(ownerRev);

				// Unpack the ZDOs primary data
				zdo->Unpack(des, 0);

				VH_DISPATCH_MOD_EVENT(IModManager::Events::ZDOUnpacked, peer, zdo);

				// Only disperse through world if ZDO is new
				if (created) {
					//if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::ZDOCreated, peer, zdo)) {
					//	_EraseZDO(pair.first);
					//	continue;
					//}

					zdo->_SetPosition(pos); //unrevised because is fresh zdo
					_AddZDOToZone(zdo);
					m_objectsByPrefab[zdo->GetPrefabHash()].insert(zdo);
				}
				else {
					//if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::ZDOModified, peer, zdo, copy, pos)) {
					//	zdo = std::move(copy);
					//	continue;
					//}

					zdo->SetPosition(pos);
				}

				assert(_FindZDOContainer(zdo->GetZone()));

				assert(_FindZDOContainer(zdo->GetZone())->contains(zdo));

				peer->m_zdos[zdoid] = {
					zdo->GetRevision(),
					time 
				};
				/*
			}
			catch (const std::runtime_error& e) {
				// erase the zdo from map
				if (created) // if the zdo was just created, throw it away
					EraseZDO(pair.first);
				else zdo = copy; // else, restore the ZDO to the prior revision

				// This will kick the malicious peer
				std::rethrow_exception(std::make_exception_ptr(e));
			}*/
		}
	}); 
}

void IZDOManager::OnPeerQuit(Peer& peer) {
	for (auto&& itr = m_objectsByID.begin(); itr != m_objectsByID.end(); ) {
		auto &&zdo = ZDO::make_unsafe_value(itr);

		if (!zdo->IsPersistent()
			&& (!zdo->HasOwner() || zdo->IsOwner(peer.GetUserID()) || !NetManager()->FindPeerByUserID(zdo->Owner())))
		{
			itr = _DestroyZDO(itr);
		}
		else
			++itr;
	}
}



std::size_t IZDOManager::GetSumZDOMembers() {
	std::size_t res = 0;
	for (auto&& zdo : m_objectsByID) {
		//res += zdo.second->m_members.size();
	}
	return res;
}

float IZDOManager::GetMeanZDOMembers() {
	return (float) GetSumZDOMembers() / m_objectsByID.size();
}

float IZDOManager::GetStDevZDOMembers() {
	const float mean = GetMeanZDOMembers();
	const float n = m_objectsByID.size();

	float res = 0;
	for (auto&& zdo : m_objectsByID) {
		//res += std::pow((float)zdo.second->m_members.size() - mean, 2.f);
	}

	return std::sqrt(res / n);
}

std::size_t IZDOManager::GetTotalZDOAlloc() {
	//std::size_t bytes = m_objectsByID.size() * sizeof(ZDO);
	//for (auto&& zdo : m_objectsByID) bytes += zdo->GetTotalAlloc();
	//return bytes;
	return ZDO::GetTotalAlloc(false);
}

std::size_t IZDOManager::GetCountEmptyZDOs() {
	// so gather each ZDO member, and write how many of them are empty
	std::size_t count = 0;
	for (auto&& zdo : m_objectsByID) {
		auto alloc = zdo->GetTotalAlloc(false);
		if (alloc == 0)
			count++;
	}
	return count;
}
