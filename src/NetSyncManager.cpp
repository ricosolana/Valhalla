#include <array>
#include "NetSyncManager.h"
#include "NetManager.h"
#include "ValhallaServer.h"
#include "NetHashes.h"
#include "ZoneSystem.h"
#include "NetRouteManager.h"
#include "HashUtils.h"

namespace NetSyncManager {

	struct SyncPeer {
		NetPeer* m_peer;
		robin_hood::unordered_map<NetID, NetSync::Rev, HashUtils::Hasher> m_syncs;
		robin_hood::unordered_set<NetID, HashUtils::Hasher> m_forceSend;
		robin_hood::unordered_set<NetID, HashUtils::Hasher> m_invalidSector;
		int m_sendIndex = 0; // used incrementally for which next zdos to send from index

		SyncPeer(NetPeer* peer) : m_peer(peer) {}

		void NetSyncSectorInvalidated(NetSync* netSync) {
			throw std::runtime_error("Not implemented");
			//if (sync->m_owner == m_peer->m_uuid)
			//	return;
			//
			//if (m_syncs.contains(sync->m_uid) 
			//	&& !ZNetScene.instance.InActiveArea(sync->GetSector(), m_peer->m_pos)) {
			//	m_invalidSector.insert(sync.m_uid);
			//	m_syncs.erase(sync->m_uid);
			//}
		}

		void ForceSendNetSync(NetID id) {
			m_forceSend.insert(id);
		}

		// Returns whether the sync is outdated
		//	tests if the peer does not have a copy
		//	tests if the zdo is outdated in owner and data
		bool ShouldSend(NetSync* netSync) {
			auto find = m_syncs.find(netSync->ID());

			//return find == m_syncs.end() 
			//	|| !netSync->Outdated(find->second);

			return find == m_syncs.end()
				|| netSync->m_rev.m_ownerRev > find->second.m_ownerRev
				|| netSync->m_rev.m_dataRev > find->second.m_dataRev;
		}
	};

	// forward declararing

	void ResetSectorArray();
	SyncPeer* GetPeer(OWNER_t uuid);
	SyncPeer* GetPeer(NetPeer *netPeer);
	SyncPeer* GetPeer(NetRpc *rpc);
	void UpdateStats(float dt);
	//void SendZDOToPeers(float dt);
	void SendZDOToPeers2(float dt);
	void FlushClientObjects(); // sends all zdos; the fast that they're labeled as objects here proves the point a better name is SyncObject?
	void ReleaseZDOS(float dt);
	bool IsInPeerActiveArea(const Vector2i& sector, OWNER_t id);
	void ReleaseNearbyZDOS(const Vector3& refPosition, OWNER_t id);
	void SendDestroyed();
	void RPC_DestroyZDO(OWNER_t sender, NetPackage pkg);
	void HandleDestroyedZDO(const NetID& uid);
	void SendAllZDOs(SyncPeer* peer);
	bool SendZDOs(SyncPeer* peer, bool flush);
	void RPC_ZDOData(NetRpc *rpc, NetPackage pkg);
	void CreateSyncList(SyncPeer* peer, std::vector<NetSync*>& toSync);
	void AddForceSendZdos(SyncPeer* peer, std::vector<NetSync*>& syncList);
	//static int ServerSendCompare(NetSync* x, NetSync* y);
	void ServerSortSendZDOS(std::vector<NetSync*>& objects, const Vector3& refPos, SyncPeer* peer);
	//static int ClientSendCompare(NetSync* x, NetSync* y); // used as comparator
	//void ClientSortSendZDOS(std::vector<NetSync*> objects, SyncPeer* peer);
	//void PrintZdoList(std::vector<NetSync*> zdos);
	//void AddDistantObjects(SyncPeer* peer, int maxItems, std::vector<NetSync*> &toSync);
	int SectorToIndex(const Vector2i& s);
	void FindObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void FindDistantObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void RemoveOrphanNonPersistentZDOS();
	bool IsPeerConnected(OWNER_t uid);
	//bool InvalidZDO(NetSync* zdo); // used as comparator
	//std::vector<NetSync> GetSaveClone();
	void RPC_RequestZDO(OWNER_t sender, NetID id);
	//SyncPeer* GetPeer(OWNER_t uid);




	//typedef SyncPeer ZDOPeer;

	static constexpr int SECTOR_WIDTH = 512;
	static constexpr int MAX_DEAD_OBJECTS = 100000;

    // try statically accessing from header
	std::vector<std::unique_ptr<SyncPeer>> m_peers;



    //
    robin_hood::unordered_map<NetID, std::unique_ptr<NetSync>, HashUtils::Hasher> m_objectsByID;    // primary lifetime container

    std::array<robin_hood::unordered_set<NetSync*>, SECTOR_WIDTH*SECTOR_WIDTH> m_objectsBySector;   // a bunch of objects
    robin_hood::unordered_map<Vector2i, robin_hood::unordered_set<NetSync*>, HashUtils::Hasher> m_objectsByOutsideSector;

    robin_hood::unordered_map<NetID, OWNER_t, HashUtils::Hasher> m_deadZDOs;
	std::vector<NetSync*> tempSectorObjects;
	std::vector<NetID> m_destroySendList;

	// this is used incrementally for assigning new revisions and zdos that are created
	uint32_t m_nextUid = 1;
	float m_sendTimer = 0; // redundant ?
	float m_releaseZDOTimer = 0; // redundant ?
	int m_nextSendPeer = -1; // redundant ?

	std::vector<NetSync*> m_tempToSync;
	std::vector<NetSync*> m_tempToSyncDistant;
	std::vector<NetSync*> m_tempNearObjects;
	std::vector<NetID> m_tempRemoveList;

