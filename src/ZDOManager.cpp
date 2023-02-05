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



void IZDOManager::AddToSector(ZDO* zdo) {
	int num = SectorToIndex(zdo->Sector());
	if (num != -1) {
		m_objectsBySector[num].insert(zdo);
	}
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

	const auto end = pkg.Position();
	pkg.SetPos(start);
	pkg.Write(count);
	pkg.SetPos(end);

	// Write dead zdos
	pkg.Write((int32_t)m_deadZDOs.size());
	for (auto&& dead : m_deadZDOs) {
		pkg.Write(dead.first);
		auto t = dead.second.count();
		static_assert(sizeof(t) == 8);
		pkg.Write(t);
	}
}

void IZDOManager::Init() {
	LOG(INFO) << "Initializing ZDOManager";

	RouteManager()->Register(Hashes::Routed::DestroyZDO, [this](Peer*, BYTES_t bytes) {
		// TODO constraint check
		DataReader reader(bytes);
		auto num = reader.Read<uint32_t>();
		while (num--) {
			auto uid = reader.Read<NetID>();
			HandleDestroyedZDO(uid);
		}
	});
	
	RouteManager()->Register(Hashes::Routed::RequestZDO, [this](Peer* peer, NetID id) {
		peer->ForceSendZDO(id);
	});
}

void IZDOManager::Load(DataReader& reader, int version) {
	reader.Read<OWNER_t>(); // skip server id
	auto nextUid = reader.Read<uint32_t>();
	const auto count = reader.Read<int32_t>();

	LOG(INFO) << "Loading " << count << " zdos, data version:" << version;

	for (int i = 0; i < count; i++) {
		auto zdo = std::make_unique<ZDO>();
		zdo->m_id = reader.Read<NetID>();
		auto zdoBytes = reader.Read<BYTES_t>();

		DataReader zdoReader(zdoBytes);
		zdo->Load(zdoReader, version);

		AddToSector(zdo.get());

		zdo->Abandon();
		if (zdo->ID().m_uuid == SERVER_ID
			&& zdo->ID().m_id >= nextUid)
		{
			nextUid = zdo->ID().m_id + 1;
		}
		m_objectsByID[zdo->ID()] = std::move(zdo);
	}

	auto deadCount = reader.Read<int32_t>();
	for (int j = 0; j < deadCount; j++) {
		auto key = reader.Read<NetID>();
		auto value = TICKS_t(reader.Read<int64_t>());
		m_deadZDOs[key] = value;
		if (key.m_uuid == SERVER_ID && key.m_id >= nextUid) {
			nextUid = key.m_id + 1;
		}
	}
	CapDeadZDOList();
	m_nextUid = nextUid;

	LOG(INFO) << "Loaded " << m_deadZDOs.size() << " dead zdos";
}

void IZDOManager::CapDeadZDOList() {
	while (m_deadZDOs.size() > MAX_DEAD_OBJECTS) {
		m_deadZDOs.erase(m_deadZDOs.begin());
	}
}

ZDO* IZDOManager::CreateZDO(const Vector3& position) {
	NetID zdoid;

	do {
		zdoid = NetID(Valhalla()->ID(), m_nextUid++);
	} while (GetZDO(zdoid));

	return CreateZDO(zdoid, position);
}

ZDO* IZDOManager::CreateZDO(const NetID& uid, const Vector3& position) {
	// See version #2
	// ...returns a pair object whose first element is an iterator 
	//		pointing either to the newly inserted element in the 
	//		container or to the element whose key is equivalent...
	// https://cplusplus.com/reference/unordered_map/unordered_map/insert/
	auto&& ret = m_objectsByID.insert({ uid, std::make_unique<ZDO>(uid, position) })
		.first->second.get();

	AddToSector(ret);

	return ret;
}

