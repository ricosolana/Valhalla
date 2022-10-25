#pragma once

#include <robin_hood.h>
#include <vector>

#include "NetSync.h"
#include "Vector.h"
#include "HashUtils.h"
#include "NetPeer.h"

namespace NetSyncManager {
	void OnNewPeer(NetPeer::Ptr peer);





	//void ShutDown();

	void Stop();

	void PrepareSave();

	// bwriter
	void SaveAsync(NetPackage &writer);

	// breader
	void Load(NetPackage &reader, int version);

	void RemoveOldGeneratedZDOS();

	void CapDeadZDOList();

	NetSync* CreateNewZDO(const Vector3 &position);

	NetSync* CreateNewZDO(const NetID &uid, const Vector3 &position);

	// Sector Coords -> Sector Pitch
	// Returns -1 on invalid sector
	void AddToSector(NetSync *zdo, const Vector2i &sector);

	void ZDOSectorInvalidated(NetSync *zdo);

	void RemoveFromSector(NetSync* zdo, const Vector2i &sector);

	NetSync* GetZDO(const NetID &id);

	// called when registering joining peer
	//void AddPeer(NetPeer::Ptr netPeer);
	void RemovePeer(NetPeer::Ptr netPeer);

	void Update(float dt);

	void DestroyZDO(NetSync* zdo);

	void FindSectorObjects(const Vector2i &sector, int area, int distantArea, 
		std::vector<NetSync*> sectorObjects, std::vector<NetSync*> distantSectorObjects = nullptr);

	void FindSectorObjects(const Vector2i &sector, int area, std::vector<NetSync*> sectorObjects);

	//long GetMyID();

	void GetAllZDOsWithPrefab(std::string prefab, std::vector<NetSync*> zdos);

	bool GetAllZDOsWithPrefabIterative(std::string prefab, std::vector<NetSync*> zdos, ref int index);

	// periodic stat logging
	//int NrOfObjects();
	//int GetSentZDOs();
	//int GetRecvZDOs();

	// seems to be client only for hud
	//void GetAverageStats(out float sentZdos, out float recvZdos);
	//int GetClientChangeQueue();
	//void RequestZDO(ZDOID id);

	void ForceSendZDO(const NetID &id);

	void ForceSendZDO(long peerID, const NetID& id);

	void ClientChanged(const NetID& id);

	std::function<void(NetSync*)> m_onZDODestroyed;
}
