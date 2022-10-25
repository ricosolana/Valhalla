#include "NetSyncManager.h"
#include "ValhallaServer.h"
#include "NetHashes.h"
#include "ZoneSystem.h"
#include "NetRouteManager.h"

namespace NetSyncManager {

	struct NetSyncPeer {
		NetPeer::Ptr m_peer;
		robin_hood::unordered_map<NetID, NetSync::Rev, HashUtils::Hasher> m_syncs;
		robin_hood::unordered_set<NetID, HashUtils::Hasher> m_forceSend;
		robin_hood::unordered_set<NetID, HashUtils::Hasher> m_invalidSector;
		int m_sendIndex = 0; // used incrementally for which next zdos to send from index

		NetSyncPeer(NetPeer::Ptr peer) : m_peer(peer) {}

		void NetSyncSectorInvalidated(NetSync *netSync) {
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
		bool ShouldSend(NetSync *netSync) {
			auto find = m_syncs.find(netSync->ID());
			return find != m_syncs.end()
				|| netSync->GetRev().m_ownerRev > find->second.m_ownerRev
				|| netSync->GetRev().m_dataRev > find->second.m_dataRev;
			return 0;
		}
	};

	typedef NetSync ZDO;
	typedef NetID ZDOID;
	typedef NetPeer ZNetPeer;
	typedef NetSyncPeer ZDOPeer;

	//robin_hood::unordered_map<UUID_t, std::unique_ptr<NetSyncPeer>> m_peers;
	std::vector<std::unique_ptr<NetSyncPeer>> m_peers;
	robin_hood::unordered_map<NetID, UUID_t, HashUtils::Hasher> m_deadZDOs;

	static constexpr int SECTOR_WIDTH = 512;




	int m_nextSendPeer = -1;

	std::vector<NetSync*> tempSectorObjects;

	static long compareReceiver;




	robin_hood::unordered_map<NetID, NetSync*, HashUtils::Hasher> m_objectsByID;

	// list<zdo>[]
	//std::vector<std::vector<NetSync*>> m_objectsBySector;
	// the usage of array<vec> seems to be well defined and ok
	// there are no conflicting usages for testing for null
	//std::array<std::vector<NetSync*>, SECTOR_WIDTH> m_objectsBySector;
	std::array<robin_hood::unordered_set<NetSync*>, SECTOR_WIDTH> m_objectsBySector;

	//robin_hood::unordered_map<Vector2i, std::vector<NetSync*>> m_objectsByOutsideSector;
	robin_hood::unordered_map<Vector2i, robin_hood::unordered_set<NetSync*>> m_objectsByOutsideSector;

	//std::vector<NetSyncPeer> m_peers;

	static constexpr int m_maxDeadZDOs = 100000;

	//robin_hood::unordered_map<NetID, int64_t> m_deadZDOs;

	std::vector<NetID> m_destroySendList;

	robin_hood::unordered_set<NetID> m_clientChangeQueue;

	//UUID_t m_myid; // effectively const on server start

	uint32_t m_nextUid = 1;

	//int32_t m_width;

	//int32_t m_halfWidth;

	float m_sendTimer;

	static constexpr float m_sendFPS = 20;

	float m_releaseZDOTimer;

	//static ZDOMan m_instance;

	int m_zdosSent;

	int m_zdosRecv;

	int m_zdosSentLastSec;

	int m_zdosRecvLastSec;

	float m_statTimer;

	std::vector<NetSync*> m_tempToSync;

	std::vector<NetSync*> m_tempToSyncDistant;

	std::vector<NetSync*> m_tempNearObjects;

	std::vector<NetID> m_tempRemoveList;

	struct SaveData {
		//UUID_t m_myid = 0;
		uint32_t m_nextUid = 1;
		std::vector<NetSync> m_zdos;
		robin_hood::unordered_map<NetID, UUID_t, HashUtils::Hasher> m_deadZDOs;

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
			writer.Write(Valhalla()->Uuid());
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
			writer.Write<int32_t>(m_deadZDOs.size());
			for (auto&& keyValuePair : m_deadZDOs) {
				writer.Write(keyValuePair.first.m_uuid);
				writer.Write(keyValuePair.first.m_id);
				writer.Write(keyValuePair.second);
			}
			LOG(INFO) << "Saved " << m_zdos.size() << " zdos";
			//this.m_saveData = null;
		}

