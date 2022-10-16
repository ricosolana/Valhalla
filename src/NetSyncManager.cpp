#include "NetSyncManager.h"
#include "ValhallaServer.h"

namespace NetSyncManager {

	struct NetSyncPeer {
		struct PeerNetSyncInfo {
			uint32_t m_dataRevision;
			int64_t m_ownerRevision;
			float m_syncTime;

			PeerNetSyncInfo(uint32_t dataRevision, uint32_t ownerRevision, float syncTime)
			{
				m_dataRevision = dataRevision;
				m_ownerRevision = (int64_t)((uint64_t)ownerRevision);
				m_syncTime = syncTime;
			}
		};

		NetPeer::Ptr m_peer;
		robin_hood::unordered_map<NetSync::ID, PeerNetSyncInfo, HashUtils::Hasher> m_NetSyncs;
		robin_hood::unordered_set<NetSync::ID, HashUtils::Hasher> m_forceSend;
		robin_hood::unordered_set<NetSync::ID, HashUtils::Hasher> m_invalidSector;
		int m_sendIndex;

		NetSyncPeer(NetPeer::Ptr peer) : m_peer(peer) {}

		void NetSyncSectorInvalidated(NetSync *netSync) {
			throw std::runtime_error("Not implemented");
			//if (NetSync->m_owner == m_peer->m_uuid)
			//	return;
			//
			//if (m_NetSyncs.contains(NetSync->m_uid) 
			//	&& !ZNetScene.instance.InActiveArea(NetSync->GetSector(), m_peer->m_pos)) {
			//	m_invalidSector.insert(NetSync.m_uid);
			//	m_NetSyncs.erase(NetSync->m_uid);
			//}
		}

		void ForceSendNetSync(NetSync::ID id) {
			m_forceSend.insert(id);
		}

		bool ShouldSend(NetSync netSync) {
			//auto find = m_NetSyncs.find(netSync.m_NetSyncID);
			//return find != m_NetSyncs.end()
			//	|| ((uint64_t) netSync.m_ownerRevision > (uint64_t)find->second.m_ownerRevision)
			//	|| (netSync.m_dataRevision > find->second.m_dataRevision);
			return 0;
		}
	};

	static std::vector<std::unique_ptr<NetSyncPeer>> m_peers;
	static robin_hood::unordered_map<NetSync::ID, uuid_t, HashUtils::Hasher> m_deadNetSyncs;
	
	static constexpr int SECTOR_WIDTH = 512;

	// Sector Coords -> Sector Pitch
	int SectorToIndex(Vector2i s) {
		int x = s.x + SECTOR_WIDTH / 2;
		int y = s.y + SECTOR_WIDTH / 2;
		if (x < 0 || y < 0 
			|| x >= SECTOR_WIDTH || y >= SECTOR_WIDTH) {
			return -1;
		}
		return y * SECTOR_WIDTH + x;
	}

	NetSync *GetNetSync(NetSync::ID& id) {
		if (!id) {
			return nullptr;
		}
		////auto &&find = m_objec
		//NetSync result;
		//if (this.m_objectsByID.TryGetValue(id, out result))
		//{
		//	return result;
		//}
		//return null;
		return nullptr;
	}

