#include "ZDOMan.h"
#include "ValhallaServer.h"

namespace ZDOManager {

	struct ZDOPeer {
		struct PeerZDOInfo {
			uint32_t m_dataRevision;
			int64_t m_ownerRevision;
			float m_syncTime;

			PeerZDOInfo(uint32_t dataRevision, uint32_t ownerRevision, float syncTime)
			{
				m_dataRevision = dataRevision;
				m_ownerRevision = (int64_t)((uint64_t)ownerRevision);
				m_syncTime = syncTime;
			}
		};

		NetPeer::Ptr m_peer;
		robin_hood::unordered_map<ZDOID, PeerZDOInfo, HashUtils::Hasher> m_zdos;
		robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_forceSend;
		robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_invalidSector;
		int m_sendIndex;

		ZDOPeer(NetPeer::Ptr peer) : m_peer(peer) {}

		void ZDOSectorInvalidated(ZDO *zdo) {
			throw std::runtime_error("Not implemented");
			//if (zdo->m_owner == m_peer->m_uuid)
			//	return;
			//
			//if (m_zdos.contains(zdo->m_uid) 
			//	&& !ZNetScene.instance.InActiveArea(zdo->GetSector(), m_peer->m_pos)) {
			//	m_invalidSector.insert(zdo.m_uid);
			//	m_zdos.erase(zdo->m_uid);
			//}
		}

		void ForceSendZDO(ZDOID id) {
			m_forceSend.insert(id);
		}

		bool ShouldSend(ZDO zdo) {
			auto find = m_zdos.find(zdo.m_uid);
			return find != m_zdos.end()
				|| ((uint64_t)zdo.m_ownerRevision > (uint64_t)find->second.m_ownerRevision)
				|| (zdo.m_dataRevision > find->second.m_dataRevision);
		}
	};

	static std::vector<std::unique_ptr<ZDOPeer>> m_peers;
	static robin_hood::unordered_map<ZDOID, uuid_t, HashUtils::Hasher> m_deadZDOs;
	
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

	ZDO *GetZDO(ZDOID& id) {
		if (!id) {
			return nullptr;
		}
		////auto &&find = m_objec
		//ZDO result;
		//if (this.m_objectsByID.TryGetValue(id, out result))
		//{
		//	return result;
		//}
		//return null;
		return nullptr;
	}

