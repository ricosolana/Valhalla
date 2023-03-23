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
	LOG(INFO) << "Initializing ZDOManager";

	RouteManager()->Register(Hashes::Routed::DestroyZDO, 
		[this](Peer*, BYTES_t bytes) {
			// TODO constraint check
			for (auto&& uid : DataReader(bytes).Read<std::list<ZDOID>>())
				EraseZDO(uid);
		}
	);

	RouteManager()->Register(Hashes::Routed::RequestZDO, 
		[this](Peer* peer, ZDOID id) {
			peer->ForceSendZDO(id);
		}
	);
}

void IZDOManager::Update() {
	auto&& peers = NetManager()->GetPeers();

	// Occasionally release ZDOs
	PERIODIC_NOW(SERVER_SETTINGS.zdoAssignInterval, {
		for (auto&& peer : peers) {
			AssignOrReleaseZDOs(*peer.get());
		}
	});

	// Send ZDOS:
	PERIODIC_NOW(SERVER_SETTINGS.zdoSendInterval, {
		for (auto&& peer : peers) {
			SendZDOs(*peer.get(), false);
		}
	});

	PERIODIC_NOW(1min, {
		LOG(INFO) << "Currently " << m_objectsByID.size() << " zdos (~" << (GetTotalZDOAlloc() / 1000000.f) << "Mb)";
		LOG(INFO) << "ZDO members (sum: " << GetSumZDOMembers() 
			<< ", mean: " << GetMeanZDOMembers() 
			<< ", stdev: " << GetStDevZDOMembers() 
			<< ", empty: " << GetCountEmptyZDOs()
			<< ")";
	});

	if (m_destroySendList.empty())
		return;

	static BYTES_t bytes; bytes.clear();

	DataWriter writer(bytes);
	writer.Write(m_destroySendList);

	m_destroySendList.clear();
	RouteManager()->Invoke(IRouteManager::EVERYBODY, Hashes::Routed::DestroyZDO, bytes);
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



void IZDOManager::Save(DataWriter& pkg) {
	//pkg.Write(Valhalla()->ID());
	pkg.Write<OWNER_t>(0);
	pkg.Write(m_nextUid);
	
	{
		// Write zdos (persistent)
		const auto start = pkg.Position();

		int32_t count = 0;
		pkg.Write(count);

		{
			//NetPackage zdoPkg;
			for (auto&& sectorObjects : m_objectsBySector) {
				for (auto zdo : sectorObjects) {
					if (zdo->m_prefab.get().FlagsAbsent(Prefab::Flag::SESSIONED)) {
						pkg.Write(zdo->ID());
						pkg.SubWrite(
							[&]() {
								zdo->Save(pkg);
							}
						);
						count++;
					}
				}
			}
		}

		//const auto end = pkg.Position();
		pkg.SetPos(start);
		pkg.Write(count);
		//pkg.SetPos(end);
		pkg.SetPos(pkg.m_provider.get().size());
	}

	pkg.Write<int32_t>(0);
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
		auto zdoReader = reader.SubRead();

		bool modern = zdo->Load(zdoReader, version);

		// TODO redundant?
		if (zdo->ID().m_uuid == SERVER_ID
			&& zdo->ID().m_id >= m_nextUid)
		{
			assert(false && "looks like this might be significant");
			m_nextUid = zdo->ID().m_id + 1;
		}

		if (modern || !SERVER_SETTINGS.worldModern) {
			auto&& prefab = zdo->GetPrefab();

			AddZDOToZone(*zdo.get());
			m_objectsByPrefab[prefab.m_hash].insert(zdo.get());

			if (prefab.FlagsPresent(Prefab::Flag::DUNGEON)) {
				// Only add real sky dungeon
				if (zdo->Position().y > 4000)
					DungeonManager()->m_dungeonInstances.push_back(zdo->ID());
			}

			m_objectsByID[zdo->ID()] = std::move(zdo);
		}
		else purgeCount++;
	}

	auto deadCount = reader.Read<int32_t>();
	for (int j = 0; j < deadCount; j++) {
		reader.Read<ZDOID>();
		TICKS_t(reader.Read<int64_t>());
	}

	LOG(INFO) << "Loaded " << m_objectsByID.size() << " zdos";
	if (purgeCount)
		LOG(INFO) << "Purged " << purgeCount << " old zdos";
}