		// Load data from file
		void Load(NetPackage& reader, int version) {
			reader.Read<UUID_t>(); // skip
			auto num = reader.Read<uint32_t>();
			auto num2 = reader.Read<int32_t>();
			//ZDOPool.Release(m_objectsByID);
			m_objectsByID.clear();
			ResetSectorArray();
			LOG(INFO) << "Loading " << num2 << " zdos , my id " << m_myid << " data version:" << version;

			NetPackage zpackage;
			for (int i = 0; i < num2; i++) {
				//ZDO zdo = ZDOPool.Create(this);
				ZDO zdo;
				//zdo.m_uid = ZDOID(reader);
				zdo
				int count = reader.ReadInt32();
				byte[] data = reader.ReadBytes(count);
				zpackage.Load(data);
				zdo.Load(zpackage, version);
				zdo.SetOwner(0L);
				m_objectsByID.insert({ zdo.m_uid, zdo });
				AddToSector(zdo, zdo.GetSector());
				if (zdo.m_uid.userID == m_myid && zdo.m_uid.id >= num)
				{
					num = zdo.m_uid.id + 1U;
				}
			}
			m_deadZDOs.clear();
			int num3 = reader.Read<int32_t>();
			for (int j = 0; j < num3; j++) {
				auto key = reader.Read<NetID>();
				long value = reader.Read<UUID_t>();
				m_deadZDOs.insert({ key, value });
				if (key.m_uuid == m_myid && key.m_id >= num) {
					num = key.m_id + 1U;
				}
			}
			CapDeadZDOList();
			LOG(INFO) << "Loaded " << m_deadZDOs.size() << " dead zdos";
			RemoveOldGeneratedZDOS();
			m_nextUid = num;
		}
	};

	std::unique_ptr<SaveData> m_saveData;






	// forward declararing privates

	void ResetSectorArray();
	//NetSyncPeer* FindPeer(NetPeer::Ptr netPeer);
	//NetSyncPeer* FindPeer(NetRpc *rpc);
	void UpdateStats(float dt);
	//void SendZDOToPeers(float dt);
	void SendZDOToPeers2(float dt);
	void FlushClientObjects();
	void ReleaseZDOS(float dt);
	bool IsInPeerActiveArea(const Vector2i &sector, UUID_t id);
	void ReleaseNearbyZDOS(const Vector3 &refPosition, UUID_t id);
	void SendDestroyed();
	void RPC_DestroyZDO(UUID_t sender, NetPackage pkg);
	void HandleDestroyedZDO(const NetID &uid);
	void SendAllZDOs(NetSyncPeer* peer);
	bool SendZDOs(NetSyncPeer* peer, bool flush);
	void RPC_ZDOData(NetRpc *rpc, NetPackage pkg);
	void CreateSyncList(NetSyncPeer* peer, std::vector<NetSync*> &toSync);
	void AddForceSendZdos(NetSyncPeer* peer, std::vector<NetSync*> &syncList);
	static int ServerSendCompare(NetSync* x, NetSync* y);
	void ServerSortSendZDOS(std::vector<NetSync*> &objects, const Vector3 &refPos, NetSyncPeer* peer);
	//static int ClientSendCompare(NetSync* x, NetSync* y); // used as comparator
	//void ClientSortSendZDOS(std::vector<NetSync*> objects, NetSyncPeer* peer);
	//void PrintZdoList(std::vector<NetSync*> zdos);
	void AddDistantObjects(NetSyncPeer* peer, int maxItems, std::vector<NetSync*> &toSync);
	//int SectorToIndex(const Vector2i &s);
	void FindObjects(const Vector2i &sector, std::vector<NetSync*> &objects);
	void FindDistantObjects(const Vector2i &sector, std::vector<NetSync*> &objects);
	void RemoveOrphanNonPersistentZDOS();
	bool IsPeerConnected(UUID_t uid);
	//bool InvalidZDO(NetSync* zdo); // used as comparator
	std::vector<NetSync> GetSaveClone();
	void RPC_RequestZDO(UUID_t sender, NetID id);
	//NetSyncPeer* GetPeer(UUID_t uid);














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
		//ZDOPool.Release(m_objectsByID);
		m_objectsByID.clear();
		ResetSectorArray();
		m_saveData->Load(reader, version);
		//LOG(INFO) << "Loading " << num2 << " zdos , my id " << m_myid << " data version:" << version;

