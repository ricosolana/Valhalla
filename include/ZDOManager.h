#pragma once

#include <vector>

#include "ZDO.h"
#include "Vector.h"
#include "NetPeer.h"

//class NetSync;



class IManagerZDO {
	friend class SaveData;

	struct SyncPeer {
		NetPeer* m_peer;
		robin_hood::unordered_map<NetID, NetSync::Rev> m_syncs;
		robin_hood::unordered_set<NetID> m_forceSend;
		robin_hood::unordered_set<NetID> m_invalidSector;
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

private:
	void OnNewPeer(NetPeer* peer);
	void OnPeerQuit(NetPeer* peer);

	void Init();

	//void ShutDown();

	void Stop();

	SyncPeer* GetPeer(OWNER_t uuid);
	SyncPeer* GetPeer(NetPeer* netPeer);
	SyncPeer* GetPeer(NetRpc* rpc);

	void SendZDOToPeers2();
	void FlushClientObjects(); // sends all zdos; the fast that they're labeled as objects here proves the point a better name is SyncObject?
	void ReleaseZDOS();
	bool IsInPeerActiveArea(const Vector2i& sector, OWNER_t id);
	void ReleaseNearbyZDOS(const Vector3& refPosition, OWNER_t id);
	void SendDestroyedZDOs();
	void HandleDestroyedZDO(const NetID& uid);
	void SendAllZDOs(SyncPeer* peer);
	bool SendZDOs(SyncPeer* peer, bool flush);
	void RPC_ZDOData(NetRpc* rpc, NetPackage pkg);
	void CreateSyncList(SyncPeer* peer, std::vector<NetSync*>& toSync);
	void AddForceSendZDOs(SyncPeer* peer, std::vector<NetSync*>& syncList);

	void ServerSortSendZDOS(std::vector<NetSync*>& objects, const Vector3& refPos, SyncPeer* peer);

	int SectorToIndex(const Vector2i& s);
	void FindObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void FindDistantObjects(const Vector2i& sector, std::vector<NetSync*>& objects);
	void RemoveOrphanNonPersistentZDOS();
	bool IsPeerConnected(OWNER_t uid);

	static constexpr int SECTOR_WIDTH = 512;
	static constexpr int MAX_DEAD_OBJECTS = 100000;

	std::list<std::unique_ptr<SyncPeer>> m_peers; // Peer lifetimes



	// so ensure with a bunch of asserts of something that all ZDO external references are removed once the zdo is popped from here
	robin_hood::unordered_map<NetID, std::unique_ptr<NetSync>> m_objectsByID;    // primary lifetime container



	std::array<robin_hood::unordered_set<NetSync*>, SECTOR_WIDTH* SECTOR_WIDTH> m_objectsBySector;   // a bunch of objects
	// TODO this might be essentially never used in game
	robin_hood::unordered_map<Vector2i, robin_hood::unordered_set<NetSync*>> m_objectsByOutsideSector;

	//constexpr static int s00 = sizeof(m_objectsBySector);
	//constexpr static int s01 = sizeof(robin_hood::unordered_map<Vector2i, robin_hood::unordered_set<NetSync*>, HashUtils::Hasher>);

	robin_hood::unordered_map<NetID, int64_t> m_deadZDOs;
	//std::vector<NetSync*> tempSectorObjects;
	std::vector<NetID> m_destroySendList;

	// Increments indefinitely for new ZDO id
	uint32_t m_nextUid = 1;

public:
	void PrepareSave();

	// bwriter
	void SaveAsync(NetPackage& writer);

	// broader
	void Load(NetPackage& reader, int version);

	// This actually frees the zdos list
	// They arent marked, theyre trashed
	void ReleaseLegacyZDOS();

	void CapDeadZDOList();

	NetSync* CreateNewZDO(const Vector3& position);

	NetSync* CreateNewZDO(const NetID& uid, const Vector3& position);

	// Sector Coords -> Sector Pitch
	// Returns -1 on invalid sector
	void AddToSector(NetSync* zdo, const Vector2i& sector);

	// used by zdo to self invalidate its sector
	void ZDOSectorInvalidated(NetSync* zdo);

	void RemoveFromSector(NetSync* zdo, const Vector2i& sector);

	NetSync* GetZDO(const NetID& id);

	// called when registering joining peer

	void Update();

	void MarkDestroyZDO(NetSync* zdo);

	void FindSectorObjects(const Vector2i& sector, int area, int distantArea,
		std::vector<NetSync*>& sectorObjects, std::vector<NetSync*>* distantSectorObjects = nullptr);

	void FindSectorObjects(const Vector2i& sector, int area, std::vector<NetSync*>& sectorObjects);

	//long GetMyID();

	void GetAllZDOsWithPrefab(const std::string& prefab, std::vector<NetSync*> zdos);

	// Used to get portals incrementally in a coroutine
	// basically, the coroutine thread is frozen in place
	// its not real multithreading, but is confusing for no reason
	// this can be refactored to have clearer intent
	bool GetAllZDOsWithPrefabIterative(const std::string& prefab, std::vector<NetSync*> zdos, int& index);

	// periodic stat logging
	//int NrOfObjects();
	//int GetSentZDOs();
	//int GetRecvZDOs();

	// seems to be client only for hud
	//void GetAverageStats(out float sentZdos, out float recvZdos);
	//int GetClientChangeQueue();
	//void RequestZDO(ZDOID id);

	void ForceSendZDO(const NetID& id);

	void ForceSendZDO(OWNER_t peerID, const NetID& id);

	//void ClientChanged(const NetID& id);

	//std::function<void(NetSync*)> m_onZDODestroyed;

	//void RPC_NetSyncData(NetRpc* rpc, NetPackage pkg);
};

IManagerZDO* ZDOManager();
