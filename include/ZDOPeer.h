#pragma once

#include <vector>
#include <robin_hood.h>
#include "ZDOID.h"
#include "HashUtils.h"

class ZDO;
struct ZNetPeer;

struct PeerZDOInfo
{
	PeerZDOInfo(uint32_t dataRevision, uint32_t ownerRevision, float syncTime);

	uint32_t m_dataRevision;
	int64_t m_ownerRevision;
	float m_syncTime;
};

struct ZDOPeer
{
	void ZDOSectorInvalidated(ZDO *zdo);
	void ForceSendZDO(ZDOID id);
	bool ShouldSend(ZDO *zdo);

	ZNetPeer *m_peer;
	robin_hood::unordered_map<ZDOID, PeerZDOInfo, HashUtils::Hasher> m_zdos;
	robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_forceSend;
	robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_invalidSector;
	int32_t m_sendIndex;
};
