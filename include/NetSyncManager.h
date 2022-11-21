#pragma once

#include <robin_hood.h>
#include <vector>

#include "NetSync.h"
#include "Vector.h"
#include "HashUtils.h"
#include "NetPeer.h"

//class NetSync;

namespace NetSyncManager {
	void OnNewPeer(NetPeer *peer);
    void OnPeerQuit(NetPeer *peer);

    void Init();

	//void ShutDown();

	void Stop();

	void PrepareSave();

	// bwriter
	void SaveAsync(NetPackage &writer);

	// broader
	void Load(NetPackage &reader, int version);

    // This actually frees the zdos list
    // They arent marked, theyre trashed
	void ReleaseLegacyZDOS();

	void CapDeadZDOList();

	NetSync* CreateNewZDO(const Vector3 &position);

	NetSync* CreateNewZDO(const NetID &uid, const Vector3 &position);

	// Sector Coords -> Sector Pitch
	// Returns -1 on invalid sector
	void AddToSector(NetSync *zdo, const Vector2i &sector);

    // used by zdo to self invalidate its sector
	void ZDOSectorInvalidated(NetSync *zdo);

	void RemoveFromSector(NetSync* zdo, const Vector2i &sector);

	NetSync* GetZDO(const NetID &id);

	// called when registering joining peer

	void Update(float dt);

	void MarkDestroyZDO(NetSync* zdo);

	void FindSectorObjects(const Vector2i &sector, int area, int distantArea, 
		std::vector<NetSync*> &sectorObjects, std::vector<NetSync*> *distantSectorObjects = nullptr);

	void FindSectorObjects(const Vector2i &sector, int area, std::vector<NetSync*> &sectorObjects);

	//long GetMyID();

	void GetAllZDOsWithPrefab(const std::string &prefab, std::vector<NetSync*> zdos);

	// Used to get portals incrementally in a coroutine
	// basically, the coroutine thread is frozen in place
	// its not real multithreading, but is confusing for no reason
	// this can be refactored to have clearer intent
	bool GetAllZDOsWithPrefabIterative(const std::string &prefab, std::vector<NetSync*> zdos, int &index);

	// periodic stat logging
	//int NrOfObjects();
	//int GetSentZDOs();
	//int GetRecvZDOs();

	// seems to be client only for hud
	//void GetAverageStats(out float sentZdos, out float recvZdos);
	//int GetClientChangeQueue();
	//void RequestZDO(ZDOID id);

	void ForceSendZDO(const NetID &id);

	void ForceSendZDO(OWNER_t peerID, const NetID& id);

	//void ClientChanged(const NetID& id);

	//std::function<void(NetSync*)> m_onZDODestroyed;

	//void RPC_NetSyncData(NetRpc* rpc, NetPackage pkg);
}
