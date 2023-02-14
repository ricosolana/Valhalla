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
	LOG(INFO) << "Initializing ZDOManager";

	RouteManager()->Register(Hashes::Routed::DestroyZDO, [this](Peer*, BYTES_t bytes) {
		// TODO constraint check
		DataReader reader(bytes);
		auto destroyed = reader.Read<std::list<NetID>>();
		for (auto&& uid : destroyed)
			HandleDestroyedZDO(uid);
		});

	RouteManager()->Register(Hashes::Routed::RequestZDO, [this](Peer* peer, NetID id) {
		peer->ForceSendZDO(id);
		});
}

void IZDOManager::Update() {
	auto&& peers = NetManager()->GetPeers();

	// Occasionally release ZDOs
	PERIODIC_NOW(SERVER_SETTINGS.zdoAssignInterval, {
		for (auto&& pair : peers) {
			auto&& peer = pair.second;
			ReleaseNearbyZDOS(peer.get());
		}
	});

	// Send ZDOS:
	PERIODIC_NOW(SERVER_SETTINGS.zdoSendInterval, {
		for (auto&& pair : peers) {
			auto&& peer = pair.second;
			SendZDOs(peer.get(), false);
		}
	});

	PERIODIC_NOW(1min, {
		//size_t bytes = m_objectsByID.calcNumBytesInfo(m_objectsByID.calcNumElementsWithBuffer(m_objectsByID.mask() + 1));
		size_t bytes = m_objectsByID.size() * sizeof(ZDO);
		
		float kb = bytes / 1000.f;
		LOG(INFO) << "Currently " << m_objectsByID.size() << " zdos (~" << kb << "kb)";
	});

	if (m_destroySendList.empty())
		return;

	static BYTES_t bytes; bytes.clear();

	DataWriter writer(bytes);
	writer.Write(m_destroySendList);

	m_destroySendList.clear();
	RouteManager()->Invoke(IRouteManager::EVERYBODY, Hashes::Routed::DestroyZDO, bytes);
}


bool IZDOManager::AddToSector(ZDO* zdo) {
	int num = SectorToIndex(zdo->Sector());
	if (num != -1) {
		auto&& pair = m_objectsBySector[num].insert(zdo);
		assert(pair.second);
		return true;
	}
	return false;
}

void IZDOManager::RemoveFromSector(ZDO* zdo) {
	int num = SectorToIndex(zdo->Sector());
	if (num != -1) {
		m_objectsBySector[num].erase(zdo);
	}
}

