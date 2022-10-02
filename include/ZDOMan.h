#pragma once

#include <robin_hood.h>
#include <vector>
#include "ZDOID.h"
#include "ZDO.h"
#include "Vector2.h"
#include "HashUtils.h"
#include "ZNetPeer.h"

// might be better named
// zeta object transceiver
// zeta object manager
// object net sync manager
class ZDOMan {
	struct SaveData {
		uuid_t m_myid;
		uint32_t m_nextUid = 1U;
		std::vector<ZDO*> m_zdos;
		robin_hood::unordered_map<ZDOID, int64_t, HashUtils::Hasher> m_deadZDOs;
	};

	struct PeerZDOInfo
	{
		// Token: 0x06000B97 RID: 2967 RVA: 0x00052B38 File Offset: 0x00050D38
		PeerZDOInfo(uint32_t dataRevision, uint32_t ownerRevision, float syncTime)
		{
			m_dataRevision = dataRevision;
			m_ownerRevision = (int64_t)((uint64_t)ownerRevision);
			m_syncTime = syncTime;
		}

		// Token: 0x04000CD7 RID: 3287
		uint32_t m_dataRevision;

		// Token: 0x04000CD8 RID: 3288
		int64_t m_ownerRevision;

		// Token: 0x04000CD9 RID: 3289
		float m_syncTime;
	};

	struct ZDOPeer {
		using Ptr = std::unique_ptr<ZDOPeer>;

		// Token: 0x06000B93 RID: 2963 RVA: 0x00052A48 File Offset: 0x00050C48
		void ZDOSectorInvalidated(ZDO zdo);

		// Token: 0x06000B94 RID: 2964 RVA: 0x00052ABD File Offset: 0x00050CBD
		void ForceSendZDO(ZDOID id);

		// Token: 0x06000B95 RID: 2965 RVA: 0x00052ACC File Offset: 0x00050CCC
		bool ShouldSend(ZDO zdo);

		// Token: 0x04000CD2 RID: 3282
		ZNetPeer::Ptr m_peer;

		robin_hood::unordered_map<ZDOID, PeerZDOInfo, HashUtils::Hasher> m_zdos;

		// Token: 0x04000CD3 RID: 3283
		//public Dictionary<ZDOID, ZDOMan.ZDOPeer.PeerZDOInfo> m_zdos = new Dictionary<ZDOID, ZDOMan.ZDOPeer.PeerZDOInfo>();

		robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_forceSend;

		// Token: 0x04000CD4 RID: 3284
		//public HashSet<ZDOID> m_forceSend = new HashSet<ZDOID>();

		robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_invalidSector;

		// Token: 0x04000CD5 RID: 3285
		//public HashSet<ZDOID> m_invalidSector = new HashSet<ZDOID>();

		// Token: 0x04000CD6 RID: 3286
		int m_sendIndex;

		// Token: 0x02000126 RID: 294

		ZDOPeer(ZNetPeer::Ptr peer) {

		}

	};

	// why static
	static int64_t compareReceiver;

	robin_hood::unordered_map<ZDOID, ZDO, HashUtils::Hasher> m_objectsByID;
	std::unique_ptr<std::vector<ZDO>> m_objectsBySector; // array of vector
	//robin_hood::unordered_map<Vector2i, List<ZDO>> m_objectsByOutsideSector;
	//List<ZDOMan.ZDOPeer> m_peers = new List<ZDOMan.ZDOPeer>();
	const int m_maxDeadZDOs = 100000;
	robin_hood::unordered_map<ZDOID, uuid_t, HashUtils::Hasher> m_deadZDOs;
	std::vector<ZDOID> m_destroySendList;
	robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_clientChangeQueue;
	uuid_t m_myid;
	uint32_t m_nextUid = 1U;
	int32_t m_width;
	int32_t m_halfWidth;
	float m_sendTimer;
	const float m_sendFPS = 20.f;
	float m_releaseZDOTimer;
	static ZDOMan m_instance;
	int32_t m_zdosSent;
	int32_t m_zdosRecv;
	int32_t m_zdosSentLastSec;
	int32_t m_zdosRecvLastSec;
	float m_statTimer;
	std::vector<ZDO> m_tempToSync;
	std::vector<ZDO> m_tempToSyncDistant;
	std::vector<ZDO> m_tempNearObjects;
	std::vector<ZDOID> m_tempRemoveList;
	std::vector<ZDOPeer::Ptr> m_peers;
	SaveData m_saveData;

public:
	//Action<ZDO> m_onZDODestroyed;

	ZDOMan();

	int64_t GetMyID();

	void AddPeer(ZNetPeer::Ptr peer);
	ZDO GetZDO(ZDOID& zdoid);

private:
	void RPC_ZDOData(ZRpc* rpc, ZPackage::Ptr pkg);

	void RPC_DestroyZDO(uuid_t sender, ZPackage pkg);
	void RPC_RequestZDO(uuid_t sender, ZDOID id);

	ZDOPeer* GetZDOPeer(ZRpc* rpc);
};

//ZDOMan ZDOMan();