ZDO& IZDOManager::Instantiate(const Vector3& position) {
	ZDOID zdoid = ZDOID(Valhalla()->ID(), 0);
	for(;;) {
		zdoid.m_id = m_nextUid++;
		auto&& pair = m_objectsByID.insert({ zdoid, nullptr });
		if (!pair.second) // if insert failed, keep looping
			continue;

		auto&& zdo = pair.first->second;

		zdo = std::make_unique<ZDO>(zdoid, position);
		AddZDOToZone(*zdo.get());
		return *zdo.get();
	}
}

ZDO& IZDOManager::Instantiate(const ZDOID& uid, const Vector3& position) {
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
	//m_objectsByPrefab[zdo->PrefabHash()].insert(zdo.get());

	return *zdo.get();
}

ZDO* IZDOManager::GetZDO(const ZDOID& id) {
	if (id) {
		auto&& find = m_objectsByID.find(id);
		if (find != m_objectsByID.end())
			return find->second.get();
	}
	return nullptr;
}



std::pair<ZDO&, bool> IZDOManager::GetOrInstantiate(const ZDOID& id, const Vector3& def) {
	auto&& pair = m_objectsByID.insert({ id, nullptr });
	
	auto&& zdo = pair.first->second;
	if (!pair.second) // if new insert failed, return it
		return { *zdo.get(), false };

	zdo = std::make_unique<ZDO>(id, def);
	return { *zdo.get(), true };
}



ZDO& IZDOManager::Instantiate(const Prefab& prefab, const Vector3& pos, const Quaternion& rot) {
	auto&& zdo = ZDOManager()->Instantiate(pos);
	zdo.m_rotation = rot;
	zdo.m_prefab = prefab;

	if (prefab.FlagsPresent(Prefab::Flag::SYNC_INITIAL_SCALE))
		zdo.Set("scale", prefab.m_localScale);

	return zdo;
}

ZDO& IZDOManager::Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot, const Prefab** outPrefab) {
	auto&& prefab = PrefabManager()->RequirePrefab(hash);
	if (outPrefab) *outPrefab = &prefab;

	return Instantiate(prefab, pos, rot);
}

ZDO& IZDOManager::Instantiate(const ZDO& zdo) {
	auto&& copy = Instantiate(zdo.m_prefab, zdo.m_pos, zdo.m_rotation);

	const ZDOID temp = copy.m_id; // Copying copies everything (including UID, which MUST be unique for every ZDO)
	copy = zdo;
	copy.m_id = temp;

	return copy;
}



void IZDOManager::AssignOrReleaseZDOs(Peer& peer) {
	auto&& zone = IZoneManager::WorldToZonePos(peer.m_pos);

	std::list<std::reference_wrapper<ZDO>> m_tempNearObjects;
	GetZDOs_Zone(zone, m_tempNearObjects); // get zdos: zone, nearby
	GetZDOs_NeighborZones(zone, m_tempNearObjects); // get zdos: zone, nearby

	for (auto&& ref : m_tempNearObjects) {
		auto&& zdo = ref.get();
		if (zdo.m_prefab.get().FlagsAbsent(Prefab::Flag::SESSIONED)) {
			if (zdo.IsOwner(peer.m_uuid)) {
				
				// If peer no longer in area of zdo, unclaim zdo
				if (!ZoneManager()->ZonesOverlap(zdo.GetZone(), zone)) {
					zdo.Disown();
				}
			}
			else {
				// If ZDO no longer has owner, or the owner went far away,
				//  Then assign this new peer as owner 
				if (!(zdo.HasOwner() && ZoneManager()->IsPeerNearby(zdo.GetZone(), zdo.m_owner))
					&& ZoneManager()->ZonesOverlap(zdo.GetZone(), zone)) {
					
					zdo.SetOwner(peer.m_uuid);
				}
			}
		}
	}

	if (SERVER_SETTINGS.zdoSmartAssign) {

		float minSqDist = std::numeric_limits<float>::max();
		Vector3 closestPos;

		// get the distance to the closest peer
		for (auto&& otherPeer : NetManager()->GetPeers()) {
			if (otherPeer.get() == &peer)
				continue;

			if (!ZoneManager()->IsPeerNearby(IZoneManager::WorldToZonePos(otherPeer->m_pos), peer.m_uuid))
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
				if (zdo.GetPrefab().FlagsAbsent(Prefab::Flag::SESSIONED)
					&& zdo.m_pos.SqDistance(closestPos) > 12 * 12 // Ensure the ZDO is far from the other player
					) {
					zdo.SetOwner(peer.m_uuid);
				}
			}
		}
	}

}

