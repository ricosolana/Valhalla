#pragma once

#include <robin_hood.h>

#include "ZDOManager.h"

class ZDOPeer {
	friend class IZDOManager;

private:
	NetPeer* m_peer;

	robin_hood::unordered_map<NetID, ZDO::Rev> m_zdos;
	robin_hood::unordered_set<NetID> m_forceSend;
	robin_hood::unordered_set<NetID> m_invalidSector;
	int m_sendIndex = 0; // used incrementally for which next zdos to send from index

private:
	ZDOPeer(NetPeer* peer);

	void ZDOSectorInvalidated(ZDO* zdo);

	void ForceSendZDO(NetID id) {
		m_forceSend.insert(id);
	}

	// Returns whether the sync is outdated
	//	tests if the peer does not have a copy
	//	tests if the zdo is outdated in owner and data
	bool ShouldSend(ZDO* zdo) {
		auto find = m_zdos.find(zdo->ID());

		return find == m_zdos.end()
			|| zdo->m_rev.m_ownerRev > find->second.m_ownerRev
			|| zdo->m_rev.m_dataRev > find->second.m_dataRev;
	}
};