	struct SaveData {
		//OWNER_t m_myid = 0;
		uint32_t m_nextUid = 1;
		std::vector<NetSync> m_zdos;
		robin_hood::unordered_map<NetID, OWNER_t, HashUtils::Hasher> m_deadZDOs;

		// Initialize
		SaveData(uint32_t nextUid)
			: m_nextUid(nextUid) {
			for (int i = 0; i < m_objectsBySector.size(); i++) {
				for (auto zdo : m_objectsBySector[i]) {
					if (zdo->Persists()) {
						m_zdos.push_back(*zdo);
					}
				}
			}

			for (auto&& pair : m_objectsByOutsideSector) {
				for (auto&& zdo : pair.second) {
					if (zdo->Persists())
						m_zdos.push_back(*zdo);
				}
			}
		}

		// Save data to file
		void Save(NetPackage& writer) {
			writer.Write(Valhalla()->ID());
			writer.Write(m_nextUid);
			writer.Write<int32_t>(m_zdos.size());

			NetPackage zpackage;
			for (auto&& zdo : m_zdos) {
				writer.Write(zdo.ID());
				zpackage.m_stream.Clear();
				zdo.Save(zpackage);
				auto&& bytes = zpackage.m_stream.Bytes();
				writer.Write(bytes);
			}
			writer.Write((int32_t)m_deadZDOs.size());
			for (auto&& keyValuePair : m_deadZDOs) {
				writer.Write(keyValuePair.first.m_uuid);
				writer.Write(keyValuePair.first.m_id);
				writer.Write(keyValuePair.second);
			}
			LOG(INFO) << "Saved " << m_zdos.size() << " zdos";
			//this.m_saveData = null;
		}
	};

	std::unique_ptr<SaveData> m_saveData;



















	void ResetSectorArray() {
		for (auto&& sector : m_objectsBySector)
			sector.clear();
		//m_objectsBySector = new List<ZDO>[this.m_width * this.m_width];
		m_objectsByOutsideSector.clear();
	}

	void Stop() {
		//ZDOPool.Release(m_objectsByID);
		m_objectsByID.clear();
		m_tempToSync.clear();
		m_tempToSyncDistant.clear();
		m_tempNearObjects.clear();
		m_tempRemoveList.clear();
		m_peers.clear();
		ResetSectorArray();
		//GC.Collect();
	}

	void PrepareSave() {
		auto now = steady_clock::now();
		m_saveData = std::make_unique<SaveData>(m_nextUid);
		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - now).count();
		LOG(INFO) << "Zdo save took " << elapsed << "ms";

