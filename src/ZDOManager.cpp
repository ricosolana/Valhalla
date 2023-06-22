#include <array>

#include "ZDOManager.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "Hashes.h"
#include "ZoneManager.h"
#include "RouteManager.h"
#include "HashUtils.h"
#include "DungeonManager.h"

auto ZDO_MANAGER(std::make_unique<IZDOManager>());
IZDOManager* ZDOManager() {
	return ZDO_MANAGER.get();
}



void IZDOManager::Init() {
	LOG_INFO(LOGGER, "Initializing ZDOManager");

	RouteManager()->Register(Hashes::Routed::DestroyZDO, 
		[this](Peer*, DataReader reader) {
			// TODO constraint check
			reader.AsEach([this](ZDOID zdoid) {
				EraseZDO(zdoid);
			});
		}
	);
	//auto&& insert = ZDOManager()->m_objectsByID.begin()->second->
		//m_members.insert({0, ZDO::Ord()});
	//insert.first->second.Get
	RouteManager()->Register(Hashes::Routed::C2S_RequestZDO, 
		[this](Peer* peer, ZDOID id) {
			peer->ForceSendZDO(id);
		}
	);
}

void IZDOManager::Update() {
	ZoneScoped;

	if (VUtils::run_periodic<struct periodic_zdo_stats>(3min)) {
		LOG_INFO(LOGGER, "Currently {} zdos (~{:0.02f}mb)", m_objectsByID.size(), (GetTotalZDOAlloc() / 1000000.f));
	}
	/*
	PERIODIC_NOW(3min, {
		LOG_INFO(LOGGER, "Currently {} zdos (~{:0.02f}mb)", m_objectsByID.size(), (GetTotalZDOAlloc() / 1000000.f));
		//VLOG(1) << "ZDO members (sum: " << GetSumZDOMembers()
			//<< ", mean: " << GetMeanZDOMembers()
			//<< ", stdev: " << GetStDevZDOMembers()
			//<< ", empty: " << GetCountEmptyZDOs()
			//<< ")";
	})*/;



	// TODO requires testing
	//	link portals if mode enabled
#if VH_IS_ON(VH_PORTAL_LINKING)
	if (VUtils::run_periodic<struct link_portals>(1s)) {
		auto&& portals = GetZDOs(Hashes::Object::portal_wood);

		// TODO use the optimized Lua ported code for linking portals
		//	not the exact code but the way the algo works

		auto&& FindRandomUnconnectedPortal = [&](ZDOID skip, std::string_view tag) -> ZDO* {
			std::vector<ZDO> list;
			for (auto&& zdo : portals) {
				if (zdo.ID() != skip
					&& zdo.GetString(Hashes::ZDO::TeleportWorld::TAG) == tag
					&& !zdo.GetConnectionZDOID(ZDOConnector::Type::Portal))
				{
					list.push_back(zdo);
				}
			}

			if (list.empty()) {
				return nullptr;
			}

			return &list[VUtils::Random::State().Range(0, list.size())].get();
		};

		for (auto&& ref : portals) {
			auto&& zdo = ref.get();
			auto&& connectionZDOID = zdo.GetConnectionZDOID(ZDOConnector::Type::Portal);
			auto&& string = zdo.GetString(Hashes::ZDO::TeleportWorld::TAG);
			if (connectionZDOID) {
				auto&& zdo2 = GetZDO(connectionZDOID);
				if (!zdo2 || zdo2->GetString(Hashes::ZDO::TeleportWorld::TAG) != string)
				{
					zdo.SetLocal();
					zdo.SetConnection(ZDOConnector::Type::Portal, ZDOID::NONE);
					ForceSendZDO(zdo.ID());
				}
			}
		}

		for (auto&& ref : portals) {
			auto&& zdo3 = ref.get();

			if (!zdo3.GetConnectionZDOID(ZDOConnector::Type::Portal)) {
				auto&& string2 = zdo3.GetString(Hashes::ZDO::TeleportWorld::TAG);
				auto&& zdo4 = FindRandomUnconnectedPortal(zdo3.ID(), string2);
				if (zdo4) {
					zdo3.SetLocal();
					zdo4->SetLocal();
					zdo3.SetConnection(ZDOConnector::Type::Portal, zdo4->ID());
					zdo4->SetConnection(ZDOConnector::Type::Portal, zdo3->ID());
					ForceSendZDO(zdo3.ID());
					ForceSendZDO(zdo4->ID());
				}
			}
		}
	}
#endif



	auto&& peers = NetManager()->GetPeers();
	
	if (VUtils::run_periodic<struct zdos_release_assign>(VH_SETTINGS.zdoAssignInterval)) {
		for (auto&& peer : peers) {
			if (
#if VH_IS_ON(VH_PLAYER_CAPTURE)
				(VH_SETTINGS.packetMode != PacketMode::PLAYBACK 
				|| std::dynamic_pointer_cast<ReplaySocket>(peer->m_socket))) &&
#endif // VH_PLAYER_CAPTURE
				!peer->IsGated()) 
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

	// Send ZDOS:
	//PERIODIC_NOW(VH_SETTINGS.zdoSendInterval, {
	//	for (auto&& peer : peers) {
	//		SendZDOs(*peer, false);
	//	}
	//});
	

	if (!m_destroySendList.empty()) {

		// TODO make a member variable?
		//	think about emulated zdo containers (like replaying actions to specific peers)
		//	this is a functionality I might be planning on into the future
		m_temp.clear();
		DataWriter(m_temp).Write(m_destroySendList);
		m_destroySendList.clear();

		RouteManager()->InvokeAll(Hashes::Routed::DestroyZDO, m_temp);
	}
}



void IZDOManager::_AddZDOToZone(ZDO& zdo) {
	if (auto&& container = _GetZDOContainer<true>(zdo.GetZone())) {
		container->insert(&zdo);
	}
}

void IZDOManager::_RemoveFromSector(ZDO& zdo) {
	if (auto&& container = _GetZDOContainer(zdo.GetZone())) {
		container->erase(&zdo);
	}
}

void IZDOManager::_InvalidateZDOZone(ZDO& zdo) {
	_RemoveFromSector(zdo);

	for (auto&& peer : NetManager()->GetPeers()) {
		peer->ZDOSectorInvalidated(zdo);
	}
}



void IZDOManager::Save(DataWriter& writer) {
	//pkg.Write(Valhalla()->ID());
	writer.Write<int64_t>(0);
	writer.Write(m_nextUid);
	
	{
		// Write zdos (persistent)
		const auto start = writer.Position();

		int32_t count = 0;
		writer.Write(count);

		for (auto&& pair : m_objectsByID) {
			auto&& zdo = *pair.second;
			if (zdo.IsPersistent()) {
				zdo.Pack(writer, false);
				count++;
			}
		}

		const auto end = writer.Position();
		writer.SetPos(start);
		writer.Write(count);
		writer.SetPos(end);
	}
}



void IZDOManager::Load(DataReader& reader, int version) {
	reader.Read<int64_t>(); // server id
	reader.Read<uint32_t>(); // next uid
	
	auto count = reader.Read<uint32_t>();
	for (decltype(count) i = 0; i < count; i++) {
		auto zdo = std::make_unique<ZDO>();

#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
		if (version < 31) {
			zdo->m_id = reader.Read<ZDOID>();
			auto zdoReader = reader.Read<DataReader>();

			zdo->Load31Pre(zdoReader, version);
		}
		else 
#endif // VH_LEGACY_WORLD_COMPATABILITY
		{
			zdo->Unpack(reader, version);
		}

		_AddZDOToZone(*zdo.get());

#if VH_IS_ON(VH_STANDARD_PREFABS)
		auto&& prefab = zdo->GetPrefab();

		m_objectsByPrefab[prefab.m_hash].insert(zdo.get());

#if VH_IS_ON(VH_DUNGEON_REGENERATION)
		if (prefab.AllFlagsPresent(Prefab::Flag::DUNGEON)) {
			// Only add real sky dungeon
			if (zdo->Position().y > 4000)
				DungeonManager()->m_dungeonInstances.push_back(zdo->ID());
		}
#endif // VH_DUNGEON_REGENERATION
#endif // VH_STANDARD_PREFABS
		m_objectsByID[zdo->ID()] = std::move(zdo);
	}

#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
	if (version < 31) {
		auto deadCount = reader.Read<int32_t>();
		for (decltype(deadCount) j = 0; j < deadCount; j++) {
			reader.Read<int64_t>();
			reader.Read<uint32_t>();
			reader.Read<int64_t>();
		}

#if VH_IS_ON(VH_STANDARD_PREFABS)
		// Owners, Terrains, and Seeds have already been converted

		// convert portals
		for (auto&& ref : GetZDOs(Hashes::Object::portal_wood)) {
			auto&& zdo = ref.get();
			
			auto&& string = zdo.GetString(Hashes::ZDO::TeleportWorld::TAG);
			ZDOID zdoid; zdo.Extract("target", zdoid);
			if (zdoid && !string.empty()) {
				auto&& zdo2 = GetZDO(zdoid);
				if (zdo2) {
					auto&& string2 = zdo2->GetString(Hashes::ZDO::TeleportWorld::TAG);
					ZDOID zdoid2; zdo2->Extract("target", zdoid2);
					if (string == string2
						&& zdoid == zdo2->ID()
						&& zdoid2 == zdo.ID()
						) 
					{
						zdo.SetLocal();
						zdo2->SetLocal();
						zdo.SetConnection(ZDOConnector::Type::Portal, zdo2->ID());
						zdo2->SetConnection(ZDOConnector::Type::Portal, zdo.ID());
					}
				}
			}
		}
		
		// convert spawners
		for (auto&& ref : GetZDOs(Prefab::Flag::CREATURE_SPAWNER, Prefab::Flag::NONE)) {
			auto&& zdo = ref.get();
			zdo.SetLocal();
			ZDOID zdoid; zdo.Extract("spawn_id", zdoid);
			auto&& zdo2 = GetZDO(zdoid);
			zdo.SetConnection(ZDOConnector::Type::Spawned, zdo2 ? zdo2->ID() : ZDOID::NONE);
		}

		// convert sync transforms
		for (auto&& ref : GetZDOs(Prefab::Flag::SYNCED_TRANSFORM, Prefab::Flag::NONE)) {
			auto&& zdo = ref.get();
			zdo.SetLocal();
			ZDOID zdoid; zdo.Extract("parentID", zdoid);
			auto&& zdo2 = GetZDO(zdoid);
			if (zdo2) {
				zdo.SetConnection(ZDOConnector::Type::Spawned, zdo2->ID());
			}
			else {
				//zdo.m_pack.Set<ZDO::FLAGS_PACK_INDEX>(
					// zero out connector bit
					//zdo.m_pack.Get<ZDO::FLAGS_PACK_INDEX>() & static_cast<uint32_t>(~ZDO::LocalFlag::Member_Connection)
				//);
			}
		}
#endif // VH_STANDARD_PREFABS
	}
#endif // VH_LEGACY_WORLD_COMPATABILITY

	LOG_INFO(LOGGER, "Loaded {} zdos", m_objectsByID.size());
}

ZDO::reference IZDOManager::_Instantiate(Vector3f position) {
	ZDOID zdoid = ZDOID(VH_ID, 0);
	for(;;) {
		zdoid.SetUID(m_nextUid++);
		auto&& pair = m_objectsByID.insert({ zdoid, nullptr });
		if (!pair.second) { // if insert failed, keep looping
			continue;
		}

		auto&& zdo = pair.first->second;

		zdo = std::make_unique<ZDO>(zdoid, position);
		_AddZDOToZone(*zdo);
		return *zdo;
	}
}

ZDO::reference IZDOManager::_Instantiate(ZDOID uid, Vector3f position) {
	// See version #2
	// ...returns a pair object whose first element is an iterator 
	//		pointing either to the newly inserted element in the 
	//		container or to the element whose key is equivalent...
	// https://cplusplus.com/reference/unordered_map/unordered_map/insert/

	auto&& pair = m_objectsByID.insert({ uid, nullptr });
	if (!pair.second) // if insert failed, throw
		throw std::runtime_error("zdo id already exists");

	auto&& zdo = pair.first->second; zdo = std::make_unique<ZDO>(uid, position);

	_AddZDOToZone(*zdo);
	//m_objectsByPrefab[zdo->PrefabHash()].insert(zdo.get());

	return zdo;
}

ZDO* IZDOManager::GetZDO(ZDOID id) {
	if (id) {
		auto&& find = m_objectsByID.find(id);
		if (find != m_objectsByID.end())
			return find->second.get();
	}
	return nullptr;
}



std::pair<IZDOManager::ZDO_iterator, bool> IZDOManager::_GetOrInstantiate(ZDOID id, Vector3f def) {
	auto&& pair = m_objectsByID.insert({ id, nullptr });
	
	if (!pair.second) // if new insert failed, return it
		return pair;

	auto&& zdo = pair.first->second;

	zdo = std::make_unique<ZDO>(id, def);
	return pair;
}



#if VH_IS_ON(VH_STANDARD_PREFABS)
ZDO::reference IZDOManager::Instantiate(const Prefab& prefab, Vector3f pos) {
	auto&& zdo = ZDOManager()->_Instantiate(pos);
	//zdo.get().m_encoded.SetPrefabIndex(PrefabManager()->RequirePrefabIndexByHash(prefab.m_hash));
	//zdo.get().m_pack.Set<ZDO::PREFAB_PACK_INDEX>(PrefabManager()->RequirePrefabIndexByHash(prefab.m_hash));
	zdo.get()._SetPrefabHash(prefab.m_hash);
	if (prefab.AllFlagsPresent(Prefab::Flag::SYNC_INITIAL_SCALE)) {
		zdo.get().SetLocalScale(prefab.m_localScale, false);
	}
	
	//zdo.get()._SetPersistent(!prefab.AllFlagsPresent(Prefab::Flag::SESSIONED));
	//zdo.get()._SetDistant(prefab.AllFlagsPresent(Prefab::Flag::DISTANT));
	//zdo.get()._SetType(prefab.m_type);

	return zdo;
}

ZDO::reference IZDOManager::Instantiate(HASH_t hash, Vector3f pos, const Prefab** outPrefab) {
	//auto&& zdo = Instantiate()
	auto&& prefab = PrefabManager()->RequirePrefabByHash(hash);
	if (outPrefab) *outPrefab = &prefab;
	
	return Instantiate(prefab, pos);
}
#endif

ZDO::reference IZDOManager::Instantiate(const ZDO& zdo) {
	auto&& copy = _Instantiate(zdo->Position());
	
	//copy.get().m_encoded = zdo.m_encoded;
	copy.get().m_pack = zdo->m_pack;
	copy.get().m_rotation = zdo->m_rotation;

	return copy;
}



void IZDOManager::AssignOrReleaseZDOs(Peer& peer) {
	ZoneScoped;

	auto&& zone = IZoneManager::WorldToZonePos(peer.m_pos);

	std::list<ZDO::reference> m_tempNearObjects;
	GetZDOs_Zone(zone, m_tempNearObjects); // get zdos: zone, nearby
	GetZDOs_NeighborZones(zone, m_tempNearObjects); // get zdos: zone, nearby

	for (auto&& ref : m_tempNearObjects) {
		auto&& zdo = ref.get();
		if (zdo.IsPersistent()) {
			if (zdo.IsOwner(peer.m_characterID.GetOwner())) {
				// If peer no longer in area of zdo, unclaim zdo
				if (!ZoneManager()->ZonesOverlap(zdo.GetZone(), zone)) {
					zdo.Disown();
				}
			}
			else {
				// If ZDO no longer has owner, or the owner went far away,
				//  Then assign this new peer as owner 
				if (!(zdo.HasOwner() && ZoneManager()->IsPeerNearby(zdo.GetZone(), zdo.Owner()))
					&& ZoneManager()->ZonesOverlap(zdo.GetZone(), zone)) {
					
					zdo.SetOwner(peer.m_characterID.GetOwner());
				}
			}
		}
	}

	if (VH_SETTINGS.zdoAssignAlgorithm == AssignAlgorithm::DYNAMIC_RADIUS) {

		float minSqDist = std::numeric_limits<float>::max();
		Vector3f closestPos;

		// get the distance to the closest peer
		for (auto&& otherPeer : NetManager()->GetPeers()) {
			if (otherPeer == &peer)
				continue;

			if (!ZoneManager()->IsPeerNearby(IZoneManager::WorldToZonePos(otherPeer->m_pos), peer.m_characterID.GetOwner()))
				continue;

			float sqDist = otherPeer->m_pos.SqDistance(peer.m_pos);
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
			for (auto&& ref : zdos) {
				auto&& zdo = ref.get();
				if (zdo.IsPersistent()
					&& zdo.m_pos.SqDistance(closestPos) > 12 * 12 // Ensure the ZDO is far from the other player
					) {
					zdo.SetOwner(peer.m_characterID.GetOwner());
				}
			}
		}
	}

}

IZDOManager::ZDO_iterator IZDOManager::_EraseZDO(IZDOManager::ZDO_iterator itr) {
	assert(itr != m_objectsByID.end());

	auto&& zdoid = itr->first;
	auto&& zdo = itr->second;

	//VLOG(2) << "Destroying zdo (" << zdo->GetPrefab().m_name << ")";

	_RemoveFromSector(*zdo);
#if VH_IS_ON(VH_STANDARD_PREFABS)
	auto&& pfind = m_objectsByPrefab.find(zdo->GetPrefabHash());
	if (pfind != m_objectsByPrefab.end()) pfind->second.erase(zdo.get());
#endif

	// cleans up some zdos
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->m_zdos.erase(zdoid);
	}

	m_erasedZDOs.insert(zdoid);
	
	// erase members and connectors
	ZDO::ZDO_MEMBERS.erase(zdo->ID());
	//ZDO::ZDO_CONNECTORS.erase(zdo->ID());
	ZDO::ZDO_TARGETED_CONNECTORS.erase(zdo->ID());

	return m_objectsByID.erase(itr);
}