	NetSyncPeer* GetPeer(uuid_t uuid) {
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

	NetSyncPeer* GetPeer(NetPeer* netpeer) {
		return GetPeer(netpeer->m_uuid);
	}

	NetSync *CreateNewNetSync(NetSync::ID uid, Vector3 position)
	{
		//NetSync NetSync = NetSyncPool.Create(this, uid, position);
		//NetSync.m_owner = this.m_myid;
		//NetSync.m_timeCreated = ZNet.instance.GetTime().Ticks;
		//this.m_objectsByID.Add(uid, NetSync);
		//return NetSync;
		return nullptr;
	}

	void RPC_NetSyncData(NetRpc* rpc, NetPackage::Ptr pkg) {
		throw std::runtime_error("Not implemented");

		auto NetSyncpeer = GetPeer(rpc);
		assert(NetSyncpeer);

		float time = 0; //Time.time;
		//int num = 0; // NetSyncs to be received
		auto invalid_sector_count = pkg->Read<int32_t>(); // invalid sector count
		for (int i = 0; i < invalid_sector_count; i++)
		{
			auto id = pkg->Read<NetSync::ID>();
			//NetSync NetSync = this.GetNetSync(id);
			//if (NetSync != null)
			//{
			//	NetSync.InvalidateSector();
			//}
		}

		int NetSyncsRecv = 0;
		for (;;)
		{
			auto NetSyncid = pkg->Read<NetSync::ID>(); // uid
			if (!NetSyncid)
				break;

			NetSyncsRecv++;

			auto NetSync = GetNetSync(NetSyncid);

			auto ownerRevision = pkg->Read<uint32_t>(); // owner revision
			auto dataRevision = pkg->Read<uint32_t>(); // data revision
			auto owner = pkg->Read<uuid_t>(); // owner
			auto vec3 = pkg->Read<Vector3>(); // position
			auto pkg2 = pkg->Read<NetPackage::Ptr>(); //
			
			bool flagCreated = false;
			if (NetSync) {
				if (dataRevision <= NetSync->m_dataRevision) {
					if (ownerRevision > NetSync->m_ownerRevision) {
						//NetSync->m_owner = owner;
						NetSync->m_ownerRevision = ownerRevision;
						NetSyncpeer->m_NetSyncs.insert({ NetSyncid, NetSyncPeer::PeerNetSyncInfo(dataRevision, ownerRevision, time) });
					}
					continue;
				}
			}
			else
			{
				NetSync = CreateNewNetSync(NetSyncid, vec3);
				flagCreated = true;
			}

			NetSync->m_ownerRevision = ownerRevision;
			NetSync->m_dataRevision = dataRevision;
			//NetSync->m_owner = owner;
			//NetSync->InternalSetPosition(vector);
			//NetSyncpeer->m_NetSyncs.insert({ NetSyncid, 
			//	NetSyncPeer::PeerNetSyncInfo(NetSync->m_dataRevision, NetSync->m_ownerRevision, time) }
			//);
			//NetSync->Deserialize(pkg2);
			//
			//if (flagCreated && m_deadNetSyncs.contains(NetSyncid)) {
			//	NetSync->SetOwner(Valhalla()->m_serverUuid);
			//	this.DestroyNetSync(NetSync);
			//} else 
			//	NetSync->o
		}

		//m_NetSyncsRecv += NetSyncsRecv;
			
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
		*    - invalidSectors NetSyncid array with size above
		*  - array of NetSync [
		*		NetSyncid,
		*		owner rev
		*		data rev
		*		owner
		*		NetSync pos
		*		NetSync data
		*    ] NetSyncID::null for termination
		*/

		auto pkg(PKG());
		bool NetSyncsWritten = false;

		pkg->Write(static_cast<int32_t>(peer->m_invalidSector.size()));
		if (!peer->m_invalidSector.empty()) {
			for (auto&& id : peer->m_invalidSector) {
				pkg->Write(id);
			}

			peer->m_invalidSector.clear();
			NetSyncsWritten = true;
		}

		//float time = Time.time;
		float time = 0;
		
		//int currentTemp = 0;
		//while (currentTemp < m_tempToSync.Count && pkg->GetStream().Length() <= availableSpace)
		//{
		//	NetSync *NetSync = m_tempToSync[currentTemp];
		//	peer->m_forceSend.erase(NetSync->m_uid);
		//
		//
		//	pkg->Write(NetSync->m_uid);
		//	pkg->Write(NetSync->m_ownerRevision);
		//	pkg->Write(NetSync->m_dataRevision);
		//	pkg->Write(NetSync->m_owner);
		//	pkg->Write(NetSync->GetPosition());
		//	
		//	auto NetSyncpkg(PKG());
		//	NetSync->Serialize(NetSyncpkg); // dump NetSync information onto packet
		//	pkg->Write(NetSyncpkg);
		//
		//	peer->m_NetSyncs[NetSync->m_uid] = NetSyncPeer::PeerNetSyncInfo(
		//		NetSync->m_dataRevision, NetSync->m_ownerRevision, time);
		//	
		//	NetSyncsWritten = true;
		//	//m_NetSyncsSent++;
		//	currentTemp++;
		//}
		//pkg->Write(NetSyncID::NONE); // used as the null terminator

		if (NetSyncsWritten)
			peer->m_peer->m_rpc->Invoke("NetSyncData", pkg);

		return NetSyncsWritten;
	}

	void OnNewPeer(NetPeer::Ptr peer) {		
		m_peers.push_back(std::make_unique<NetSyncPeer>(peer));
		peer->m_rpc->Register("NetSyncData", &RPC_NetSyncData);

		//NetSyncMan.NetSyncPeer NetSyncpeer = new NetSyncMan.NetSyncPeer();
		//NetSyncpeer.m_peer = netPeer;
		//this.m_peers.Add(NetSyncpeer);
		//NetSyncpeer.m_peer.m_rpc.Register<ZPackage>("NetSyncData", new Action<ZRpc, ZPackage>(this.RPC_NetSyncData));
	}



}
