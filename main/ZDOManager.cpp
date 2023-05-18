#include <array>

#include "ZDOManager.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "Hashes.h"
#include "ZoneManager.h"
#include "RouteManager.h"
#include "HashUtils.h"

auto ZDO_MANAGER(std::make_unique<IZDOManager>());
IZDOManager* ZDOManager() {
	return ZDO_MANAGER.get();
}



void IZDOManager::Init() {
	LOG_INFO(LOGGER, "Initializing ZDOManager");

	RouteManager()->Register(Hashes::Routed::DestroyZDO, 
		[this](Peer*, DataReader reader) {
			// TODO constraint check
			reader.AsEach([this](const ZDOID& zdoid) {
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
	PERIODIC_NOW(3min, {
		LOG_INFO(LOGGER, "Currently {} zdos (~{:0.02f}mb)", m_objectsByID.size(), (GetTotalZDOAlloc() / 1000000.f));
		//VLOG(1) << "ZDO members (sum: " << GetSumZDOMembers()
			//<< ", mean: " << GetMeanZDOMembers()
			//<< ", stdev: " << GetStDevZDOMembers()
			//<< ", empty: " << GetCountEmptyZDOs()
			//<< ")";
	});

	auto&& peers = NetManager()->GetPeers();
	
#ifdef VH_OPTION_ENABLE_CAPTURE
	// Occasionally release ZDOs
	PERIODIC_NOW(VH_SETTINGS.zdoAssignInterval, {
		for (auto&& peer : peers) {
			if (
				(VH_SETTINGS.packetMode != PacketMode::PLAYBACK
				|| std::dynamic_pointer_cast<ReplaySocket>(peer->m_socket)) 
				&& !peer->m_gatedPlaythrough)
			AssignOrReleaseZDOs(*peer);
		}
	});
#else // macro expansion screwed this up
	PERIODIC_NOW(VH_SETTINGS.zdoAssignInterval, {
		for (auto&& peer : peers) {
			AssignOrReleaseZDOs(*peer);
		}
	});
#endif

	// Send ZDOS:
	PERIODIC_NOW(VH_SETTINGS.zdoSendInterval, {
		for (auto&& peer : peers) {
			SendZDOs(*peer, false);
		}
	});
	

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


bool IZDOManager::AddZDOToZone(ZDO& zdo) {
	int num = SectorToIndex(zdo.GetZone());
	if (num != -1) {
		auto&& pair = m_objectsBySector[num].insert(&zdo);
		assert(pair.second);
		return true;
	}
	return false;
}

void IZDOManager::RemoveFromSector(ZDO& zdo) {
	int num = SectorToIndex(zdo.GetZone());
	if (num != -1) {
		m_objectsBySector[num].erase(&zdo);
	}
}

void IZDOManager::InvalidateZDOZone(ZDO& zdo) {
	RemoveFromSector(zdo);

	for (auto&& peer : NetManager()->GetPeers()) {
		peer->ZDOSectorInvalidated(zdo);
	}
}



void IZDOManager::Save(DataWriter& writer) {
	//pkg.Write(Valhalla()->ID());
	writer.Write<OWNER_t>(0);
	writer.Write(m_nextUid);
	
	{
		// Write zdos (persistent)
		const auto start = writer.Position();

		int32_t count = 0;
		writer.Write(count);

		{
			//NetPackage zdoPkg;
			for (auto&& sectorObjects : m_objectsBySector) {
				for (auto zdo : sectorObjects) {
					if (zdo->m_prefab.get().AnyFlagsAbsent(Prefab::Flag::SESSIONED)) {
						writer.Write(zdo->ID());
						writer.SubWrite([&zdo](DataWriter& writer) {
							zdo->Save(writer);
						});
						count++;
					}
				}
			}
		}

		//const auto end = pkg.Position();
		writer.SetPos(start);
		writer.Write(count);
		//pkg.SetPos(end);
		writer.SetPos(writer.size());
	}

	writer.Write<int32_t>(0);
}



void IZDOManager::Load(DataReader& reader, int version) {
	reader.Read<OWNER_t>(); // skip server id
	m_nextUid = reader.Read<uint32_t>();
	const auto count = reader.Read<int32_t>();
	if (count < 0)
		throw std::runtime_error("count must be positive");

	int purgeCount = 0;

	for (int i = 0; i < count; i++) {
		auto zdo = std::make_unique<ZDO>();
		zdo->m_id = reader.Read<ZDOID>();
		auto zdoReader = reader.Read<DataReader>();

		bool modern = zdo->Load(zdoReader, version);

		// TODO redundant?
		//	thinking about it, this might be for very old versions where the ID is not normal
		//if (zdo->ID().m_uuid == VH_ID
		//	&& zdo->ID().m_id >= m_nextUid)
		//{
		//	assert(false && "looks like this might be significant");
		//	m_nextUid = zdo->ID().m_id + 1;
		//}

		if (modern || !VH_SETTINGS.worldModern) {
			auto&& prefab = zdo->GetPrefab();

			AddZDOToZone(*zdo.get());
			m_objectsByPrefab[prefab.m_hash].insert(zdo.get());

			m_objectsByID[zdo->ID()] = std::move(zdo);
		}
		else purgeCount++;
	}

	auto deadCount = reader.Read<int32_t>();
	for (int j = 0; j < deadCount; j++) {
		reader.Read<ZDOID>();
		TICKS_t(reader.Read<int64_t>());
	}

	LOG_INFO(LOGGER, "Loaded {} zdos", m_objectsByID.size());
	if (purgeCount) {
		LOG_INFO(LOGGER, "Purged {} old zdos", purgeCount);
	}
}

ZDO& IZDOManager::Instantiate(Vector3f position) {
	ZDOID zdoid = ZDOID(VH_ID, 0);
	for(;;) {
		zdoid.SetUID(m_nextUid++);
		auto&& pair = m_objectsByID.insert({ zdoid, nullptr });
		if (!pair.second) { // if insert failed, keep looping
			continue;
		}

		auto&& zdo = pair.first->second;

		zdo = std::make_unique<ZDO>(zdoid, position);
		AddZDOToZone(*zdo.get());
		return *zdo.get();
	}
}

ZDO& IZDOManager::Instantiate(ZDOID uid, Vector3f position) {
	// See version #2
	// ...returns a pair object whose first element is an iterator 
	//		pointing either to the newly inserted element in the 
	//		container or to the element whose key is equivalent...
	// https://cplusplus.com/reference/unordered_map/unordered_map/insert/

	auto&& pair = m_objectsByID.insert({ uid, nullptr });
	if (!pair.second) // if insert failed, throw
		throw std::runtime_error("zdo id already exists");

	auto&& zdo = pair.first->second; zdo = std::make_unique<ZDO>(uid, position);

	AddZDOToZone(*zdo.get());

	return *zdo.get();
}

ZDO* IZDOManager::GetZDO(ZDOID id) {
	if (id) {
		auto&& find = m_objectsByID.find(id);
		if (find != m_objectsByID.end())
			return find->second.get();
	}
	return nullptr;
}



std::pair<decltype(IZDOManager::m_objectsByID)::iterator, bool> IZDOManager::GetOrInstantiate(ZDOID id, Vector3f def) {
	auto&& pair = m_objectsByID.insert({ id, nullptr });
	
	if (!pair.second) // if new insert failed, return it
		return pair;

	auto&& zdo = pair.first->second;

	zdo = std::make_unique<ZDO>(id, def);
	return pair;
}



void IZDOManager::AssignOrReleaseZDOs(Peer& peer) {
	auto&& zone = IZoneManager::WorldToZonePos(peer.m_pos);

	std::list<std::reference_wrapper<ZDO>> m_tempNearObjects;
	GetZDOs_Zone(zone, m_tempNearObjects); // get zdos: zone, nearby
	GetZDOs_NeighborZones(zone, m_tempNearObjects); // get zdos: zone, nearby

	for (auto&& ref : m_tempNearObjects) {
		auto&& zdo = ref.get();
		if (zdo.m_persistent) {
			if (zdo.IsOwner(peer.m_uuid)) {
				
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
					
					zdo.SetOwner(peer.m_uuid);
				}
			}
		}
	}
}

decltype(IZDOManager::m_objectsByID)::iterator IZDOManager::EraseZDO(decltype(IZDOManager::m_objectsByID)::iterator itr) {
	auto&& zdoid = itr->first;
	auto&& zdo = itr->second;

	// TODO I dont really understand the point of this
	//if (zdoid.m_uuid == VH_ID && zdoid.m_id >= m_nextUid)
		//m_nextUid = zdoid.m_uuid + 1;

	//VLOG(2) << "Destroying zdo (" << zdo->GetPrefab().m_name << ")";

	RemoveFromSector(*zdo);

	// cleans up some zdos
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->m_zdos.erase(zdoid);
	}

	m_erasedZDOs.insert(zdoid);
	return m_objectsByID.erase(itr);
}

void IZDOManager::EraseZDO(ZDOID zdoid) {
	auto&& find = m_objectsByID.find(zdoid);
	if (find != m_objectsByID.end())
		EraseZDO(find);
}

void IZDOManager::SendAllZDOs(Peer& peer) {
	while (SendZDOs(peer, true));
}

void IZDOManager::GetZDOs_ActiveZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out, std::list<std::reference_wrapper<ZDO>>& outDistant) {
	// Add ZDOs from immediate sector
	GetZDOs_Zone(zone, out);

	// Add ZDOs from nearby zones
	GetZDOs_NeighborZones(zone, out);

	// Add ZDOs from distant zones
	GetZDOs_DistantZones(zone, outDistant);
}

void IZDOManager::GetZDOs_NeighborZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& sectorObjects) {
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

void IZDOManager::GetZDOs_DistantZones(ZoneID zone, std::list<std::reference_wrapper<ZDO>>& out) {
	for (auto r = IZoneManager::NEAR_ACTIVE_AREA + 1; 
		r <= IZoneManager::NEAR_ACTIVE_AREA + IZoneManager::DISTANT_ACTIVE_AREA; 
		r++) {
		for (auto x = zone.x - r; x <= zone.x + r; x++) {
			GetZDOs_Distant({ x, zone.y - r }, out);
			GetZDOs_Distant({ x, zone.y + r }, out);
		}
		for (auto y = zone.y - r + 1; y <= zone.y + r - 1; y++) {
			GetZDOs_Distant({ zone.x - r, y }, out);
			GetZDOs_Distant({ zone.x + r, y }, out);
		}
	}
}

std::list<std::pair<std::reference_wrapper<ZDO>, float>> IZDOManager::CreateSyncList(Peer& peer) {
	auto zone = IZoneManager::WorldToZonePos(peer.m_pos);

	// Gather all updated ZDO's
	std::list<std::reference_wrapper<ZDO>> zoneZDOs;
	std::list<std::reference_wrapper<ZDO>> distantZDOs;
	GetZDOs_ActiveZones(zone, zoneZDOs, distantZDOs);

	std::list<std::pair<std::reference_wrapper<ZDO>, float>> result;

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
	result.sort([&](const std::pair<std::reference_wrapper<ZDO>, float>& first, const std::pair<std::reference_wrapper<ZDO>, float>& second) {

		// Sort in rough order of:
		//	flag -> type/priority -> distance ASC -> age ASC

		// https://www.reddit.com/r/valheim/comments/mga1iw/understanding_the_new_networking_mechanisms_from/

		auto&& a = first.first.get();
		auto&& b = second.first.get();

		bool flag = a.GetPrefab().m_type == Prefab::Type::PRIORITIZED && a.HasOwner() && !a.IsOwner(peer.m_uuid);
		bool flag2 = b.GetPrefab().m_type == Prefab::Type::PRIORITIZED && b.HasOwner() && !b.IsOwner(peer.m_uuid);

		if (flag == flag2) {
			if ((flag && flag2) || a.GetPrefab().m_type == b.GetPrefab().m_type) {
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
				return a.GetPrefab().m_type >= b.GetPrefab().m_type;
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

void IZDOManager::GetZDOs_Zone(ZoneID sector, std::list<std::reference_wrapper<ZDO>>& objects) {
	auto&& obj = m_objectsBySector[sector];
	std::transform(obj.begin(), obj.end(), std::back_inserter(objects), [](ZDO* zdo) { return std::ref(*zdo); });
}

void IZDOManager::GetZDOs_Distant(ZoneID sector, std::list<std::reference_wrapper<ZDO>>& objects) {
	auto&& list = m_objectsBySector[sector];

	for (auto&& zdo : list) {
		if (zdo->m_distant)
			objects.push_back(*zdo);
	}
}



// Global send
void IZDOManager::ForceSendZDO(ZDOID id) {
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->ForceSendZDO(id);
	}
}

int IZDOManager::SectorToIndex(ZoneID s) const {
	if (s.x * s.x + s.y * s.y >= IZoneManager::WORLD_RADIUS_IN_ZONES * IZoneManager::WORLD_RADIUS_IN_ZONES)
		return -1;

	int x = s.x + IZoneManager::WORLD_RADIUS_IN_ZONES;
	int y = s.y + IZoneManager::WORLD_RADIUS_IN_ZONES;
	if (x < 0 || y < 0
		|| x >= IZoneManager::WORLD_DIAMETER_IN_ZONES || y >= IZoneManager::WORLD_DIAMETER_IN_ZONES) {
		return -1;
	}

	assert(x >= 0 && y >= 0 && x < IZoneManager::WORLD_DIAMETER_IN_ZONES && y < IZoneManager::WORLD_DIAMETER_IN_ZONES && "sector exceeds world radius");

	return y * IZoneManager::WORLD_DIAMETER_IN_ZONES + x;
}

bool IZDOManager::SendZDOs(Peer& peer, bool flush) {
	auto sendQueueSize = peer.m_socket->GetSendQueueSize();

	// flushing forces a packet send
	const auto threshold = VH_SETTINGS.zdoMaxCongestion;
	if (!flush && sendQueueSize > threshold)
		return false;

	auto availableSpace = threshold - sendQueueSize;
	if (availableSpace < VH_SETTINGS.zdoMinCongestion)
		return false;

	auto syncList = CreateSyncList(peer);

	// continue only if there are updated/invalid NetSyncs to send
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

			writer.Write(zdo.ID());
			writer.Write(zdo.GetOwnerRevision());
			writer.Write(zdo.m_dataRev);
			writer.Write(zdo.Owner());
			writer.Write(zdo.m_pos);

			writer.SubWrite([&zdo](DataWriter& writer) {
				zdo.Serialize(writer);
			});

			peer.m_zdos[zdo.ID()] = { ZDO::Rev{
				.m_dataRev = zdo.m_dataRev,
				.m_ownerRev = zdo.GetOwnerRevision()
			}, time };
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
		reader.AsEach([this](const ZDOID& zdoid) {
			if (auto zdo = GetZDO(zdoid))
				InvalidateZDOZone(*zdo);
			}
		);

		auto time = Valhalla()->Time();

		while (auto zdoid = reader.Read<ZDOID>()) {
			auto ownerRev = reader.Read<uint32_t>();	// owner revision
			auto dataRev = reader.Read<uint32_t>();		// data revision
			auto owner = reader.Read<OWNER_t>();		// owner
			auto pos = reader.Read<Vector3f>();			// position

			auto des = reader.Read<DataReader>();				// dont move this

			auto&& pair = this->GetOrInstantiate(zdoid, pos);

			auto&& zdo = *pair.first->second.get();
			auto&& created = pair.second;

			if (!created) {
				// If the incoming data revision is at most older or equal to this revision, we do NOT need to deserialize
				//	(because the data will be the same, or at the worst case, it will be outdated)
				if (dataRev <= zdo.m_dataRev) {

					// If the owner has changed, keep a copy
					if (ownerRev > zdo.GetOwnerRevision()) {
						zdo._SetOwner(owner);
						zdo.SetOwnerRevision(ownerRev);
						peer->m_zdos[zdoid] = { 
							ZDO::Rev {.m_dataRev = dataRev, .m_ownerRev = ownerRev}, 
							time 
						};
					}
					continue;
				}
			}
			else {
				if (m_erasedZDOs.contains(zdoid)) {
					m_destroySendList.push_back(zdoid);
					m_objectsByID.erase(pair.first);
					continue;
				}
			}



			zdo._SetOwner(owner);
			zdo.m_dataRev = dataRev;
			zdo.SetOwnerRevision(ownerRev);

			// Unpack the ZDOs primary data
			zdo.Deserialize(des);

			// Only disperse through world if ZDO is new
			if (created) {
				AddZDOToZone(zdo);
			}
			else {
				zdo.SetPosition(pos);
			}

			peer->m_zdos[zdoid] = {
				ZDO::Rev{ .m_dataRev = zdo.m_dataRev, .m_ownerRev = zdo.GetOwnerRevision() },
				time 
			};
		}
	}); 
}

void IZDOManager::OnPeerQuit(Peer& peer) {
	for (auto&& itr = m_objectsByID.begin(); itr != m_objectsByID.end(); ) {
		auto&& pair = *itr;

		auto&& zdo = *pair.second.get();
		//auto&& prefab = zdo.GetPrefabHash();
		
		// Apparently peer does unclaim sessioned ZDOs (Player zdo had 0 owner)
		//assert((prefab.FlagsAbsent(Prefab::Flag::SESSIONED) || zdo.HasOwner()) && "Session ZDOs should always be owned");

		// Remove temporary ZDOs belonging to peers (like particles and attack anims, vfx, sfx...)
		//if (prefab.FlagsPresent(Prefab::Flag::SESSIONED)
			//&& (zdo.IsOwner(peer.m_uuid)))
		if (!zdo.m_persistent
			&& (!zdo.HasOwner() || zdo.IsOwner(peer.m_uuid) || !NetManager()->GetPeerByUUID(zdo.Owner())))
		{
			itr = DestroyZDO(itr);
		}
		else
			++itr;
	}
}

void IZDOManager::DestroyZDO(ZDOID zdoid) {
	m_destroySendList.push_back(zdoid);
	EraseZDO(zdoid);
}

decltype(IZDOManager::m_objectsByID)::iterator IZDOManager::DestroyZDO(decltype(IZDOManager::m_objectsByID)::iterator itr) {
	m_destroySendList.push_back(itr->first);
	return EraseZDO(itr);
}



size_t IZDOManager::GetSumZDOMembers() {
	size_t res = 0;
	for (auto&& zdo : m_objectsByID) {
		res += zdo.second->m_members.size();
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
		res += std::pow((float)zdo.second->m_members.size() - mean, 2.f);
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