void IZDOManager::GetZDOs_ActiveZones(ZoneID zone, std::list<ZDO::reference>& out, std::list<ZDO::reference>& outDistant) {
	// Add ZDOs from immediate sector
	GetZDOs_Zone(zone, out);

	// Add ZDOs from nearby zones
	GetZDOs_NeighborZones(zone, out);

	// Add ZDOs from distant zones
	GetZDOs_DistantZones(zone, outDistant);
}

void IZDOManager::GetZDOs_NeighborZones(ZoneID zone, std::list<ZDO::reference>& sectorObjects) {
	for (auto z = zone.y - IZoneManager::NEAR_ACTIVE_AREA; z <= zone.y + IZoneManager::NEAR_ACTIVE_AREA; z++) {
		for (auto x = zone.x - IZoneManager::NEAR_ACTIVE_AREA; x <= zone.x + IZoneManager::NEAR_ACTIVE_AREA; x++) {
			auto current = ZoneID(x, z);
			// Skip the center zone
			if (current == zone)
				continue;

			GetZDOs_Zone(current, sectorObjects);
		}
	}
}

void IZDOManager::GetZDOs_DistantZones(ZoneID zone, std::list<ZDO::reference>& out) {
	for (int16_t r = IZoneManager::NEAR_ACTIVE_AREA + 1; 
		r <= IZoneManager::NEAR_ACTIVE_AREA + IZoneManager::DISTANT_ACTIVE_AREA; 
		r++) {
		for (int16_t x = zone.x - r; x <= zone.x + r; x++) {
			GetZDOs_Distant(ZoneID(x, zone.y - r), out);
			GetZDOs_Distant(ZoneID(x, zone.y + r), out);
		}
		for (int16_t y = zone.y - r + 1; y <= zone.y + r - 1; y++) {
			GetZDOs_Distant(ZoneID(zone.x - r, y), out);
			GetZDOs_Distant(ZoneID(zone.x + r, y), out);
		}
	}
}