void IZDOManager::Update() {
	auto&& peers = NetManager()->GetPeers();

	PERIODIC_NOW(2s, {
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



	if (m_destroySendList.empty())
		return;

	static BYTES_t bytes; bytes.clear();

	DataWriter writer(bytes);
	writer.Write(m_destroySendList);

	m_destroySendList.clear();
	RouteManager()->Invoke(IRouteManager::EVERYBODY, Hashes::Routed::DestroyZDO, bytes);
}



void IZDOManager::ReleaseNearbyZDOS(Peer* peer) {
	auto&& zone = IZoneManager::WorldToZonePos(peer->m_pos);

	std::vector<ZDO*> m_tempNearObjects;
	FindSectorObjects(zone, IZoneManager::NEAR_ACTIVE_AREA, 0, m_tempNearObjects);

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
		m_objectsByID.erase(zdo->ID());
	}

	auto&& peers = NetManager()->GetPeers();
	for (auto&& pair : peers) {
		auto&& peer = pair.second;
		peer->m_zdos.erase(uid);
	}

	m_deadZDOs[uid] = Valhalla()->Ticks(); // ticks;
}

void IZDOManager::SendAllZDOs(Peer* peer) {
	while (SendZDOs(peer, true));
}

void IZDOManager::FindSectorObjects(const ZoneID& sector, int area, int distantArea, std::vector<ZDO*>& sectorObjects, std::vector<ZDO*>* distantSectorObjects) {
	FindObjects(sector, sectorObjects);
	for (int i = 1; i <= area; i++)
	{
		for (int j = sector.x - i; j <= sector.x + i; j++)
		{
			FindObjects(Vector2i(j, sector.y - i), sectorObjects);
			FindObjects(Vector2i(j, sector.y + i), sectorObjects);
		}
		for (int k = sector.y - i + 1; k <= sector.y + i - 1; k++)
		{
			FindObjects(Vector2i(sector.x - i, k), sectorObjects);
			FindObjects(Vector2i(sector.x + i, k), sectorObjects);
		}
	}
	auto&& objects = (distantSectorObjects) ? *distantSectorObjects : sectorObjects;
	for (int l = area + 1; l <= area + distantArea; l++)
	{
		for (int m = sector.x - l; m <= sector.x + l; m++)
		{
			FindDistantObjects(Vector2i(m, sector.y - l), objects);
			FindDistantObjects(Vector2i(m, sector.y + l), objects);
		}
		for (int n = sector.y - l + 1; n <= sector.y + l - 1; n++)
		{
			FindDistantObjects(Vector2i(sector.x - l, n), objects);
			FindDistantObjects(Vector2i(sector.x + l, n), objects);
		}
	}
}

void IZDOManager::FindSectorObjects(const ZoneID &sector, int area, std::vector<ZDO*>& sectorObjects) {
	for (int i = sector.y - area; i <= sector.y + area; i++) {
		for (int j = sector.x - area; j <= sector.x + area; j++) {
			FindObjects(Vector2i(j, i), sectorObjects);
		}
	}
}

void IZDOManager::CreateSyncList(Peer* peer, std::vector<ZDO*>& toSync) {
	Vector3 refPos = peer->m_pos;
	auto zone = IZoneManager::WorldToZonePos(refPos);

	// Gather all updated ZDO's
	std::vector<ZDO*> tempSectorObjects;
	std::vector<ZDO*> m_tempToSyncDistant;
	FindSectorObjects(zone, IZoneManager::NEAR_ACTIVE_AREA, IZoneManager::DISTANT_ACTIVE_AREA,
		tempSectorObjects, &m_tempToSyncDistant);

	// Prepare client-side outdated ZDO's
	for (auto&& zdo : tempSectorObjects) {
		if (peer->IsOutdatedZDO(zdo)) {
			toSync.push_back(zdo);
		}
	}

	// Prioritize ZDO's
	ServerSortSendZDOS(toSync, peer);
	if (toSync.size() < 10) {
		for (auto&& zdo2 : m_tempToSyncDistant) {
			if (peer->IsOutdatedZDO(zdo2)) {
				toSync.push_back(zdo2);
			}
		}
	}

	AddForceSendZDOs(peer, toSync);
}

void IZDOManager::AddForceSendZDOs(Peer* peer, std::vector<ZDO*>& syncList) {
	for (auto&& itr = peer->m_forceSend.begin(); itr != peer->m_forceSend.end();) {
		auto&& zdoid = *itr;
		auto zdo = GetZDO(zdoid);
		if (zdo && peer->IsOutdatedZDO(zdo)) {
			syncList.insert(syncList.begin(), zdo);
			++itr;
		}
		else {
			itr = peer->m_forceSend.erase(itr);
		}
	}	
}

void IZDOManager::ServerSortSendZDOS(std::vector<ZDO*>& objects, Peer* peer) {
	auto uuid = peer->m_uuid;
	auto time = Valhalla()->Time();

	auto&& pos = peer->m_pos;

	auto&& zdos = peer->m_zdos;

	std::sort(objects.begin(), objects.end(), [&zdos, uuid, pos, time](const ZDO* a, const ZDO* b) {

		// Sort in rough order of:
		//	flag -> type/priority -> distance ASC -> age ASC

		// https://www.reddit.com/r/valheim/comments/mga1iw/understanding_the_new_networking_mechanisms_from/

		bool flag = a->Type() == ZDO::ObjectType::Prioritized && a->HasOwner() && a->Owner() != uuid;
		bool flag2 = b->Type() == ZDO::ObjectType::Prioritized && b->HasOwner() && b->Owner() != uuid;

		if (flag == flag2) {
			if (a->Type() == b->Type()) {
				float sub1 = zdos.contains(a->m_id) ? std::clamp(time - a->m_rev.m_time, 0.f, 100.f) * 1.5f
					: 150;
				float sub2 = zdos.contains(b->m_id) ? std::clamp(time - b->m_rev.m_time, 0.f, 100.f) * 1.5f
					: 150;

				//float sub1 = zdos.contains(a->m_id) ? VUtils::Math::Clamp(duration_cast<seconds>(ticks - a->m_rev.m_ticks), 0, 100) * 1.5f 
				//	: 150;
				//float sub2 = zdos.contains(b->m_id) ? VUtils::Math::Clamp(duration_cast<seconds>(ticks - b->m_rev.m_ticks), 0, 100) * 1.5f 
				//	: 150;

				return a->Position().SqDistance(pos) - sub1 <
					b->Position().SqDistance(pos) - sub2;
			}
			else
				return a->Type() < b->Type();
		}
		else {
			return !flag ? true : false;
		}
	});
}

void IZDOManager::FindObjects(const ZoneID& sector, std::vector<ZDO*>& objects) {
	int num = SectorToIndex(sector);
	if (num != -1) {
		auto&& obj = m_objectsBySector[num];
		objects.insert(objects.end(),
			obj.begin(), obj.end());
	}
}

void IZDOManager::FindDistantObjects(const ZoneID& sector, std::vector<ZDO*>& objects) {
	auto num = SectorToIndex(sector);
	if (num != -1) {
		auto&& list = m_objectsBySector[num];

		for (auto&& zdo : list) {
			if (zdo->m_distant)
				objects.push_back(zdo);
		}
	}
}

// this seems to be unused (might have been the earlier method for portal connecting, but the devs realized that
//	iterating a few thousand zdos every frame isnt ideal)
/*
void GetAllZDOsWithPrefab(const std::string& prefab, std::vector<ZDO*> &zdos) {
	int stableHashCode = VUtils::GetStableHashCode(prefab);
	for (auto&& pair: m_objectsByID) {
		auto zdo = pair.second;
		if (zdo->m_prefab == stableHashCode) {
			zdos.push_back(zdo);
		}
	}
}*/

// this is used only to get portal objects in world
// also since portals are treated as zdos (NOT AS PORTALS), they are in the same huge list
// this isnt particularly great, because portals are global and can be loaded at any time
// portals could be stored in separate list for the better
// This is really Unity-specific xue to coroutines and yields
/*
bool IZDOManager::GetAllZDOsWithPrefabIterative(const std::string& prefab, std::vector<ZDO*>& zdos, int& index) {
	auto stableHashCode = VUtils::String::GetStableHashCode(prefab);

	// Search through all sector objects for PREFAB
	if (index >= m_objectsBySector.size()) {
		for (auto&& pair : m_objectsByOutsideSector) {
			auto&& list = pair.second;
			for (auto&& zdo : list) {
				if (zdo->PrefabHash() == stableHashCode) {
					zdos.push_back(zdo);
				}
			}
		}

		for (auto&& it2 = zdos.begin(); it2 != zdos.end();) {
			if (!(*it2)->Valid())
				it2 = zdos.erase(it2);
			else
				++it2;
		}
		return true;
	}

	// search through all 
	for (int found = 0; index < m_objectsBySector.size(); index++) {
		auto&& list2 = m_objectsBySector[index];

		for (auto&& zdo2 : list2) {
			if (zdo2->PrefabHash() == stableHashCode) {
				zdos.push_back(zdo2);
			}
		}

		if (++found > 400) {
			break;
		}
	}
	return false;
}*/


// Global send
void IZDOManager::ForceSendZDO(const NetID& id) {
	for (auto&& pair : NetManager()->GetPeers()) {
		auto&& peer = pair.second;
		peer->ForceSendZDO(id);
	}
}

int IZDOManager::SectorToIndex(const ZoneID& s) const {
	int x = s.x + WIDTH_IN_ZONES / 2;
	int y = s.y + WIDTH_IN_ZONES / 2;
	if (x < 0 || y < 0
		|| x >= WIDTH_IN_ZONES || y >= WIDTH_IN_ZONES) {
		return -1;
	}
	return y * WIDTH_IN_ZONES + x;
}

ZDO* IZDOManager::GetZDO(const NetID& id) {
	if (id) {
		auto&& find = m_objectsByID.find(id);
		if (find != m_objectsByID.end())
			return find->second.get();
	}
	return nullptr;
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

	static std::vector<ZDO*> m_tempToSync; m_tempToSync.clear();
	CreateSyncList(peer, m_tempToSync);

	// continue only if there are updated/invalid NetSyncs to send
	if (m_tempToSync.empty() && peer->m_invalidSector.empty())
		return false;

	static BYTES_t bytes; bytes.clear();
	DataWriter writer(bytes);

	writer.Write(peer->m_invalidSector);

	const auto time = Valhalla()->Time();

	for (int i=0; i < m_tempToSync.size() 
		&& static_cast<uint32_t>(writer.Length()) <= availableSpace; i++)
	{
		auto sync = m_tempToSync[i];
		peer->m_forceSend.erase(sync->ID());

		writer.Write(sync->ID());
		writer.Write(sync->m_rev.m_ownerRev);
		writer.Write(sync->m_rev.m_dataRev);
		writer.Write(sync->Owner());
		writer.Write(sync->Position());

		writer.SubWrite([sync, &writer]() {
			sync->Serialize(writer);
		});

		peer->m_zdos[sync->ID()] = ZDO::Rev {
			.m_dataRev = sync->m_rev.m_dataRev, 
			.m_ownerRev = sync->m_rev.m_ownerRev, 
			.m_time = time
		};
	}
	writer.Write(NetID::NONE); // null terminator

	if (!peer->m_invalidSector.empty() || !m_tempToSync.empty()) {
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
			ZDO* zdo = this->GetZDO(zdoid);				// TODO use only 1 lookup total (use a null insert)

			auto ownerRev = reader.Read<uint32_t>();	// owner revision
			auto dataRev = reader.Read<uint32_t>();		// data revision
			auto owner = reader.Read<OWNER_t>();		// owner
			auto vec3 = reader.Read<Vector3>();			// position

			ZDO::Rev rev = { 
				.m_dataRev = dataRev, 
				.m_ownerRev = ownerRev, 
				.m_time = time 
			};

			// if the zdo already existed (locally/remotely), compare revisions
			bool flagCreated = false;
			if (zdo) {
				// if the client data rev is not new, and they've reassigned the owner:
				if (dataRev <= zdo->m_rev.m_dataRev) {
					if (ownerRev > zdo->m_rev.m_ownerRev) {
						zdo->m_owner = owner;
						zdo->m_rev.m_ownerRev = ownerRev;
						peer->m_zdos[zdoid] = rev;
					}
					continue;
				}
			}
			else {
				zdo = CreateZDO(zdoid, vec3); // 2 lookups is wasteful
				flagCreated = true;
			}

			zdo->m_owner = owner;
			zdo->m_rev = rev;
			zdo->SetPosition(vec3);

			auto des = reader.SubRead();
			zdo->Deserialize(des);

			peer->m_zdos[zdoid] = {
				.m_dataRev = zdo->m_rev.m_dataRev,
				.m_ownerRev = zdo->m_rev.m_ownerRev,
				.m_time = time
			};

			// If the ZDO was just created as a copy, but it was removed recently
			if (flagCreated && m_deadZDOs.contains(zdoid)) {
				zdo->SetLocal();
				m_destroySendList.push_back(zdo->ID());
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