void IZDOManager::InvalidateSector(ZDO* zdo) {
	assert(zdo);
	RemoveFromSector(zdo);

	auto&& peers = NetManager()->GetPeers();
	for (auto&& pair : peers) {
		auto&& peer = pair.second;
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
					if (zdo->m_persistent) {
						pkg.Write(zdo->ID());
						pkg.SubWrite([zdo, &pkg]() {
							zdo->Save(pkg);
							});
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

	// Write dead zdos
	//pkg.Write((int32_t)m_deadZDOs.size());
	//for (auto&& dead : m_deadZDOs) {
	//	pkg.Write(dead.first);
	//	auto t = dead.second.count();
	//	static_assert(sizeof(t) == 8);
	//	pkg.Write(t);
	//}
}



void IZDOManager::Load(DataReader& reader, int version) {
	reader.Read<OWNER_t>(); // skip server id
	auto nextUid = reader.Read<uint32_t>();
	const auto count = reader.Read<int32_t>();
	if (count < 0)
		throw std::runtime_error("count must be positive");

	int purgeCount = 0;

	for (int i = 0; i < count; i++) {
		auto zdo = std::make_unique<ZDO>();
		zdo->m_id = reader.Read<NetID>();
		auto zdoBytes = reader.Read<BYTES_t>();

		DataReader zdoReader(zdoBytes);
		bool modern = zdo->Load(zdoReader, version);

		zdo->Abandon();
		if (zdo->ID().m_uuid == SERVER_ID
			&& zdo->ID().m_id >= nextUid)
		{
			nextUid = zdo->ID().m_id + 1;
		}

		if (modern || !SERVER_SETTINGS.worldModern) {
			AddToSector(zdo.get());
			m_objectsByPrefab[zdo->PrefabHash()].insert(zdo.get());
			m_objectsByID[zdo->ID()] = std::move(zdo);
		}
		else purgeCount++;
	}



	auto deadCount = reader.Read<int32_t>();
	for (int j = 0; j < deadCount; j++) {
		auto key = reader.Read<NetID>();
		auto value = TICKS_t(reader.Read<int64_t>());
		//m_deadZDOs[key] = value;
		//if (key.m_uuid == SERVER_ID && key.m_id >= nextUid) {
		//	nextUid = key.m_id + 1;
		//}
	}

	//CapDeadZDOList();

	m_nextUid = nextUid;

	LOG(INFO) << "Loaded " << m_objectsByID.size() << " zdos";
	LOG(INFO) << "Purged " << purgeCount << " old zdos";
	//LOG(INFO) << "Loaded " << m_deadZDOs.size() << " dead zdos";
}

ZDO* IZDOManager::AddZDO(const Vector3& position) {
	NetID zdoid = NetID(Valhalla()->ID(), 0);
	for(;;) {
		zdoid.m_id = m_nextUid++;
		auto&& pair = m_objectsByID.insert({ zdoid, nullptr });
		if (!pair.second) // if insert failed, keep looping
			continue;

		auto&& zdo = pair.first->second;

		zdo = std::make_unique<ZDO>(zdoid, position);
		AddToSector(zdo.get());
		return zdo.get();
	}
}

ZDO* IZDOManager::AddZDO(const NetID& uid, const Vector3& position) {
	// See version #2
	// ...returns a pair object whose first element is an iterator 
	//		pointing either to the newly inserted element in the 
	//		container or to the element whose key is equivalent...
	// https://cplusplus.com/reference/unordered_map/unordered_map/insert/

	auto&& pair = m_objectsByID.insert({ uid, nullptr });
	if (!pair.second) // if insert failed, throw
		throw VUtils::data_error("zdo id already exists");

	auto&& zdo = pair.first->second; zdo = std::make_unique<ZDO>(uid, position);

	AddToSector(zdo.get());
	//m_objectsByPrefab[zdo->PrefabHash()].insert(zdo.get());

	return zdo.get();
}

ZDO* IZDOManager::GetZDO(const NetID& id) {
	if (id) {
		auto&& find = m_objectsByID.find(id);
		if (find != m_objectsByID.end())
			return find->second.get();
	}
	return nullptr;
}

std::pair<ZDO*, bool> IZDOManager::GetOrCreateZDO(const NetID& id, const Vector3& def) {
	auto&& pair = m_objectsByID.insert({ id, nullptr });

	auto&& zdo = pair.first->second;
	if (!pair.second) // if new insert failed, return it
		return { zdo.get(), false };

	zdo = std::make_unique<ZDO>(id, def);
	AddToSector(zdo.get());
	return { zdo.get(), true };
}



void IZDOManager::ReleaseNearbyZDOS(Peer* peer) {
	auto&& zone = IZoneManager::WorldToZonePos(peer->m_pos);

	std::list<ZDO*> m_tempNearObjects;
	GetZDOs_Zone(zone, m_tempNearObjects); // get zdos: zone, nearby
	GetZDOs_NeighborZones(zone, m_tempNearObjects); // get zdos: zone, nearby

	for (auto&& zdo : m_tempNearObjects) {
		if (zdo->m_persistent) {
			if (zdo->Owner() == peer->m_uuid) {
				
				// If owner-peer no longer in area, make it unclaimed
				if (!ZoneManager()->ZonesOverlap(zdo->Sector(), zone)) {
					zdo->Abandon();
				}
			}
			else {
				// If ZDO no longer has owner, or the owner went far away,
				//  Then assign this new peer as owner 
				if (!(zdo->HasOwner() && ZoneManager()->IsInPeerActiveArea(zdo->Sector(), zdo->m_owner))
					&& ZoneManager()->ZonesOverlap(zdo->Sector(), zone)) {
					
					zdo->SetOwner(peer->m_uuid);
				}
			}
		}
	}
}

void IZDOManager::HandleDestroyedZDO(const NetID& uid) {
	if (uid.m_uuid == SERVER_ID && uid.m_id >= m_nextUid)
		m_nextUid = uid.m_uuid + 1;

	{
		auto zdo = GetZDO(uid);
		if (!zdo)
			return;

		RemoveFromSector(zdo);
		auto&& find = m_objectsByPrefab.find(zdo->m_prefab); // .erase(zdo->)
		if (find != m_objectsByPrefab.end()) find->second.erase(zdo);
		m_objectsByID.erase(zdo->ID());
	}

	auto&& peers = NetManager()->GetPeers();
	for (auto&& pair : peers) {
		auto&& peer = pair.second;
		peer->m_zdos.erase(uid);
	}

	m_deadZDOs[uid] = Valhalla()->Ticks();
}

void IZDOManager::SendAllZDOs(Peer* peer) {
	while (SendZDOs(peer, true));
}

void IZDOManager::GetZDOs_ActiveZones(const ZoneID& zone, std::list<ZDO*>& out, std::list<ZDO*>& outDistant) {
	// Add ZDOs from immediate sector
	GetZDOs_Zone(zone, out);

	// Add ZDOs from nearby zones
	GetZDOs_NeighborZones(zone, out);

	// Add ZDOs from distant zones
	GetZDOs_DistantZones(zone, outDistant);
}

void IZDOManager::GetZDOs_NeighborZones(const ZoneID &zone, std::list<ZDO*>& sectorObjects) {
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

void IZDOManager::GetZDOs_DistantZones(const ZoneID& zone, std::list<ZDO*>& out) {
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

std::list<ZDO*> IZDOManager::CreateSyncList(Peer* peer) {
	auto zone = IZoneManager::WorldToZonePos(peer->m_pos);

	// Gather all updated ZDO's
	std::list<ZDO*> tempSectorObjects;
	std::list<ZDO*> m_tempToSyncDistant;
	GetZDOs_ActiveZones(zone, tempSectorObjects, m_tempToSyncDistant);

	std::list<ZDO*> result;

	// Prepare client-side outdated ZDO's
	for (auto&& zdo : tempSectorObjects) {
		if (peer->IsOutdatedZDO(zdo)) {
			result.push_back(zdo);
		}
	}

	// Prioritize ZDO's
	//ServerSortSendZDOS(toSync, peer);
	
	auto time(Valhalla()->Time());

	result.sort([=](const ZDO* a, const ZDO* b) {

		// Sort in rough order of:
		//	flag -> type/priority -> distance ASC -> age ASC

		// https://www.reddit.com/r/valheim/comments/mga1iw/understanding_the_new_networking_mechanisms_from/

		bool flag = a->Type() == ZDO::ObjectType::Prioritized && a->HasOwner() && a->Owner() != peer->m_uuid;
		bool flag2 = b->Type() == ZDO::ObjectType::Prioritized && b->HasOwner() && b->Owner() != peer->m_uuid;

		if (flag == flag2) {
			if (a->Type() == b->Type()) {
				float sub1 = peer->m_zdos.contains(a->m_id) ? std::clamp(time - a->m_rev.m_time, 0.f, 100.f) * 1.5f
					: 150;
				float sub2 = peer->m_zdos.contains(b->m_id) ? std::clamp(time - b->m_rev.m_time, 0.f, 100.f) * 1.5f
					: 150;

				return a->Position().SqDistance(peer->m_pos) - sub1 <
					b->Position().SqDistance(peer->m_pos) - sub2;
			}
			else
				return a->Type() < b->Type();
		}
		else {
			return !flag ? true : false;
		}
	});

	if (result.size() < 10) {
		for (auto&& zdo2 : m_tempToSyncDistant) {
			if (peer->IsOutdatedZDO(zdo2)) {
				result.push_back(zdo2);
			}
		}
	}

	//AddForceSendZDOs(peer, toSync);

	for (auto&& itr = peer->m_forceSend.begin(); itr != peer->m_forceSend.end();) {
		auto&& zdoid = *itr;
		auto zdo = GetZDO(zdoid);
		if (zdo && peer->IsOutdatedZDO(zdo)) {
			result.push_front(zdo);
			++itr;
		}
		else {
			itr = peer->m_forceSend.erase(itr);
		}
	}

	return result;
}

void IZDOManager::GetZDOs_Zone(const ZoneID& sector, std::list<ZDO*>& objects) {
	int num = SectorToIndex(sector);
	if (num != -1) {
		auto&& obj = m_objectsBySector[num];
		objects.insert(objects.end(),
			obj.begin(), obj.end());
	}
}

void IZDOManager::GetZDOs_Distant(const ZoneID& sector, std::list<ZDO*>& objects) {
	auto num = SectorToIndex(sector);
	if (num != -1) {
		auto&& list = m_objectsBySector[num];

		for (auto&& zdo : list) {
			if (zdo->m_distant)
				objects.push_back(zdo);
		}
	}
}

std::list<ZDO*> IZDOManager::GetZDOs_Prefab(HASH_t prefabHash) {
	std::list<ZDO*> out;
	auto&& zdos = m_objectsByPrefab.find(prefabHash);
	if (zdos != m_objectsByPrefab.end())
		out.insert(out.end(),
			zdos->second.begin(),
			zdos->second.end()
		);
	return out;
}

std::list<ZDO*> IZDOManager::GetZDOs_Radius(const Vector3& pos, float radius) {
	std::list<ZDO*> out;

	auto zone = IZoneManager::WorldToZonePos(pos);

	auto minZone = IZoneManager::WorldToZonePos(Vector3(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3(pos.x + radius, 0, pos.z + radius));

	for (auto z=minZone.y; z < maxZone.y; z++) {
		for (auto x = minZone.x; x < maxZone.x; x++) {
			//FindObjects({ x, z }, out);
			int num = SectorToIndex({x, z});
			if (num != -1) {
				auto&& objects = m_objectsBySector[num];
				for (auto&& obj : objects) {
					if (obj->m_position.SqDistance(pos) <= radius * radius)
						out.push_back(obj);
				}
			}
		}
	}

	return out;
}

std::list<ZDO*> IZDOManager::GetZDOs_PrefabRadius(const Vector3& pos, float radius, HASH_t prefabHash) {
	std::list<ZDO*> out;

	auto zone = IZoneManager::WorldToZonePos(pos);

	auto minZone = IZoneManager::WorldToZonePos(Vector3(pos.x - radius, 0, pos.z - radius));
	auto maxZone = IZoneManager::WorldToZonePos(Vector3(pos.x + radius, 0, pos.z + radius));

	for (auto z = minZone.y; z < maxZone.y; z++) {
		for (auto x = minZone.x; x < maxZone.x; x++) {
			//FindObjects({ x, z }, out);
			int num = SectorToIndex({ x, z });
			if (num != -1) {
				auto&& objects = m_objectsBySector[num];
				for (auto&& obj : objects) {
					if (obj->m_prefab == prefabHash 
						&& obj->m_position.SqDistance(pos) <= radius * radius)
						out.push_back(obj);
				}
			}
		}
	}

	return out;
}

// Global send
void IZDOManager::ForceSendZDO(const NetID& id) {
	for (auto&& pair : NetManager()->GetPeers()) {
		auto&& peer = pair.second;
		peer->ForceSendZDO(id);
	}
}

int IZDOManager::SectorToIndex(const ZoneID& s) const {
	if (s.x * s.x + s.y * s.y >= IZoneManager::WORLD_RADIUS_IN_ZONES * IZoneManager::WORLD_RADIUS_IN_ZONES)
		return -1;

	int x = s.x + IZoneManager::WORLD_RADIUS_IN_ZONES;
	int y = s.y + IZoneManager::WORLD_RADIUS_IN_ZONES;
	//if (x < 0 || y < 0
	//	|| x >= IZoneManager::WORLD_DIAMETER_IN_ZONES || y >= IZoneManager::WORLD_DIAMETER_IN_ZONES) {
	//	return -1;
	//}

	assert(x >= 0 && y >= 0 && x < IZoneManager::WORLD_DIAMETER_IN_ZONES&& y < IZoneManager::WORLD_DIAMETER_IN_ZONES && "sector exceeds world radius");

	return y * IZoneManager::WORLD_DIAMETER_IN_ZONES + x;
}

bool IZDOManager::SendZDOs(Peer* peer, bool flush) {
	auto sendQueueSize = peer->m_socket->GetSendQueueSize();

	// flushing forces a packet send
	const auto threshold = SERVER_SETTINGS.zdoMaxCongestion;
	if (!flush && sendQueueSize > threshold)
		return false;

	auto availableSpace = threshold - sendQueueSize;
	if (availableSpace < SERVER_SETTINGS.zdoMinCongestion)
		return false;

	//static std::vector<ZDO*> m_tempToSync; m_tempToSync.clear();

	auto syncList = CreateSyncList(peer);

	// continue only if there are updated/invalid NetSyncs to send
	if (syncList.empty() && peer->m_invalidSector.empty())
		return false;

	static BYTES_t bytes; bytes.clear();
	DataWriter writer(bytes);

	writer.Write(peer->m_invalidSector);

	const auto time = Valhalla()->Time();

	for (auto&& itr = syncList.begin(); 
		itr != syncList.end() && writer.Length() <= availableSpace;
		itr++) {

		auto zdo = *itr;

		peer->m_forceSend.erase(zdo->ID());

		writer.Write(zdo->ID());
		writer.Write(zdo->m_rev.m_ownerRev);
		writer.Write(zdo->m_rev.m_dataRev);
		writer.Write(zdo->Owner());
		writer.Write(zdo->Position());

		writer.SubWrite([zdo, &writer]() {
			zdo->Serialize(writer);
		});

		peer->m_zdos[zdo->ID()] = ZDO::Rev {
			.m_dataRev = zdo->m_rev.m_dataRev,
			.m_ownerRev = zdo->m_rev.m_ownerRev,
			.m_time = time
		};
	}
	writer.Write(NetID::NONE); // null terminator

	if (!peer->m_invalidSector.empty() || !syncList.empty()) {
		peer->Invoke(Hashes::Rpc::ZDOData, bytes);
		peer->m_invalidSector.clear();

		return true;
	}

	return false;
}

void IZDOManager::OnNewPeer(Peer* peer) {
	peer->Register(Hashes::Rpc::ZDOData, [this](Peer* peer, BYTES_t bytes) {
		OPTICK_CATEGORY("RPC_ZDOData", Optick::Category::Network);

		DataReader reader(bytes);

		{
			auto invalidSectors = reader.Read<std::vector<NetID>>();
			for (auto&& id : invalidSectors) {
				ZDO* zdo = GetZDO(id);

				if (zdo) InvalidateSector(zdo);
			}
		}

		auto time = Valhalla()->Time();

		while (auto zdoid = reader.Read<NetID>()) {
			auto ownerRev = reader.Read<uint32_t>();	// owner revision
			auto dataRev = reader.Read<uint32_t>();		// data revision
			auto owner = reader.Read<OWNER_t>();		// owner
			auto pos = reader.Read<Vector3>();			// position

			auto des = reader.SubRead();				// dont move this

			ZDO::Rev rev = { 
				.m_dataRev = dataRev, 
				.m_ownerRev = ownerRev, 
				.m_time = time 
			};

			auto &&pair = this->GetOrCreateZDO(zdoid, pos);

			auto &&zdo = pair.first;
			auto &&created = pair.second;
			if (!created) {
				// if the client data rev is not new, and they've reassigned the owner:
				if (dataRev <= zdo->m_rev.m_dataRev) {
					if (ownerRev > zdo->m_rev.m_ownerRev) {
						//if (ModManager()->CallEvent("ZDOOwnerChange", &copy, zdo) == EventStatus::CANCEL)
							//*zdo = copy; // if zdo modification was to be cancelled

						zdo->m_owner = owner;
						zdo->m_rev.m_ownerRev = ownerRev;
						peer->m_zdos[zdoid] = rev;
					}
					continue;
				}
			}

			// Create a copy of ZDO prior to any modifications
			ZDO copy(*zdo);

			zdo->m_owner = owner;
			zdo->m_rev = rev;

			// Only set position if ZDO has previously existed
			if (!created)
				zdo->SetPosition(pos);

			peer->m_zdos[zdoid] = {
				.m_dataRev = zdo->m_rev.m_dataRev,
				.m_ownerRev = zdo->m_rev.m_ownerRev,
				.m_time = time
			};

			zdo->Deserialize(des);

			if (created) {
				if (m_deadZDOs.contains(zdoid)) {
					zdo->SetLocal();
					m_destroySendList.push_back(zdo->ID());
				}
				else {
					m_objectsByPrefab[zdo->PrefabHash()].insert(zdo);
				}
			}
			else {
				if (ModManager()->CallEvent("ZDOChange", &copy, zdo) == EventStatus::CANCEL)
					*zdo = copy; // if zdo modification was to be cancelled
			}
		}
	});		
}

void IZDOManager::OnPeerQuit(Peer* peer) {
	// This is the kind of iteration removal I am trying to avoid
	for (auto&& pair : m_objectsByID) {
		auto&& zdo = pair.second;
		if (!zdo->m_persistent
			&& (!zdo->HasOwner() || zdo->Owner() == peer->m_uuid))
		{
			auto&& uid = zdo->ID();
			LOG(INFO) << "Destroying abandoned non persistent zdo (" << zdo->m_prefab << " " << zdo->Owner() << ")";
			zdo->SetLocal();
			m_destroySendList.push_back(zdo->ID());
		}
	}
}