std::list<std::pair<ZDO::reference, float>> IZDOManager::CreateSyncList(Peer& peer) {
	auto zone = IZoneManager::WorldToZonePos(peer.m_pos);

	// Gather all updated ZDO's
	std::list<ZDO::reference> zoneZDOs;
	std::list<ZDO::reference> distantZDOs;
	GetZDOs_ActiveZones(zone, zoneZDOs, distantZDOs);

	std::list<std::pair<ZDO::reference, float>> result;

	// Prepare client-side outdated ZDO's
	const auto time(Valhalla()->Time());
	for (auto&& ref : zoneZDOs) {
		decltype(Peer::m_zdos)::iterator outItr;
		if (peer.IsOutdatedZDO(ref, outItr)) {
			auto&& zdo = ref.get();

			float weight = 150;
			if (outItr != peer.m_zdos.end())
				weight = std::min(time - outItr->second.second, 100.f) * 1.5f;

			result.push_back({ zdo, zdo.Position().SqDistance(peer.m_pos) - weight * weight });
		}
	}

	// Prioritize ZDO's	
	result.sort([&](const std::pair<ZDO::reference, float>& first, const std::pair<ZDO::reference, float>& second) {

		// Sort in rough order of:
		//	flag -> type/priority -> distance ASC -> age ASC

		// https://www.reddit.com/r/valheim/comments/mga1iw/understanding_the_new_networking_mechanisms_from/

		auto&& a = first.first.get();
		auto&& b = second.first.get();

		bool flag = a.GetType() == ObjectType::PRIORITIZED && a.HasOwner() && !a.IsOwner(peer.m_characterID.GetOwner());
		bool flag2 = b.GetType() == ObjectType::PRIORITIZED && b.HasOwner() && !b.IsOwner(peer.m_characterID.GetOwner());

		if (flag == flag2) {
			if ((flag && flag2) || a.GetType() == b.GetType()) {
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
				return a.GetType() >= b.GetType();
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
		if (zdo && peer.IsOutdatedZDO(*zdo)) {
			result.push_front({ *zdo, 0 });
			++itr;
		}
		else {
			itr = peer.m_forceSend.erase(itr);
		}
	}

	return result;
}

void IZDOManager::GetZDOs_Zone(ZoneID zone, std::list<ZDO::reference>& objects) {
	if (auto&& container = _GetZDOContainer(zone)) {
		std::transform(container->begin(), container->end(), std::back_inserter(objects), [](ZDO* zdo) { return std::ref(*zdo); });
	}
}

void IZDOManager::GetZDOs_Distant(ZoneID zone, std::list<ZDO::reference>& objects) {
	if (auto&& container = _GetZDOContainer(zone)) {
		for (auto&& zdo : *container) {
			if (zdo->IsDistant()) {
				objects.push_back(*zdo);
			}
		}
	}
}



#if VH_IS_ON(VH_STANDARD_PREFABS)
std::list<ZDO::reference> IZDOManager::GetZDOs(HASH_t prefab) {
	std::list<ZDO::reference> out;
	auto&& find = m_objectsByPrefab.find(prefab);
	if (find != m_objectsByPrefab.end()) {
		auto&& zdos = find->second;
		std::transform(zdos.begin(), zdos.end(), std::back_inserter(out), [](ZDO* zdo) { return std::ref(*zdo); });
	}
	return out;
}

std::list<ZDO::reference> IZDOManager::GetZDOs(const std::function<bool(const ZDO&)>& pred) {
	std::list<ZDO::reference> out;
	for (auto&& pair : m_objectsByID) {
		auto&& zdo = *pair.second;
		if (!pred || pred(zdo)) {
			out.push_back(zdo);
		}
	}
	return out;
}
#endif



std::list<ZDO::reference> IZDOManager::SomeZDOs(Vector3f pos, float radius, size_t max, const std::function<bool(const ZDO&)>& pred) {
	std::list<ZDO::reference> out;

	const float sqRadius = radius * radius;

	auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));

	for (auto z = minZone.y; z <= maxZone.y; z++) {
		for (auto x = minZone.x; x <= maxZone.x; x++) {
			if (auto&& container = _GetZDOContainer(ZoneID(x, z))) {
				for (auto&& zdo : *container) {
					if (zdo->Position().SqDistance(pos) <= sqRadius
						&& (!pred || pred(*zdo)))
					{
						if (max--)
							out.push_back(*zdo);
						else
							return out;
					}
				}
			}
		}
	}

	return out;
}

std::list<ZDO::reference> IZDOManager::SomeZDOs(ZoneID zone, size_t max, const std::function<bool(const ZDO&)>& pred) {
	std::list<ZDO::reference> out;

	if (auto&& container = _GetZDOContainer(zone)) {
		for (auto&& obj : *container) {
			if (!pred || pred(*obj)) {
				if (max--)
					out.push_back(*obj);
				else
					return out;
			}
		}
	}

	return out;
}



ZDO* IZDOManager::NearestZDO(Vector3f pos, float radius, const std::function<bool(const ZDO&)>& pred) {
	//std::list<ZDO::reference> out;

	const float sqRadius = radius * radius;
	
	ZDO* zdo{};
	float minSqDist = std::numeric_limits<float>::max();

	auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));

	for (auto z = minZone.y; z <= maxZone.y; z++) {
		for (auto x = minZone.x; x <= maxZone.x; x++) {
			if (auto&& container = _GetZDOContainer(ZoneID(x, z))) {
				for (auto&& obj : *container) {
					float sqDist = obj->Position().SqDistance(pos);
					if (sqDist <= sqRadius // Filter to ZDO within radius
						&& sqDist < minSqDist // Filter to closest ZDO
						&& (!pred || pred(*obj)))
					{
						zdo = obj;
						minSqDist = sqDist;
					}
				}
			}
		}
	}

	return zdo;
}



