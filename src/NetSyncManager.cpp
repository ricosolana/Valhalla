#include "NetSyncManager.h"
#include "ValhallaServer.h"
#include "NetHashes.h"

namespace NetSyncManager {

	struct NetSyncPeer {

		struct Rev {
			uint32_t m_dataRevision;
			uint32_t m_ownerRevision;
			float m_syncTime;
			//char a;
			//uint64_t m_time; // time in ms since last modified (synced)
		};

		NetPeer::Ptr m_peer;
		robin_hood::unordered_map<NetID, Rev, HashUtils::Hasher> m_syncs;
		robin_hood::unordered_set<NetID, HashUtils::Hasher> m_forceSend;
		robin_hood::unordered_set<NetID, HashUtils::Hasher> m_invalidSector;
		//int m_sendIndex = 0;

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
			auto find = m_syncs.find(netSync->m_id);
			return find != m_syncs.end()
				|| netSync->m_ownerRevision > find->second.m_ownerRevision
				|| netSync->m_dataRevision > find->second.m_dataRevision;
			return 0;
		}
	};

	static std::vector<std::unique_ptr<NetSyncPeer>> m_peers;
	static robin_hood::unordered_map<NetID, UUID_t, HashUtils::Hasher> m_deadNetSyncs;
	
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

	NetSyncPeer* GetPeer(NetPeer* netpeer) {
		return GetPeer(netpeer->m_uuid);
	}

	NetSync *CreateNewNetSync(NetID uid, Vector3 position)
	{
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
		//int num = 0; // NetSyncs to be received
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
				if (dataRevision <= sync->m_dataRevision) {
					if (ownerRevision > sync->m_ownerRevision) {
						//sync->m_owner = owner;
						sync->m_ownerRevision = ownerRevision;
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

			sync->m_ownerRevision = ownerRevision;
			sync->m_dataRevision = dataRevision;
			//sync->m_owner = owner;
			//sync->InternalSetPosition(vector);
			//syncPeer->m_syncs.insert({ syncId, 
			//	NetSyncPeer::Rev(sync->m_dataRevision, sync->m_ownerRevision, time) }
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
		//	pkg->Write(sync->m_ownerRevision);
		//	pkg->Write(sync->m_dataRevision);
		//	pkg->Write(sync->m_owner);
		//	pkg->Write(sync->GetPosition());
		//	
		//	auto NetSyncpkg(PKG());
		//	sync->Serialize(NetSyncpkg); // dump sync information onto packet
		//	pkg->Write(NetSyncpkg);
		//
		//	peer->m_syncs[sync->m_uid] = NetSyncPeer::Rev(
		//		sync->m_dataRevision, sync->m_ownerRevision, time);
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