void IZDOManager::EraseZDO(const ZDOID& zdoid) {
	// If id is none, do nothing
	if (!zdoid)
		return;

	if (zdoid.m_uuid == SERVER_ID && zdoid.m_id >= m_nextUid)
		m_nextUid = zdoid.m_uuid + 1;

	{
		auto&& find = m_objectsByID.find(zdoid);
		if (find == m_objectsByID.end())
			return;

		auto&& zdo = find->second;

		RemoveFromSector(*zdo);
		//if (zdo->m_prefab) {
			auto&& pfind = m_objectsByPrefab.find(zdo->GetPrefab().m_hash);
			if (pfind != m_objectsByPrefab.end()) pfind->second.erase(zdo.get());
		//}

		m_objectsByID.erase(find);
	}

	// cleans up some zdos
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->m_zdos.erase(zdoid);
	}

	m_erasedZDOs.insert(zdoid);
}

void IZDOManager::SendAllZDOs(Peer& peer) {
	while (SendZDOs(peer, true));
}

void IZDOManager::GetZDOs_ActiveZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out, std::list<std::reference_wrapper<ZDO>>& outDistant) {
	// Add ZDOs from immediate sector
	GetZDOs_Zone(zone, out);

	// Add ZDOs from nearby zones
	GetZDOs_NeighborZones(zone, out);

	// Add ZDOs from distant zones
	GetZDOs_DistantZones(zone, outDistant);
}