		CapDeadZDOList();
		LOG(INFO) << "Loaded " << m_deadZDOs.size() << " dead zdos";
		RemoveOldGeneratedZDOS();
		m_nextUid = num;
	}

	void RemoveOldGeneratedZDOS() {
		for (auto&& itr = m_objectsByID.begin(); itr != m_objectsByID.end();) {
			auto pgw = itr->second->Version();
			if (pgw != 0 && pgw != ZoneSystem::PGW_VERSION) {
				RemoveFromSector(itr->second, itr->second->Sector());
				//ZDOPool.Release(pair.second);
				itr = m_objectsByID.erase(itr);
			}
			else {
				++itr;
			}
		}

		LOG(INFO) << "Removed " << list.size() << " OLD generated ZDOS";
	}

	void CapDeadZDOList() {
		for (auto&& itr = m_deadZDOs.begin(); itr != m_deadZDOs.end(); ) {
			if (m_deadZDOs.size() > m_maxDeadZDOs) {
				itr = m_deadZDOs.erase(itr);
			}
		}
	}

	NetSync* CreateNewZDO(const Vector3 &position) {
		//auto myid = m_myid;
		uint32_t nextUid = m_nextUid;
		m_nextUid = nextUid + 1U;
		auto zdoid = NetID(Valhalla()->Uuid(), nextUid);

		// this is for rare edge cases where a collision occurs
		while (GetZDO(zdoid)) {
			nextUid = m_nextUid++;
			zdoid = ZDOID(Valhalla()->Uuid(), nextUid);
		}
		return CreateNewZDO(zdoid, position);
	}

	NetSync *CreateNewZDO(const NetID &uid, const Vector3 &position) {
		return m_objectsByID.insert({ uid, std::make_unique<ZDO>() })
			.first->second;
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

	ZDO *GetZDO(const ZDOID &id) {
		if (id) {
			auto&& find = m_objectsByID.find(id);
			if (find != m_objectsByID.end())
				return find->second;
		}
		return nullptr;
	}

	//void AddPeer(ZNetPeer netPeer) {
	//	ZDOPeer zdopeer = new ZDOMan.ZDOPeer();
	//	zdopeer.m_peer = netPeer;
	//	this.m_peers.Add(zdopeer);
	//	zdopeer.m_peer.m_rpc.Register<ZPackage>("ZDOData", new Action<ZRpc, ZPackage>(this.RPC_ZDOData));
	//}

	void RemovePeer(ZNetPeer::Ptr netPeer) {
		for (auto&& itr = m_peers.begin(); itr != m_peers.end(); ) {
			if (itr->get()->m_peer->m_uuid == netPeer->m_uuid) {
				RemoveOrphanNonPersistentZDOS();
				itr = m_peers.erase(itr);
				return;
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
		//if (uid == this.m_myid)
			//return ZNetScene.instance.InActiveArea(sector, ZNet.instance.GetReferencePosition());

		auto &&peer = NetManager::GetPeer(uid);
		return peer && ZNetScene::InActiveArea(sector, peer->m_pos);
	}

	void ReleaseNearbyZDOS(const Vector3 &refPosition, UUID_t uid) {
		auto&& zone = ZoneSystem::GetZoneCoords(refPosition);
		m_tempNearObjects.clear();
		FindSectorObjects(zone, ZoneSystem::ACTIVE_AREA, 0, m_tempNearObjects, nullptr);
		for (auto&& zdo : m_tempNearObjects) {
			if (zdo->Persists()) {
				if (zdo.m_owner == uid) {
					if (!ZNetScene::InActiveArea(zdo.GetSector(), zone))
					{
						zdo->SetOwner(0L);
					}
				}
				else if ((!zdo->HasOwner() 
					|| !IsInPeerActiveArea(zdo->Sector(), zdo->Owner())) 
					&& ZNetScene::InActiveArea(zdo->Sector(), zone))
				{
					zdo->SetOwner(uid);
				}
			}
		}
	}

	void DestroyZDO(ZDO *zdo) {
		if (zdo->Local())
			m_destroySendList.push_back(zdo->m_id);
	}

	void SendDestroyed() {
		if (m_destroySendList.empty())
			return;

		NetPackage zpackage;
		zpackage.Write<int32_t>(m_destroySendList.size());
		for (auto &&id : m_destroySendList)
			zpackage.Write(id);

		m_destroySendList.clear();
		NetRpcManager::Invoke(0, "DestroyZDO", zpackage);
	}

	void RPC_DestroyZDO(UUID_t sender, NetPackage pkg) {
		int num = pkg.Read<int32_t>();
		for (int i = 0; i < num; i++) {
			ZDOID uid = pkg.Read<ZDOID>();
			HandleDestroyedZDO(uid);
		}
	}

	void HandleDestroyedZDO(const ZDOID &uid) {
		if (uid.m_uuid == Valhalla()->Uuid() && uid.m_id >= m_nextUid)
			m_nextUid = uid.m_uuid + 1;

		auto zdo = GetZDO(uid);
		if (!zdo)
			return;

		if (m_onZDODestroyed)
			m_onZDODestroyed(zdo);

		RemoveFromSector(zdo, zdo->Sector());
		m_objectsByID.erase(zdo.m_uid);
		ZDOPool.Release(zdo);
		for (auto &&zdopeer : m_peers)
		{
			zdopeer.m_zdos.Remove(uid);
		}

		long ticks = ZNet.instance.GetTime().Ticks; // ew
		m_deadZDOs[uid] = ticks;		
	}

	void SendAllZDOs(ZDOPeer *peer) {
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
		ZPackage pkg2 = new ZPackage();
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
			pkg.ReadPackage(ref pkg2);
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
			zdo2.Deserialize(pkg2);
			if (ZNet.instance.IsServer() && flag && this.m_deadZDOs.ContainsKey(zdoid))
			{
				zdo2.SetOwner(this.m_myid);
				this.DestroyZDO(zdo2);
			}
		}
		this.m_zdosRecv += found;
	}*/

	void FindSectorObjects(const Vector2i &sector, int area, int distantArea, std::vector<ZDO*> &sectorObjects, std::vector<ZDO*> *distantSectorObjects) {
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

	void FindSectorObjects(Vector2i sector, int area, std::vector<ZDO*> sectorObjects)
	{
		for (int i = sector.y - area; i <= sector.y + area; i++)
		{
			for (int j = sector.x - area; j <= sector.x + area; j++)
			{
				FindObjects(Vector2i(j, i), sectorObjects);
			}
		}
	}

	void CreateSyncList(ZDOPeer *peer, std::vector<ZDO*> toSync) {
		Vector3 refPos = peer->m_peer->m_pos;
		auto zone = ZoneSystem::GetZoneCoords(refPos);
		tempSectorObjects.clear();
		m_tempToSyncDistant.clear();
		FindSectorObjects(zone, ZoneSystem::ACTIVE_AREA, ZoneSystem::ACTIVE_DISTANT_AREA, tempSectorObjects, m_tempToSyncDistant);
		for (auto&& zdo : tempSectorObjects) {
			if (peer->ShouldSend(zdo))
			{
				toSync.push_back(zdo);
			}
		}
		ServerSortSendZDOS(toSync, refPos, peer);
		if (toSync.size() < 10) {
			for (auto &&zdo2 : m_tempToSyncDistant) {
				if (peer->ShouldSend(zdo2))
				{
					toSync.push_back(zdo2);
				}
			}
		}
		AddForceSendZdos(peer, toSync);
	}

	void AddForceSendZdos(ZDOPeer *peer, std::vector<ZDO*> syncList){
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

	void ServerSortSendZDOS(std::vector<ZDO*> &objects, const Vector3 &refPos, ZDOPeer *peer) {
		auto uuid = peer->m_peer->m_uuid;
		float time = Valhalla()->Time();
		std::sort(objects.begin(), objects.end(), [uuid, refPos, time] (const ZDO* x, const ZDO* y) {

			// zdos are sorted according to
			// priority -> distance -> age ->

			bool flag = x->m_type == ZDO::ObjectType::Prioritized && x->HasOwner() && x->m_owner != uuid;
			bool flag2 = y->m_type == ZDO::ObjectType::Prioritized && y->HasOwner() && y->m_owner != uuid;

			if (flag == flag2) {
				if (x->m_type == y->m_type) {
					float age1 = MathUtils::Clamp(time - x->m_timeCreated, 0, 100);
					float age2 = MathUtils::Clamp(time - y->m_timeCreated, 0, 100);
					return x->m_position.SqDistance(refPos) - age1 < 
							y->m_position.SqDistance(refPos) - age2;
				}
				else
					return x->m_type < y->m_type;
			}
			else
				return !flag ? true : false;
		});

		//compareReceiver = peer->m_peer->m_uuid;
		//std::sort(objects.begin(), objects.end(), ServerSendCompare);
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
		IEnumerable<KeyValuePair<ZDOID, ZDO>> enumerable = m_objectsByID.Skip(peer.m_sendIndex).Take(maxItems);
		peer.m_sendIndex += maxItems;
		for (auto&& keyValuePair : enumerable)
		{
			toSync.Add(keyValuePair.Value);
		}
	}

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

	void FindObjects(const Vector2i &sector, std::vector<ZDO*> &objects) {
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

	void FindDistantObjects(const Vector2i &sector, std::vector<ZDO*> &objects) {
		auto num = SectorToIndex(sector);
		if (num >= 0) {
			auto&& list = m_objectsBySector[num];

			for (auto&& zdo : list) {
				if (zdo->m_distant)
					objects.push_back(zdo);
			}
		}
		else {
			auto&& find = m_objectsByOutsideSector.find(sector);
			if (find != m_objectsByOutsideSector.end()) {
				for (auto&& zdo : find->second) {
					if (zdo->m_distant)
						objects.push_back(zdo);
				}
			}
		}
	}

	void RemoveOrphanNonPersistentZDOS() {
		for (auto&& keyValuePair : m_objectsByID) {
			auto value = keyValuePair.second;
			if (!value->m_persistent 
				&& (!value->HasOwner() || !IsPeerConnected(value->m_owner)))
			{
				//auto &&uid = value->m_id;
				//LOG(INFO) << "Destroying abandoned non persistent zdo " << uid << " owner " << value->m_owner;
				value->SetOwner(m_myid);
				DestroyZDO(value);
			}
		}
	}

	bool IsPeerConnected(UUID_t uid) {
		// kinda dumb below for server?
		if (m_myid == uid)
			return true;

		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_uuid == uid)
				return true;
		}
		return false;
	}

	void GetAllZDOsWithPrefab(std::string prefab, std::vector<ZDO*> &zdos) {
		int stableHashCode = Utils::GetStableHashCode(prefab);
		for (auto&& pair: m_objectsByID) {
			auto zdo = pair.second;
			if (zdo->m_prefab == stableHashCode) {
				zdos.push_back(zdo);
			}
		}
	}

	//bool InvalidZDO(ZDO zdo) {
	//	return !zdo.Valid();
	//}

	// Token: 0x06000B88 RID: 2952 RVA: 0x00052748 File Offset: 0x00050948
	bool GetAllZDOsWithPrefabIterative(std::string prefab, std::vector<ZDO*> &zdos, int &index) {
		int stableHashCode = Utils::GetStableHashCode(prefab);
		if (index >= m_objectsBySector.size()) {
			for (auto&& pair : m_objectsByOutsideSector) {
				auto&& list = pair.second;
				for (auto&& zdo : list) {
					if (zdo->m_prefab == stableHashCode) {
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

		for (int found = 0; index < m_objectsBySector.size(); index++) {
			auto &&list2 = m_objectsBySector[index];

			for (auto &&zdo2 : list2) {
				if (zdo2->m_prefab == stableHashCode) {
					zdos.push_back(zdo2);
				}
			}

			if (++found > 400) {
				break;
			}			
		}
		return false;
	}

	int NrOfObjects()
	{
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

	void RPC_RequestZDO(UUID_t sender, ZDOID id) {
		auto&& peer = GetPeer(sender);
		if (peer)
			peer->ForceSendNetSync(id);
	}

	//ZDOPeer *GetPeer(UUID_t uid) {
	//	for (auto&& zdopeer : m_peers) {
	//		if (zdopeer.m_peer.m_uid == uid)
	//		{
	//			return zdopeer;
	//		}
	//	}
	//	return null;
	//}

	void ForceSendZDO(const ZDOID &id) {
		for (auto&& zdopeer : m_peers)
			zdopeer->ForceSendNetSync(id);
	}

	void ForceSendZDO(long peerID, const ZDOID &id) {
		auto&& peer = GetPeer(peerID);
		if (peer)
			peer->ForceSendNetSync(id);
	}

	void ClientChanged(const ZDOID &id){
		m_clientChangeQueue.insert(id);
	}






















	int SectorToIndex(Vector2i s) {
		int x = s.x + SECTOR_WIDTH / 2;
		int y = s.y + SECTOR_WIDTH / 2;
		if (x < 0 || y < 0 
			|| x >= SECTOR_WIDTH || y >= SECTOR_WIDTH) {
			return -1;
		}
		return y * SECTOR_WIDTH + x;
	}

	NetSync *GetNetSync(NetID& id) {
		if (!id) {
			return nullptr;
		}
		////auto &&find = m_objec
		//sync result;
		//if (this.m_objectsByID.TryGetValue(id, out result))
		//{
		//	return result;
		//}
		//return null;

		

		return nullptr;
	}

	NetSyncPeer* GetPeer(UUID_t uuid) {
		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_uuid == uuid)
				return peer.get();
		}
		return nullptr;
	}

	NetSyncPeer* GetPeer(NetRpc* rpc) {
		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_rpc.get() == rpc)
				return peer.get();
		}
		return nullptr;
	}

	NetSyncPeer* GetPeer(NetPeer::Ptr netpeer) {
		return GetPeer(netpeer->m_uuid);
	}

	NetSync *CreateNewNetSync(NetID uid, Vector3 position) {
		//sync sync = NetSyncPool.Create(this, uid, position);
		//sync.m_owner = this.m_myid;
		//sync.m_timeCreated = ZNet.instance.GetTime().Ticks;
		//this.m_objectsByID.Add(uid, sync);
		//return sync;
		return nullptr;
	}

	void RPC_NetSyncData(NetRpc* rpc, NetPackage pkg) {
		throw std::runtime_error("Not implemented");

		auto syncPeer = GetPeer(rpc);
		assert(syncPeer);

		float time = 0; //Time.time;
		//int found = 0; // NetSyncs to be received
		auto invalid_sector_count = pkg.Read<int32_t>(); // invalid sector count
		for (int i = 0; i < invalid_sector_count; i++)
		{
			auto id = pkg.Read<NetID>();
			//sync sync = this.GetNetSync(id);
			//if (sync != null)
			//{
			//	sync.InvalidateSector();
			//}
		}

		int recv = 0;
		for (;;)
		{
			auto syncId = pkg.Read<NetID>(); // uid
			if (!syncId)
				break;

			recv++;

			auto sync = GetNetSync(syncId);

			auto ownerRevision = pkg.Read<uint32_t>(); // owner revision
			auto dataRevision = pkg.Read<uint32_t>(); // data revision
			auto owner = pkg.Read<UUID_t>(); // owner
			auto vec3 = pkg.Read<Vector3>(); // position
			auto pkg2 = pkg.Read<NetPackage>(); //
			
			bool flagCreated = false;
			if (sync) {
				if (dataRevision <= sync->m_dataRev) {
					if (ownerRevision > sync->m_ownerRev) {
						//sync->m_owner = owner;
						sync->m_ownerRev = ownerRevision;
						syncPeer->m_syncs.insert({ syncId, NetSyncPeer::Rev(dataRevision, ownerRevision, time) });
					}
					continue;
				}
			}
			else
			{
				sync = CreateNewNetSync(syncId, vec3);
				flagCreated = true;
			}

			sync->m_ownerRev = ownerRevision;
			sync->m_dataRev = dataRevision;
			//sync->m_owner = owner;
			//sync->InternalSetPosition(vector);
			//syncPeer->m_syncs.insert({ syncId, 
			//	NetSyncPeer::Rev(sync->m_dataRev, sync->m_ownerRev, time) }
			//);
			//sync->Deserialize(pkg2);
			//
			//if (flagCreated && m_deadNetSyncs.contains(syncId)) {
			//	sync->SetOwner(Valhalla()->m_serverUuid);
			//	this.DestroyNetSync(sync);
			//} else 
			//	sync->o
		}

		//m_NetSyncsRecv += recv;
			
	}

	bool SendNetSyncs(NetSyncPeer* peer, bool flush) {
		throw std::runtime_error("Not implemented");
		int sendQueueSize = peer->m_peer->m_rpc->m_socket->GetSendQueueSize();
		
		// flush is presumably for preventing network lagg
		if (!flush && sendQueueSize > 10240)
			return false;
		
		// this isnt really available space, more of micro space
		int availableSpace = 10240 - sendQueueSize;
		if (availableSpace < 2048)
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
		//	peer->m_syncs[sync->m_uid] = NetSyncPeer::Rev(
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

	void OnNewPeer(NetPeer::Ptr peer) {		
		m_peers.push_back(std::make_unique<NetSyncPeer>(peer));
		peer->m_rpc->Register(Rpc_Hash::ZDOData, &RPC_NetSyncData);
	}
}