	ZDOPeer* GetPeer(uuid_t uuid) {
		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_uuid == uuid)
				return peer.get();
		}
		return nullptr;
	}

	ZDOPeer* GetPeer(NetRpc* rpc) {
		for (auto&& peer : m_peers) {
			if (peer->m_peer->m_rpc.get() == rpc)
				return peer.get();
		}
		return nullptr;
	}

	ZDOPeer* GetPeer(NetPeer* netpeer) {
		return GetPeer(netpeer->m_uuid);
	}

	ZDO *CreateNewZDO(ZDOID uid, Vector3 position)
	{
		//ZDO zdo = ZDOPool.Create(this, uid, position);
		//zdo.m_owner = this.m_myid;
		//zdo.m_timeCreated = ZNet.instance.GetTime().Ticks;
		//this.m_objectsByID.Add(uid, zdo);
		//return zdo;
		return nullptr;
	}

	void RPC_ZDOData(NetRpc* rpc, NetPackage::Ptr pkg) {
		throw std::runtime_error("Not implemented");
		auto zdopeer = GetPeer(rpc);
		assert(zdopeer);

		float time = 0; //Time.time;
		//int num = 0; // zdos to be received
		auto invalid_sector_count = pkg->Read<int32_t>(); // invalid sector count
		for (int i = 0; i < invalid_sector_count; i++)
		{
			auto id = pkg->Read<ZDOID>();
			//ZDO zdo = this.GetZDO(id);
			//if (zdo != null)
			//{
			//	zdo.InvalidateSector();
			//}
		}

		int zdosRecv = 0;
		for (;;)
		{
			auto zdoid = pkg->Read<ZDOID>(); // uid
			if (!zdoid)
				break;

			zdosRecv++;

			auto zdo = GetZDO(zdoid);

			auto num3 = pkg->Read<uint32_t>(); // owner revision
			auto num4 = pkg->Read<uint32_t>(); // data revision
			auto owner = pkg->Read<uuid_t>(); // owner
			auto vector = pkg->Read<Vector3>(); // position
			auto pkg2 = pkg->Read<NetPackage::Ptr>(); //
			
			bool flag = false;
			if (zdo) {
				if (num4 <= zdo->m_dataRevision) {
					if (num3 > zdo->m_ownerRevision) {
						//zdo->m_owner = owner;
						zdo->m_ownerRevision = num3;
						zdopeer->m_zdos.insert({ zdoid, ZDOPeer::PeerZDOInfo(num4, num3, time) });
					}
					continue;
				}
			}
			else
			{
				zdo = CreateNewZDO(zdoid, vector);
				flag = true;
			}

			//zdo->m_ownerRevision = num3;
			//zdo->m_dataRevision = num4;
			//zdo->m_owner = owner;
			//zdo->InternalSetPosition(vector);
			//zdopeer->m_zdos.insert({ zdoid, 
			//	ZDOPeer::PeerZDOInfo(zdo->m_dataRevision, zdo->m_ownerRevision, time) }
			//);
			//zdo->Deserialize(pkg2);
			//if (flag && m_deadZDOs.contains(zdoid)) {
			//	zdo->SetOwner(Valhalla()->m_serverUuid);
			//	this.DestroyZDO(zdo);
			//}
		}

		//m_zdosRecv += zdosRecv;
			
	}

	bool SendZDOs(ZDOPeer* peer, bool flush) {
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

		// continue only if there are updated/invalid zdos to send
		//if (m_tempToSync.Count == 0 && peer->m_invalidSector.empty())
		//	return false;

		/*
		* ZDOData packet structure:
		*  - 4 bytes: invalidSectors.size()
		*    - invalidSectors zdoid array with size above
		*  - array of zdo [
		*		zdoid,
		*		owner rev
		*		data rev
		*		owner
		*		zdo pos
		*		zdo data
		*    ] ZDOID::null for termination
		*/

		auto pkg(PKG());
		bool zdosWritten = false;

		pkg->Write(static_cast<int32_t>(peer->m_invalidSector.size()));
		if (!peer->m_invalidSector.empty()) {
			for (auto&& id : peer->m_invalidSector) {
				pkg->Write(id);
			}

			peer->m_invalidSector.clear();
			zdosWritten = true;
		}

		//float time = Time.time;
		float time = 0;
		
		//int currentTemp = 0;
		//while (currentTemp < m_tempToSync.Count && pkg->GetStream().Length() <= availableSpace)
		//{
		//	ZDO *zdo = m_tempToSync[currentTemp];
		//	peer->m_forceSend.erase(zdo->m_uid);
		//
		//
		//	pkg->Write(zdo->m_uid);
		//	pkg->Write(zdo->m_ownerRevision);
		//	pkg->Write(zdo->m_dataRevision);
		//	pkg->Write(zdo->m_owner);
		//	pkg->Write(zdo->GetPosition());
		//	
		//	auto zdopkg(PKG());
		//	zdo->Serialize(zdopkg); // dump zdo information onto packet
		//	pkg->Write(zdopkg);
		//
		//	peer->m_zdos[zdo->m_uid] = ZDOPeer::PeerZDOInfo(
		//		zdo->m_dataRevision, zdo->m_ownerRevision, time);
		//	
		//	zdosWritten = true;
		//	//m_zdosSent++;
		//	currentTemp++;
		//}
		//pkg->Write(ZDOID::NONE); // used as the null terminator

		if (zdosWritten)
			peer->m_peer->m_rpc->Invoke("ZDOData", pkg);

		return zdosWritten;
	}

	void OnNewPeer(NetPeer::Ptr peer) {		
		m_peers.push_back(std::make_unique<ZDOPeer>(peer));
		peer->m_rpc->Register("ZDOData", &RPC_ZDOData);

		//ZDOMan.ZDOPeer zdopeer = new ZDOMan.ZDOPeer();
		//zdopeer.m_peer = netPeer;
		//this.m_peers.Add(zdopeer);
		//zdopeer.m_peer.m_rpc.Register<ZPackage>("ZDOData", new Action<ZRpc, ZPackage>(this.RPC_ZDOData));
	}



}