void IZDOManager::GetZDOs_NeighborZones(const ZoneID &zone, std::list<std::reference_wrapper<ZDO>>& sectorObjects) {
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

void IZDOManager::GetZDOs_DistantZones(const ZoneID& zone, std::list<std::reference_wrapper<ZDO>>& out) {
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
				weight = std::min(time - outItr->second.m_syncTime, 100.f) * 1.5f;

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

void IZDOManager::GetZDOs_Zone(const ZoneID& sector, std::list<std::reference_wrapper<ZDO>>& objects) {
	int num = SectorToIndex(sector);
	if (num != -1) {
		auto&& obj = m_objectsBySector[num];
		std::transform(obj.begin(), obj.end(), std::back_inserter(objects), [](ZDO* zdo) { return std::ref(*zdo); });
	}
}

void IZDOManager::GetZDOs_Distant(const ZoneID& sector, std::list<std::reference_wrapper<ZDO>>& objects) {
	auto num = SectorToIndex(sector);
	if (num != -1) {
		auto&& list = m_objectsBySector[num];

		for (auto&& zdo : list) {
			if (zdo->GetPrefab().FlagsPresent(Prefab::Flag::DISTANT))
				objects.push_back(*zdo);
		}
	}
}



std::list<std::reference_wrapper<ZDO>> IZDOManager::GetZDOs(HASH_t prefab) {
	std::list<std::reference_wrapper<ZDO>> out;
	auto&& find = m_objectsByPrefab.find(prefab);
	if (find != m_objectsByPrefab.end()) {
		auto&& zdos = find->second;
		std::transform(zdos.begin(), zdos.end(), std::back_inserter(out), [](ZDO* zdo) { return std::ref(*zdo); });
	}
	return out;
}



std::list<std::reference_wrapper<ZDO>> IZDOManager::SomeZDOs(const Vector3& pos, float radius, size_t max, const std::function<bool(const ZDO&)>& pred) {
	std::list<std::reference_wrapper<ZDO>> out;

	const float sqRadius = radius * radius;

	auto minZone = IZoneManager::WorldToZonePos(Vector3(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3(pos.x + radius, 0, pos.z + radius));

	for (auto z = minZone.y; z <= maxZone.y; z++) {
		for (auto x = minZone.x; x <= maxZone.x; x++) {
			int num = SectorToIndex({ x, z });
			if (num != -1) {
				auto&& objects = m_objectsBySector[num];
				for (auto&& obj : objects) {
					if (obj->m_pos.SqDistance(pos) <= sqRadius
						&& (!pred || pred(*obj)))
					{
						if (max--)
							out.push_back(*obj);
						else
							return out;
					}
				}
			}
		}
	}

	return out;
}

std::list<std::reference_wrapper<ZDO>> IZDOManager::SomeZDOs(const ZoneID& zone, size_t max, const std::function<bool(const ZDO&)>& pred) {
	std::list<std::reference_wrapper<ZDO>> out;

	int num = SectorToIndex(zone);
	if (num != -1) {
		auto&& objects = m_objectsBySector[num];
		for (auto&& obj : objects) {
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



ZDO* IZDOManager::NearestZDO(const Vector3& pos, float radius, const std::function<bool(const ZDO&)>& pred) {
	//std::list<std::reference_wrapper<ZDO>> out;

	const float sqRadius = radius * radius;

	ZDO* zdo = nullptr;
	float minSqDist = std::numeric_limits<float>::max();

	auto minZone = IZoneManager::WorldToZonePos(Vector3(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3(pos.x + radius, 0, pos.z + radius));

	for (auto z = minZone.y; z <= maxZone.y; z++) {
		for (auto x = minZone.x; x <= maxZone.x; x++) {
			int num = SectorToIndex({ x, z });
			if (num != -1) {
				auto&& objects = m_objectsBySector[num];
				for (auto&& obj : objects) {
					float sqDist = obj->m_pos.SqDistance(pos);
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
void IZDOManager::ForceSendZDO(const ZDOID& id) {
	for (auto&& peer : NetManager()->GetPeers()) {
		peer->ForceSendZDO(id);
	}
}

int IZDOManager::SectorToIndex(const ZoneID& s) const {
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
	const auto threshold = SERVER_SETTINGS.zdoMaxCongestion;
	if (!flush && sendQueueSize > threshold)
		return false;

	auto availableSpace = threshold - sendQueueSize;
	if (availableSpace < SERVER_SETTINGS.zdoMinCongestion)
		return false;

	auto syncList = CreateSyncList(peer);

	// continue only if there are updated/invalid NetSyncs to send
	if (syncList.empty() && peer.m_invalidSector.empty())
		return false;

	static BYTES_t bytes; bytes.clear();
	DataWriter writer(bytes);

	writer.Write(peer.m_invalidSector);

	const auto time = Valhalla()->Time();

	for (auto&& itr = syncList.begin(); 
		itr != syncList.end() && writer.Length() <= availableSpace;
		itr++) {

		auto &&zdo = itr->first.get();

		peer.m_forceSend.erase(zdo.ID());

		writer.Write(zdo.m_id);
		writer.Write(zdo.m_rev.m_ownerRev);
		writer.Write(zdo.m_rev.m_dataRev);
		writer.Write(zdo.m_owner);
		writer.Write(zdo.m_pos);

		writer.SubWrite([&]() {
			zdo.Serialize(writer);
		});

		peer.m_zdos[zdo.m_id] = ZDO::Rev {
			.m_dataRev = zdo.m_rev.m_dataRev,
			.m_ownerRev = zdo.m_rev.m_ownerRev,
			.m_syncTime = time
		};
	}
	writer.Write(ZDOID()); // null terminator

	if (!peer.m_invalidSector.empty() || !syncList.empty()) {
		peer.Invoke(Hashes::Rpc::ZDOData, bytes);
		peer.m_invalidSector.clear();

		return true;
	}

	return false;
}

void IZDOManager::OnNewPeer(Peer& peer) {
	peer.Register(Hashes::Rpc::ZDOData, [this](Peer* peer, BYTES_t bytes) {
		OPTICK_CATEGORY("RPC_ZDOData", Optick::Category::Network);

		DataReader reader(bytes);

		{
			reader.AsEach([&](const ZDOID& zdoid) {
				if (auto zdo = GetZDO(zdoid))
					InvalidateZDOZone(*zdo);
				}
			);
		}

		auto time = Valhalla()->Time();

		while (auto zdoid = reader.Read<ZDOID>()) {
			auto ownerRev = reader.Read<uint32_t>();	// owner revision
			auto dataRev = reader.Read<uint32_t>();		// data revision
			auto owner = reader.Read<OWNER_t>();		// owner
			auto pos = reader.Read<Vector3>();			// position

			auto des = reader.SubRead();				// dont move this

			ZDO::Rev rev = { 
				.m_dataRev = dataRev, 
				.m_ownerRev = ownerRev, 
				.m_syncTime = time 
			};
						
			auto&& pair = this->GetOrInstantiate(zdoid, pos);

			auto&& zdo = pair.first;
			auto&& created = pair.second;

			if (!created) {
				// If the incoming data revision is at most older or equal to this revision, we do NOT need to deserialize
				//	(because the data will be the same, or at the worst case, it will be outdated)
				if (dataRev <= zdo.m_rev.m_dataRev) {

					// If the owner has changed, keep a copy
					if (ownerRev > zdo.m_rev.m_ownerRev) {
						//if (ModManager()->CallEvent("ZDOOwnerChange", &copy, zdo) == EventStatus::CANCEL)
							//*zdo = copy; // if zdo modification was to be cancelled

						// ensure that owner change is legal
						//	owner will only change if the ZDO was handed over to another client by the controlling client
						if (!zdo.IsOwner(peer->m_uuid))
							throw std::runtime_error("non-owning peer tried changing ZDO ownership");

						zdo.m_owner = owner;
						zdo.m_rev.m_ownerRev = ownerRev;
						peer->m_zdos[zdoid] = rev;
					}
					continue;
				}
			}

			// Also used as restore point if this ZDO breaks during deserialization
			ZDO copy(zdo);

			try {
				// Create a copy of ZDO prior to any modifications
				zdo.m_owner = owner;
				zdo.m_rev = rev;

				// Only set position if ZDO has previously existed
				if (!created)
					zdo.SetPosition(pos);

				peer->m_zdos[zdoid] = {
					.m_dataRev = zdo.m_rev.m_dataRev,
					.m_ownerRev = zdo.m_rev.m_ownerRev,
					.m_syncTime = time
				};

				// TODO extract deserialize directly here, 
				//	and use to construct ZDOs directly, with a non-null prefab to guarantee safety
				zdo.Deserialize(des);

				if (created) {
					if (m_erasedZDOs.contains(zdoid)) {
						DestroyZDO(zdo, true);
					}
					else {
						// check-test
						//	if ZDO was created (presumably by the client)
						//	zdoid.uuid should match
						if (zdoid.m_uuid != peer->m_uuid || owner != peer->m_uuid)
							throw std::runtime_error("newly created ZDO.owner or uuid does not match peer id");

						AddZDOToZone(zdo);
						m_objectsByPrefab[zdo.GetPrefab().m_hash].insert(&zdo);
					}
				}
				else {
					// maybe too expensive? create another that polls for specific zdos with prefab/flags...
					//if (ModManager()->CallEvent("ZDOChange", &copy, zdo))
						//zdo = copy; // if zdo modification was to be cancelled

					// check-test
					//	theoretical: sessioned ZDOs are pretty much clients-only
					//	if sessioned/temporary ZDO was modified by the client
					//	zdoid.uuid should match (because temps are used only by local-client)
					if (zdo.GetPrefab().FlagsPresent(Prefab::Flag::SESSIONED) 
						&& (zdoid.m_uuid != peer->m_uuid || owner != peer->m_uuid))
							throw std::runtime_error("existing sessioned ZDO.owner or uuid does not match peer id");					
				}
			}
			catch (const std::runtime_error& e) {
				// erase the zdo from map
				if (created) // if the zdo was just created, throw it away
					EraseZDO(zdoid);
				else zdo = copy; // else, restore the ZDO to the prior revision

				// This will kick the malicious peer
				std::rethrow_exception(std::make_exception_ptr(e));
			}
		}
	});		
}

void IZDOManager::OnPeerQuit(Peer& peer) {
	for (auto&& pair : m_objectsByID) {
		auto&& zdo = *pair.second.get();
		auto&& prefab = zdo.GetPrefab();

		// Remove temporary ZDOs belonging to peers (like particles and attack anims, vfx, sfx...)
		if (prefab.FlagsPresent(Prefab::Flag::SESSIONED)
			&& (!zdo.HasOwner() || zdo.IsOwner(peer.m_uuid)))
		{
			LOG(INFO) << "Destroying zdo (" << prefab.m_name << ")";
			DestroyZDO(zdo);
		}
	}
}

void IZDOManager::DestroyZDO(ZDO& zdo, bool immediate) {
	zdo.SetLocal();
	m_destroySendList.push_back(zdo.m_id);
	if (immediate)
		EraseZDO(zdo.m_id);
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