// Global send
void IZDOManager::ForceSendZDO(ZDOID id) {
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->ForceSendZDO(id);
	}
}

bool IZDOManager::SendZDOs(Peer& peer, bool flush) {
	ZoneScoped;

	auto sendQueueSize = peer.m_socket->GetSendQueueSize();

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

	peer.SubInvoke(Hashes::Rpc::ZDOData, [&peer, &syncList, availableSpace](DataWriter& writer) {
		writer.Write(peer.m_invalidSector);

		const auto time = Valhalla()->Time();

		for (auto&& itr = syncList.begin();
			itr != syncList.end() && writer.size() <= availableSpace;
			itr++) {

			auto&& zdo = itr->first.get();

			peer.m_forceSend.erase(zdo.ID());

			if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::SendingZDO, peer, zdo)) {
				continue;
			}

			writer.Write(zdo.ID());
			writer.Write(zdo.GetOwnerRevision());
			writer.Write(zdo.GetDataRevision());
			writer.Write(zdo.Owner());
			writer.Write(zdo.Position());

			writer.SubWrite([&zdo](DataWriter& writer) {
				zdo.Pack(writer, true);
			});

			peer.m_zdos[zdo.ID()] = { zdo.m_rev, time };
		}
		writer.Write(ZDOID::NONE); // null terminator
	});

	if (!peer.m_invalidSector.empty() || !syncList.empty()) {
		peer.m_invalidSector.clear();

		return true;
	}

	return false;
}