		m_saveData->m_deadZDOs = m_deadZDOs;
	}

	void SaveAsync(NetPackage &writer) {
		m_saveData->Save(writer);
		LOG(INFO) << "Saved " << m_saveData->m_zdos.size() << " zdos";
		m_saveData.reset();
	}

	void Load(NetPackage& reader, int version) {
		//m_saveData->Load(reader, version);

		reader.Read<OWNER_t>(); // skip
		auto num = reader.Read<uint32_t>();
		auto num2 = reader.Read<int32_t>();
		m_objectsByID.clear();
		ResetSectorArray();

		LOG(INFO) << "Loading " << num2 << " zdos, data version:" << version;

		//NetPackage zpackage;
		for (int i = 0; i < num2; i++) {
			//ZDO zdo = ZDOPool.Create(this);
			auto data = reader.Read<BYTES_t>();
			NetPackage zpackage(data);

			auto zdo = std::make_unique<NetSync>(zpackage, version);

			//zdo->Load(zpackage, version);
			zdo->ResetOwner();
			AddToSector(zdo.get(), zdo->Sector());
			if (zdo->ID().m_uuid == Valhalla()->ID()
				&& zdo->ID().m_id >= num)
			{
				num = zdo->ID().m_id + 1U;
			}
			m_objectsByID.insert({ zdo->ID(), std::move(zdo) });
		}
		m_deadZDOs.clear();
		auto num3 = reader.Read<int32_t>();
		for (int j = 0; j < num3; j++) {
			auto key = reader.Read<NetID>();
			auto value = reader.Read<OWNER_t>();
			m_deadZDOs.insert({ key, value });
			if (key.m_uuid == Valhalla()->ID() && key.m_id >= num) {
				num = key.m_id + 1U;
			}
		}
		CapDeadZDOList();
		LOG(INFO) << "Loaded " << m_deadZDOs.size() << " dead zdos";
        ReleaseLegacyZDOS();
		m_nextUid = num;
	}

	void ReleaseLegacyZDOS() {
		int removed = 0;

		for (auto&& itr = m_objectsByID.begin(); itr != m_objectsByID.end();) {
			auto pgw = itr->second->Version();
			if (pgw != 0 && pgw != ZoneSystem::PGW_VERSION) {
				RemoveFromSector(itr->second.get(), itr->second->Sector());
				//ZDOPool.Release(pair.second);
				itr = m_objectsByID.erase(itr);
				removed++;
			}
			else {
				++itr;
			}
		}

		LOG(INFO) << "Removed " << removed << " OLD generated ZDOS";
	}

	void CapDeadZDOList() {
		for (auto&& itr = m_deadZDOs.begin(); itr != m_deadZDOs.end(); ) {
			if (m_deadZDOs.size() > MAX_DEAD_OBJECTS) {
				itr = m_deadZDOs.erase(itr);
			}
		}
	}

	NetSync* CreateNewZDO(const Vector3 &position) {
		NetID zdoid;

		do {
			zdoid = NetID(Valhalla()->ID(), m_nextUid++);
		} while (GetZDO(zdoid));

		return CreateNewZDO(zdoid, position);
	}

	NetSync *CreateNewZDO(const NetID &uid, const Vector3 &position) {
		return m_objectsByID.insert({ uid, std::make_unique<NetSync>() })
			.first->second.get();
	}

	void AddToSector(NetSync *zdo, const Vector2i &sector) {
		int num = SectorToIndex(sector);
		if (num != -1) {
			m_objectsBySector[num].insert(zdo);
		}
		else {
			// inserts an empty vec if not present, then pushes
			m_objectsByOutsideSector[sector].insert(zdo);
		}
	}

	void ZDOSectorInvalidated(NetSync* zdo) {
		for (auto&& zdopeer : m_peers) {
			zdopeer->NetSyncSectorInvalidated(zdo);
		}
	}

	// this is where considering to use a map vs an array conflicts in their pros/cons
	// In Valheim, the world is limited. Unlike Minecraft (mostly), Valheim has an end of the world
	//	An array would be best later on when the world is large and fully explored
	//	A map would be best when the world is small and partial
	// TLDR; use array for late game assumptions
	//		 use map for early game
	// hashmap late game would use a lot of memory and be slower than array
	// array early game would use more memory than is currently needed, but no major performance loss
	// but the devs use an array in the end, so yea...
	void RemoveFromSector(NetSync* zdo, const Vector2i &sector) {
		int num = SectorToIndex(sector);
		//std::vector<NetSync*> list;
		if (num != -1) {
			//m_objectsBySector[found]
			// better to use set for this
			m_objectsBySector[num].erase(zdo);
			sizeof(m_objectsBySector);
			//if (m_objectsBySector[num] != null)
			//	m_objectsBySector[num].Remove(zdo);
		}
		else {
			// 32 bytes:
			//sizeof(std::vector<NetSync*>)
			auto&& find = m_objectsByOutsideSector.find(sector);
			if (find != m_objectsByOutsideSector.end())
				find->second.erase(zdo);
				//find->second.remove(zdo);
				//list.Remove(zdo);
		}
	}

	NetSync *GetZDO(const NetID &id) {
		if (id) {
			auto&& find = m_objectsByID.find(id);
			if (find != m_objectsByID.end())
				return find->second.get();
		}
		return nullptr;
	}

	//void AddPeer(ZNetPeer netPeer) {
	//	ZDOPeer zdopeer = new ZDOMan.ZDOPeer();
	//	zdopeer.m_peer = netPeer;
	//	this.m_peers.Add(zdopeer);
	//	zdopeer.m_peer.m_rpc.Register<ZPackage>("ZDOData", new Action<ZRpc, ZPackage>(this.RPC_ZDOData));
	//}

	void RemovePeer(NetPeer *netPeer) {
		for (auto&& itr = m_peers.begin(); itr != m_peers.end(); ) {
			if (itr->get()->m_peer->m_uuid == netPeer->m_uuid) {
				RemoveOrphanNonPersistentZDOS();
				itr = m_peers.erase(itr);
				return; // DONT remove
			}
			else {
				++itr;
			}
		}
	}

	// private
	//ZDOPeer FindPeer(ZNetPeer netPeer) {
	//	for (auto zdopeer : m_peers) {
	//		if (zdopeer.m_peer == netPeer)
	//		{
	//			return zdopeer;
	//		}
	//	}
	//	return null;
	//}
	//
	//// private
	//ZDOMan.ZDOPeer FindPeer(ZRpc rpc) {
	//	foreach(ZDOMan.ZDOPeer zdopeer in this.m_peers)
	//	{
	//		if (zdopeer.m_peer.m_rpc == rpc)
	//		{
	//			return zdopeer;
	//		}
	//	}
	//	return null;
	//}

	void Update(float dt) {
		ReleaseZDOS(dt);
		
		SendZDOToPeers2(dt);
		SendDestroyed();
		//UpdateStats(dt);
	}

	//void UpdateStats(float dt) {
	//	m_statTimer += dt;
	//	if (m_statTimer >= 1) {
	//		m_statTimer = 0;
	//		m_zdosSentLastSec = this.m_zdosSent;
	//		m_zdosRecvLastSec = this.m_zdosRecv;
	//		m_zdosRecv = 0;
	//		m_zdosSent = 0;
	//	}
	//}

	//private void SendZDOToPeers(float dt)
	//{
	//	this.m_sendTimer += dt;
	//	if (this.m_sendTimer > 0.05f)
	//	{
	//		this.m_sendTimer = 0f;
	//		foreach(ZDOMan.ZDOPeer peer in this.m_peers)
	//		{
	//			this.SendZDOs(peer, false);
	//		}
	//	}
	//}

	void SendZDOToPeers2(float dt) {
		if (m_peers.empty())
			return;

		m_sendTimer += dt;
		if (m_nextSendPeer < 0) {
			if (m_sendTimer > 0.05f) {
				m_nextSendPeer = 0;
				m_sendTimer = 0;
			}
		}
		else {
			if (m_nextSendPeer < m_peers.size())
				SendZDOs(m_peers[m_nextSendPeer].get(), false);

			m_nextSendPeer++;
			if (m_nextSendPeer >= m_peers.size())
				m_nextSendPeer = -1;
		}
	}

	void FlushClientObjects() {
		for (auto&& peer : m_peers)
			SendAllZDOs(peer.get());
	}

	// Token: 0x06000B6C RID: 2924 RVA: 0x00051450 File Offset: 0x0004F650
	void ReleaseZDOS(float dt) {
		m_releaseZDOTimer += dt;
		if (m_releaseZDOTimer > 2) {
			m_releaseZDOTimer = 0;
			//ReleaseNearbyZDOS(NetManager::GetReferencePosition(), this.m_myid);
			for (auto&& zdopeer : m_peers)
				ReleaseNearbyZDOS(zdopeer->m_peer->m_pos, zdopeer->m_peer->m_uuid);
		}
	}

	bool IsInPeerActiveArea(const Vector2i &sector, long uid) {
		assert(uid != Valhalla()->ID() && "Something possibly isnt right");
		//if (uid == Valhalla()->ID())
			//return ZNetScene::InActiveArea(sector, NetManager::GetReferencePosition());

		auto &&peer = NetManager::GetPeer(uid);
		//return peer && ZNetScene::InActiveArea(sector, peer->m_pos);
		throw std::runtime_error("not implemented");
	}

	void ReleaseNearbyZDOS(const Vector3 &refPosition, OWNER_t uid) {
		throw std::runtime_error("not implemented");
		auto&& zone = ZoneSystem::GetZoneCoords(refPosition);
		m_tempNearObjects.clear();
		FindSectorObjects(zone, ZoneSystem::ACTIVE_AREA, 0, m_tempNearObjects, nullptr);
		for (auto&& zdo : m_tempNearObjects) {
			if (zdo->Persists()) {
				if (zdo->Owner() == uid) {
					//if (!ZNetScene::InActiveArea(zdo->Sector(), zone)) {
					//	zdo->ResetOwner();
					//}
				}
				//else if ((!zdo->HasOwner() 
				//	|| !IsInPeerActiveArea(zdo->Sector(), zdo->Owner())) 
				//	&& ZNetScene::InActiveArea(zdo->Sector(), zone)) {
				//	zdo->SetOwner(uid);
				//}
			}
		}
	}

	void DestroyZDO(NetSync *zdo) {
		if (zdo->Local())
			m_destroySendList.push_back(zdo->ID());
	}

	void SendDestroyed() {
		if (m_destroySendList.empty())
			return;

		NetPackage zpackage;
		zpackage.Write((int32_t)m_destroySendList.size());
		for (auto &&id : m_destroySendList)
			zpackage.Write(id);

		m_destroySendList.clear();
		NetRouteManager::Invoke(0, "DestroyZDO", zpackage);
	}

	void RPC_DestroyZDO(OWNER_t sender, NetPackage pkg) {
		int num = pkg.Read<int32_t>();
		for (int i = 0; i < num; i++) {
			auto uid = pkg.Read<NetID>();
			HandleDestroyedZDO(uid);
		}
	}

	void HandleDestroyedZDO(const NetID &uid) {
		if (uid.m_uuid == Valhalla()->ID() && uid.m_id >= m_nextUid)
			m_nextUid = uid.m_uuid + 1;

		auto zdo = GetZDO(uid);
		if (!zdo)
			return;

		//if (m_onZDODestroyed)
		//	m_onZDODestroyed(zdo);

		throw std::runtime_error("not implemented");

		RemoveFromSector(zdo, zdo->Sector());
		m_objectsByID.erase(zdo->ID());
		//ZDOPool.Release(zdo);
		for (auto &&zdopeer : m_peers) {
			zdopeer->m_syncs.erase(uid);
		}

		//long ticks = ZNet.instance.GetTime().Ticks; // ew
		m_deadZDOs[uid] = Valhalla()->Ticks(); // ticks;
	}

	void SendAllZDOs(SyncPeer *peer) {
		while (SendZDOs(peer, true));
	}

	/*
	bool SendZDOs(ZDOMan.ZDOPeer peer, bool flush)
	{
		int sendQueueSize = peer.m_peer.m_socket.GetSendQueueSize();
		if (!flush && sendQueueSize > 10240)
		{
			return false;
		}
		int found = 10240 - sendQueueSize;
		if (found < 2048)
		{
			return false;
		}
		this.m_tempToSync.Clear();
		this.CreateSyncList(peer, this.m_tempToSync);
		if (this.m_tempToSync.Count == 0 && peer.m_invalidSector.Count == 0)
		{
			return false;
		}
		ZPackage zpackage = new ZPackage();
		bool flag = false;
		if (peer.m_invalidSector.Count > 0)
		{
			flag = true;
			zpackage.Write(peer.m_invalidSector.Count);
			foreach(ZDOID id in peer.m_invalidSector)
			{
				zpackage.Write(id);
			}
			peer.m_invalidSector.Clear();
		}
		else
		{
			zpackage.Write(0);
		}
		float time = Time.time;
		ZPackage zpackage2 = new ZPackage();
		bool flag2 = false;
		int num2 = 0;
		while (num2 < this.m_tempToSync.Count && zpackage.Size() <= found)
		{
			ZDO zdo = this.m_tempToSync[num2];
			peer.m_forceSend.Remove(zdo.m_uid);
			if (!ZNet.instance.IsServer())
			{
				this.m_clientChangeQueue.Remove(zdo.m_uid);
			}
			zpackage.Write(zdo.m_uid);
			zpackage.Write(zdo.m_ownerRev);
			zpackage.Write(zdo.m_dataRev);
			zpackage.Write(zdo.m_owner);
			zpackage.Write(zdo.GetPosition());
			zpackage2.Clear();
			zdo.Serialize(zpackage2);
			zpackage.Write(zpackage2);
			peer.m_zdos[zdo.m_uid] = new ZDOMan.ZDOPeer.PeerZDOInfo(zdo.m_dataRev, zdo.m_ownerRev, time);
			flag2 = true;
			this.m_zdosSent++;
			num2++;
		}
		zpackage.Write(ZDOID.None);
		if (flag2 || flag)
		{
			peer.m_peer.m_rpc.Invoke("ZDOData", new object[]
				{
					zpackage
				});
		}
		return flag2 || flag;
	}

	// Token: 0x06000B75 RID: 2933 RVA: 0x00051A54 File Offset: 0x0004FC54
	private void RPC_ZDOData(ZRpc rpc, ZPackage pkg)
	{
		ZDOMan.ZDOPeer zdopeer = this.FindPeer(rpc);
		if (zdopeer == null)
		{
			ZLog.Log("ZDO data from unkown host, ignoring");
			return;
		}
		float time = Time.time;
		int found = 0;
		ZPackage zdoBytes = new ZPackage();
		int num2 = pkg.ReadInt();
		for (int i = 0; i < num2; i++)
		{
			ZDOID id = pkg.ReadZDOID();
			ZDO zdo = this.GetZDO(id);
			if (zdo != null)
			{
				zdo.InvalidateSector();
			}
		}
		for (;;)
		{
			ZDOID zdoid = pkg.ReadZDOID();
			if (zdoid.IsNone())
			{
				break;
			}
			found++;
			uint num3 = pkg.ReadUInt();
			uint num4 = pkg.ReadUInt();
			long owner = pkg.ReadLong();
			Vector3 vector = pkg.ReadVector3();
			pkg.ReadPackage(ref zdoBytes);
			ZDO zdo2 = this.GetZDO(zdoid);
			bool flag = false;
			if (zdo2 != null)
			{
				if (num4 <= zdo2.m_dataRev)
				{
					if (num3 > zdo2.m_ownerRev)
					{
						zdo2.m_owner = owner;
						zdo2.m_ownerRev = num3;
						zdopeer.m_zdos[zdoid] = new ZDOMan.ZDOPeer.PeerZDOInfo(num4, num3, time);
						continue;
					}
					continue;
				}
			}
			else
			{
				zdo2 = this.CreateNewZDO(zdoid, vector);
				flag = true;
			}
			zdo2.m_ownerRev = num3;
			zdo2.m_dataRev = num4;
			zdo2.m_owner = owner;
			zdo2.InternalSetPosition(vector);
			zdopeer.m_zdos[zdoid] = new ZDOMan.ZDOPeer.PeerZDOInfo(zdo2.m_dataRev, zdo2.m_ownerRev, time);
			zdo2.Deserialize(zdoBytes);
			if (ZNet.instance.IsServer() && flag && this.m_deadZDOs.ContainsKey(zdoid))
			{
				zdo2.SetOwner(this.m_myid);
				this.DestroyZDO(zdo2);
			}
		}
		this.m_zdosRecv += found;
	}*/

	void FindSectorObjects(const Vector2i &sector, int area, int distantArea, std::vector<NetSync*> &sectorObjects, std::vector<NetSync*> *distantSectorObjects) {
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

	void FindSectorObjects(Vector2i sector, int area, std::vector<NetSync*> sectorObjects)
	{
		for (int i = sector.y - area; i <= sector.y + area; i++)
		{
			for (int j = sector.x - area; j <= sector.x + area; j++)
			{
				FindObjects(Vector2i(j, i), sectorObjects);
			}
		}
	}

	void CreateSyncList(SyncPeer *peer, std::vector<NetSync*> toSync) {
		Vector3 refPos = peer->m_peer->m_pos;
		auto zone = ZoneSystem::GetZoneCoords(refPos);
		tempSectorObjects.clear();
		m_tempToSyncDistant.clear();

		FindSectorObjects(zone, ZoneSystem::ACTIVE_AREA, ZoneSystem::ACTIVE_DISTANT_AREA, 
			tempSectorObjects, &m_tempToSyncDistant);

		for (auto&& zdo : tempSectorObjects) {
			if (peer->ShouldSend(zdo)) {
				toSync.push_back(zdo);
			}
		}

		ServerSortSendZDOS(toSync, refPos, peer);
		if (toSync.size() < 10) {
			for (auto &&zdo2 : m_tempToSyncDistant) {
				if (peer->ShouldSend(zdo2)) {
					toSync.push_back(zdo2);
				}
			}
		}
		AddForceSendZdos(peer, toSync);
	}

	void AddForceSendZdos(SyncPeer *peer, std::vector<NetSync*> &syncList){
		if (!peer->m_forceSend.empty()) {
			m_tempRemoveList.clear();
			for (auto&& zdoid : peer->m_forceSend) {
				auto zdo = GetZDO(zdoid);
				if (zdo && peer->ShouldSend(zdo))
					syncList.insert(syncList.begin(), zdo);
				else
					m_tempRemoveList.push_back(zdoid);				
			}
			for (auto&& item : m_tempRemoveList) {
				peer->m_forceSend.erase(item);
			}
		}
	}

	/*
	static int ServerSendCompare(ZDO *x, ZDO *y) {
		bool flag = x->m_type == ZDO::ObjectType::Prioritized && x->HasOwner() && x->m_owner != ZDOMan.compareReceiver;
		bool flag2 = y->m_type == ZDO::ObjectType::Prioritized && y->HasOwner() && y->m_owner != ZDOMan.compareReceiver;
		if (flag && flag2)
		{
			//return x
			//return Utils.CompareFloats(x.m_tempSortValue, y.m_tempSortValue);
		}
		if (flag != flag2)
		{
			if (!flag)
			{
				return 1;
			}
			return -1;
		}
		else
		{
			if (x->m_type == y->m_type)

				return Utils.CompareFloats(x.m_tempSortValue, y.m_tempSortValue);

			int type = (int)y.m_type;
			return type.CompareTo((int)x.m_type);
		}
	}*/

	void ServerSortSendZDOS(std::vector<NetSync*> &objects, const Vector3 &refPos, SyncPeer *peer) {
		auto uuid = peer->m_peer->m_uuid;
		//float time = Valhalla()->Time();
		auto ticks = Valhalla()->Ticks();
		std::sort(objects.begin(), objects.end(), [uuid, refPos, ticks] (const NetSync* x, const NetSync* y) {

			// zdos are sorted according to
			// priority -> distance -> age ->
			bool flag = x->Type() == NetSync::ObjectType::Prioritized && x->HasOwner() && x->Owner() != uuid;
			bool flag2 = y->Type() == NetSync::ObjectType::Prioritized && y->HasOwner() && y->Owner() != uuid;

			if (flag == flag2) {
				if (x->Type() == y->Type()) {
					float age1 = MathUtils::Clamp(ticks - x->m_rev.m_time, 0, 100);
					float age2 = MathUtils::Clamp(ticks - y->m_rev.m_time, 0, 100);
					return x->Position().SqDistance(refPos) - age1 <
							y->Position().SqDistance(refPos) - age2;
				}
				else
					return x->Type() < y->Type();
			}
			else
				return !flag ? true : false;
		});
	}

	/*
	static int ClientSendCompare(ZDO x, ZDO y)
	{
		if (x.m_type == y.m_type)
		{
			return Utils.CompareFloats(x.m_tempSortValue, y.m_tempSortValue);
		}
		if (x.m_type == ZDO.ObjectType.Prioritized)
		{
			return -1;
		}
		if (y.m_type == ZDO.ObjectType.Prioritized)
		{
			return 1;
		}
		return Utils.CompareFloats(x.m_tempSortValue, y.m_tempSortValue);
	}

	// Token: 0x06000B7D RID: 2941 RVA: 0x00052270 File Offset: 0x00050470
	private void ClientSortSendZDOS(List<ZDO> objects, ZDOMan.ZDOPeer peer)
	{
		float time = Time.time;
		for (int i = 0; i < objects.Count; i++)
		{
			ZDO zdo = objects[i];
			zdo.m_tempSortValue = 0f;
			float found = 100f;
			ZDOMan.ZDOPeer.PeerZDOInfo peerZDOInfo;
			if (peer.m_zdos.TryGetValue(zdo.m_uid, out peerZDOInfo))
			{
				found = Mathf.Clamp(time - peerZDOInfo.m_syncTime, 0f, 100f);
			}
			zdo.m_tempSortValue -= found * 1.5f;
		}
		objects.Sort(new Comparison<ZDO>(ZDOMan.ClientSendCompare));
	}*/

	/*
	void PrintZdoList(List<ZDO> zdos)
	{
		ZLog.Log("Sync list " + zdos.Count.ToString());
		foreach(ZDO zdo in zdos)
		{
			string text = "";
			int prefab = zdo.GetPrefab();
			if (prefab != 0)
			{
				GameObject prefab2 = ZNetScene.instance.GetPrefab(prefab);
				if (prefab2)
				{
					text = prefab2.name;
				}
			}
			ZLog.Log(string.Concat(new string[]
				{
					"  ",
					zdo.m_uid.ToString(),
					"  ",
					zdo.m_ownerRev.ToString(),
					" prefab:",
					text
				}));
		}
	}*/

	// this doesnt appear to be used at all by Valheim
	/*
	void AddDistantObjects(ZDOPeer *peer, int maxItems, std::vector<ZDO*> &toSync) {
		if (peer->m_sendIndex >= m_objectsByID.size()) {
			peer->m_sendIndex = 0;
		}

		//toSync.insert(toSync.end(), m_objectsByID.begin()+std::min())

		for (int i = peer->m_sendIndex; i < std::min((int)m_objectsByID.size(), maxItems); ++i) {
			toSync.push_back(m_objectsByID[i]);
		}

		//for (auto &&itr = m_objectsByID.begin() + peer->m_sendIndex; 

		// what is the point of performing a stream operation on a non-sequential guaranteed map?
		// no order is ever maintained, so what is the purpose?
		// 
		IEnumerable<KeyValuePair<ZDOID, ZDO>> enumerable = m_objectsByID.Skip(peer.m_sendIndex).Take(maxItems);
		peer.m_sendIndex += maxItems;
		for (auto&& keyValuePair : enumerable)
		{
			toSync.Add(keyValuePair.Value);
		}
	}*/

	/*
	int SectorToIndex(const Vector2i &s)
	{
		int num = s.x + SECTOR_WIDTH/2;
		int num2 = s.y + SECTOR_WIDTH/2;
		if (num < 0 || num2 < 0 || num >= SECTOR_WIDTH || num2 >= SECTOR_WIDTH)
		{
			return -1;
		}
		return num2 * SECTOR_WIDTH + num;
	}*/

	void FindObjects(const Vector2i &sector, std::vector<NetSync*> &objects) {
		int num = SectorToIndex(sector);
		if (num != -1) {
			objects.insert(objects.end(), 
				m_objectsBySector[num].begin(), m_objectsBySector[num].end());
		}
		else {
			auto&& find = m_objectsByOutsideSector.find(sector);
			if (find != m_objectsByOutsideSector.end())
			{
				objects.insert(objects.end(), find->second.begin(), find->second.end());
			}
		}
	}

	void FindDistantObjects(const Vector2i &sector, std::vector<NetSync*> &objects) {
		auto num = SectorToIndex(sector);
		if (num != -1) {
			auto&& list = m_objectsBySector[num];

			for (auto&& zdo : list) {
				if (zdo->Distant())
					objects.push_back(zdo);
			}
		}
		else {
			auto&& find = m_objectsByOutsideSector.find(sector);
			if (find != m_objectsByOutsideSector.end()) {
				for (auto&& zdo : find->second) {
					if (zdo->Distant())
						objects.push_back(zdo);
				}
			}
		}
	}

	void RemoveOrphanNonPersistentZDOS() {
		for (auto&& keyValuePair : m_objectsByID) {
			auto &&value = keyValuePair.second;
			if (!value->Persists()
				&& (!value->HasOwner() || !IsPeerConnected(value->Owner())))
			{
				auto &&uid = value->ID();
				LOG(INFO) << "Destroying abandoned non persistent zdo owner: " << value->Owner();
				value->SetLocal();
				DestroyZDO(value.get());
			}
		}
	}

	bool IsPeerConnected(OWNER_t uid) {
		// kinda dumb below for server?

		// what is the purpose of this
		if (Valhalla()->ID() == uid)
			return true;

		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_uuid == uid)
				return true;
		}
		return false;
	}

	// this seems to be unused (might have been the earlier method for portal connecting, but the devs realized that
	//	iterating a few thousand zdos every frame isnt ideal)
	/*
	void GetAllZDOsWithPrefab(const std::string& prefab, std::vector<ZDO*> &zdos) {
		int stableHashCode = Utils::GetStableHashCode(prefab);
		for (auto&& pair: m_objectsByID) {
			auto zdo = pair.second;
			if (zdo->m_prefab == stableHashCode) {
				zdos.push_back(zdo);
			}
		}
	}*/

	//bool InvalidZDO(ZDO zdo) {
	//	return !zdo.Valid();
	//}

	// this is used only to get portal objects in world
	// also since portals are treated as zdos (NOT AS PORTALS), they are in the same huge list
	// this isnt particularly great, because portals are global and can be loaded at any time
	// portals could be stored in separate list for the better
	bool GetAllZDOsWithPrefabIterative(const std::string& prefab, std::vector<NetSync*> &zdos, int &index) {
		auto stableHashCode = Utils::GetStableHashCode(prefab);

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
			
			for (auto &&it2 = zdos.begin(); it2 != zdos.end();) {
				if (!(*it2)->Valid())
					it2 = zdos.erase(it2);
				else
					++it2;
			}
			return true;
		}

		// search through all 
		for (int found = 0; index < m_objectsBySector.size(); index++) {
			auto &&list2 = m_objectsBySector[index];

			for (auto &&zdo2 : list2) {
				if (zdo2->PrefabHash() == stableHashCode) {
					zdos.push_back(zdo2);
				}
			}

			if (++found > 400) {
				break;
			}			
		}
		return false;
	}

	// tacky name
	int NrOfObjects() {
		return m_objectsByID.size();
	}

	/*
	int GetSentZDOs()
	{
		return m_zdosSentLastSec;
	}

	int GetRecvZDOs()
	{
		return m_zdosRecvLastSec;
	}

	void GetAverageStats(out float sentZdos, out float recvZdos)
	{
		sentZdos = (float)this.m_zdosSentLastSec / 20f;
		recvZdos = (float)this.m_zdosRecvLastSec / 20f;
	}

	// Token: 0x06000B8E RID: 2958 RVA: 0x000529D9 File Offset: 0x00050BD9
	int GetClientChangeQueue()
	{
		return this.m_clientChangeQueue.Count;
	}

	// Token: 0x06000B8F RID: 2959 RVA: 0x000529E6 File Offset: 0x00050BE6
	void RequestZDO(ZDOID id)
	{
		ZRoutedRpc.instance.InvokeRoutedRPC("RequestZDO", new object[]
			{
				id
			});
	}*/

	void RPC_RequestZDO(OWNER_t sender, NetID id) {
		auto&& peer = GetPeer(sender);
		if (peer)
			peer->ForceSendNetSync(id);
	}

	//ZDOPeer *GetPeer(OWNER_t uid) {
	//	for (auto&& zdopeer : m_peers) {
	//		if (zdopeer.m_peer.m_uid == uid)
	//		{
	//			return zdopeer;
	//		}
	//	}
	//	return null;
	//}

	// Global send
	void ForceSendZDO(const NetID&id) {
		for (auto&& zdopeer : m_peers)
			zdopeer->ForceSendNetSync(id);
	}

	// Single target by id
	void ForceSendZDO(OWNER_t peerID, const NetID&id) {
		auto&& peer = GetPeer(peerID);
		if (peer)
			peer->ForceSendNetSync(id);
	}

	// IncreseOwnerRevision
	// used by client only, other usages are mut
	/*
	void ClientChanged(const ZDOID &id){
		m_clientChangeQueue.insert(id);
	}*/






















	int SectorToIndex(const Vector2i &s) {
		int x = s.x + SECTOR_WIDTH / 2;
		int y = s.y + SECTOR_WIDTH / 2;
		if (x < 0 || y < 0 
			|| x >= SECTOR_WIDTH || y >= SECTOR_WIDTH) {
			return -1;
		}
		return y * SECTOR_WIDTH + x;
	}

	NetSync *GetNetSync(const NetID& id) {
		if (id) {
			auto&& find = m_objectsByID.find(id);
			if (find != m_objectsByID.end())
				return find->second.get();
		}
		return nullptr;
	}

	SyncPeer* GetPeer(OWNER_t uuid) {
		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_uuid == uuid)
				return peer.get();
		}
		return nullptr;
	}

	SyncPeer* GetPeer(NetRpc* rpc) {
		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_rpc.get() == rpc)
				return peer.get();
		}
		return nullptr;
	}

	SyncPeer* GetPeer(NetPeer *netpeer) {
		return GetPeer(netpeer->m_uuid);
	}

	//NetSync *CreateNewNetSync(NetID uid, Vector3 position) {
	//	//sync sync = NetSyncPool.Create(this, uid, position);
	//	//sync.m_owner = this.m_myid;
	//	//sync.m_timeCreated = ZNet.instance.GetTime().Ticks;
	//	//this.m_objectsByID.Add(uid, sync);
	//	//return sync;
	//	return nullptr;
	//}

	void RPC_ZDOData(NetRpc* rpc, NetPackage pkg) {
		//throw std::runtime_error("Not implemented");

		auto syncPeer = GetPeer(rpc);
		assert(syncPeer);

		//int found = 0; // NetSyncs to be received
		{
			auto invalid_sector_count = pkg.Read<int32_t>(); // invalid sector count
			while (invalid_sector_count--) {
				auto id = pkg.Read<NetID>();
				auto&& sync = GetNetSync(id);
				if (sync)
					sync->InvalidateSector();
			}
		}

        auto ticks = Valhalla()->Ticks();

        static NetPackage des;

		for (;;) {
			auto syncId = pkg.Read<NetID>(); // uid
			if (!syncId)
				break;

			auto sync = GetNetSync(syncId);

			auto ownerRev = pkg.Read<uint32_t>();	// owner revision
			auto dataRev = pkg.Read<uint32_t>();	// data revision
			auto owner = pkg.Read<OWNER_t>();		// owner
			auto vec3 = pkg.Read<Vector3>();		// position
			//auto zdoBytes = pkg.Read<NetPackage>();	// serialized data
            //des = pkg.Read<NetPackage>();
            pkg.Read(des);
		
			NetSync::Rev rev = { dataRev, ownerRev, ticks };

			// if the zdo already existed (locally/remotely), compare revisions
			bool flagCreated = false;
			if (sync) {
				if (dataRev <= sync->m_rev.m_dataRev) {
					if (ownerRev > sync->m_rev.m_ownerRev) {
						sync->m_rev.m_ownerRev = ownerRev;
						syncPeer->m_syncs.insert({ syncId, rev });
					}
					continue;
				}
			}
			else {
				sync = CreateNewZDO(syncId, vec3);
				flagCreated = true;
			}
			
			sync->SetOwner(owner);
			sync->m_rev = rev;
			sync->SetPosition(vec3);
			sync->Deserialize(des);

			syncPeer->m_syncs[syncId] = rev;
			
			if (flagCreated && m_deadZDOs.contains(syncId)) {
				sync->SetLocal();
				DestroyZDO(sync);
			}
		}
	}

	bool SendZDOs(SyncPeer* peer, bool flush) {
		assert(false);
		int sendQueueSize = peer->m_peer->m_rpc->m_socket->GetSendQueueSize();
		
		// flush is presumably for preventing network lagg
        const auto CONGEST = Valhalla()->Settings().socketMaxCongestion;
		if (!flush && sendQueueSize > CONGEST)
			return false;
		
		// this isnt really available space, more of micro space
		int availableSpace = CONGEST - sendQueueSize;
		if (availableSpace < Valhalla()->Settings().socketMinCongestion)
			return false;

		//m_tempToSync.Clear();
		//CreateSyncList(peer, m_tempToSync);

		// continue only if there are updated/invalid NetSyncs to send
		//if (m_tempToSync.Count == 0 && peer->m_invalidSector.empty())
		//	return false;

		/*
		* NetSyncData packet structure:
		*  - 4 bytes: invalidSectors.size()
		*    - invalidSectors syncId array with size above
		*  - array of sync [
		*		syncId,
		*		owner rev
		*		data rev
		*		owner
		*		sync pos
		*		sync data
		*    ] NetSyncID::null for termination
		*/

		//auto pkg(PKG());
		NetPackage pkg;
		bool NetSyncsWritten = false;

		pkg.Write(static_cast<int32_t>(peer->m_invalidSector.size()));
		if (!peer->m_invalidSector.empty()) {
			for (auto&& id : peer->m_invalidSector) {
				pkg.Write(id);
			}

			peer->m_invalidSector.clear();
			NetSyncsWritten = true;
		}

		//float time = Time.time;
		float time = 0;
		
		//int currentTemp = 0;
		//while (currentTemp < m_tempToSync.Count && pkg->GetStream().Length() <= availableSpace)
		//{
		//	sync *sync = m_tempToSync[currentTemp];
		//	peer->m_forceSend.erase(sync->m_uid);
		//
		//
		//	pkg->Write(sync->m_uid);
		//	pkg->Write(sync->m_ownerRev);
		//	pkg->Write(sync->m_dataRev);
		//	pkg->Write(sync->m_owner);
		//	pkg->Write(sync->GetPosition());
		//	
		//	auto NetSyncpkg(PKG());
		//	sync->Serialize(NetSyncpkg); // dump sync information onto packet
		//	pkg->Write(NetSyncpkg);
		//
		//	peer->m_syncs[sync->m_uid] = SyncPeer::Rev(
		//		sync->m_dataRev, sync->m_ownerRev, time);
		//	
		//	NetSyncsWritten = true;
		//	//m_NetSyncsSent++;
		//	currentTemp++;
		//}
		//pkg->Write(NetSyncID::NONE); // used as the null terminator

		if (NetSyncsWritten)
			peer->m_peer->m_rpc->Invoke(Rpc_Hash::ZDOData, pkg);

		return NetSyncsWritten;
	}

	void OnNewPeer(NetPeer *peer) {
		m_peers.push_back(std::make_unique<SyncPeer>(peer));
		peer->m_rpc->Register(Rpc_Hash::ZDOData, &RPC_ZDOData);
	}
}