void IZDOManager::OnNewPeer(Peer& peer) {
	peer.Register(Hashes::Rpc::ZDOData, [this](Peer* peer, DataReader reader) {
		ZoneScoped;

		// Only allow if normal mode
		if (
#if VH_IS_ON(VH_PLAYER_CAPTURE)
			(VH_SETTINGS.packetMode == PacketMode::PLAYBACK
			&& !std::dynamic_pointer_cast<ReplaySocket>(peer->m_socket)) ||
#endif
			peer->IsGated())
			return;

		{
			reader.AsEach([this](ZDOID zdoid) {
				if (auto zdo = GetZDO(zdoid))
					_InvalidateZDOZone(*zdo);
				}
			);
		}

		auto time = Valhalla()->Time();

		while (auto zdoid = reader.Read<ZDOID>()) {
			auto ownerRev = reader.Read<uint16_t>();	// owner revision
			auto dataRev = reader.Read<uint32_t>();		// data revision
			auto owner = reader.Read<int64_t>();		// owner
			auto pos = reader.Read<Vector3f>();			// position

			auto des = reader.Read<DataReader>();		// dont move this

			/*
			ZDO::Rev rev = { 
				.m_dataRev = dataRev, 
				.m_ownerRev = ownerRev, 
				.m_syncTime = time 
			};*/

			auto&& pair = this->_GetOrInstantiate(zdoid, pos);

			auto&& zdo = *pair.first->second;
			auto&& created = pair.second;

			if (!created) [[likely]] {
				// If the incoming data revision is at most older or equal to this revision, we do NOT need to deserialize
				//	(because the data will be the same, or at the worst case, it will be outdated)
				if (dataRev <= zdo.m_rev.GetDataRevision()) {

					// If the owner has changed, keep a copy
					if (ownerRev > zdo.m_rev.GetOwnerRevision()) {
						zdo._SetOwner(owner);
						zdo.m_rev.SetOwnerRevision(ownerRev);
						peer->m_zdos[zdoid] = { 
							ZDO::Rev(dataRev, ownerRev),
							time 
						};
					}
					continue;
				}
			}
			else [[unlikely]] {
				if (m_erasedZDOs.contains(zdoid)) [[unlikely]] {
					m_destroySendList.push_back(zdoid);
					m_objectsByID.erase(pair.first);
					continue;
				}
			}



			// Also used as restore point if this ZDO breaks during deserialization
			//ZDO copy(zdo);

			//try {
				zdo._SetOwner(owner);
				zdo.m_rev.SetDataRevision(dataRev);
				zdo.m_rev.SetOwnerRevision(ownerRev);

				// Unpack the ZDOs primary data
				zdo.Unpack(des, 0);

				// Only disperse through world if ZDO is new
				if (created) {
					//if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::ZDOCreated, peer, zdo)) {
					//	EraseZDO(pair.first);
					//	continue;
					//}

					_AddZDOToZone(zdo);
#if VH_IS_ON(VH_STANDARD_PREFABS)
					m_objectsByPrefab[zdo.GetPrefabHash()].insert(&zdo);
#endif
				}
				else {
					//if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::ZDOModified, peer, zdo, copy, pos)) {
					//	zdo = std::move(copy);
					//	continue;
					//}

					zdo.SetPosition(pos);
				}

				peer->m_zdos[zdoid] = {
					zdo.m_rev,
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
		auto&& pair = *itr;

		auto&& zdo = *pair.second.get();
		//auto&& prefab = zdo.GetPrefab();
		
		// Apparently peer does unclaim sessioned ZDOs (Player zdo had 0 owner)
		//assert((prefab.FlagsAbsent(Prefab::Flag::SESSIONED) || zdo.HasOwner()) && "Session ZDOs should always be owned");

		// Remove temporary ZDOs belonging to peers (like particles and attack anims, vfx, sfx...)
		//if (prefab.FlagsPresent(Prefab::Flag::SESSIONED)
			//&& (zdo.IsOwner(peer.m_uuid)))
		//if (prefab.AllFlagsPresent(Prefab::Flag::SESSIONED) 
		if (!zdo.IsPersistent()
			&& (!zdo.HasOwner() || zdo.IsOwner(peer.m_characterID.GetOwner()) || !NetManager()->GetPeerByUUID(zdo.Owner())))
		{
			itr = _DestroyZDO(itr);
		}
		else
			++itr;
	}
}



size_t IZDOManager::GetSumZDOMembers() {
	size_t res = 0;
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

size_t IZDOManager::GetTotalZDOAlloc() {
	size_t bytes = m_objectsByID.size() * sizeof(ZDO);
	for (auto&& pair : m_objectsByID) bytes += pair.second->GetTotalAlloc();
	return bytes;
}

size_t IZDOManager::GetCountEmptyZDOs() {
	// so gather each ZDO member, and write how many of them are empty
	size_t count = 0;
	for (auto&& pair : m_objectsByID) {
		auto alloc = pair.second->GetTotalAlloc();
		if (alloc == 0)
			count++;
	}
	return count;
}
