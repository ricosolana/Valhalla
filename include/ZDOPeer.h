#pragma once

#include <robin_hood.h>

#include "ZDOManager.h"

class ZDOPeer {
	friend class IZDOManager;

private:
	robin_hood::unordered_map<NetID, ZDO::Rev> m_zdos;
	robin_hood::unordered_set<NetID> m_forceSend;
	robin_hood::unordered_set<NetID> m_invalidSector;
	int m_sendIndex = 0; // used incrementally for which next zdos to send from index

	void ZDOSectorInvalidated(ZDO* zdo) {
		throw std::runtime_error("not implemented");
		//if (sync->m_owner == m_peer->m_uuid)
		//	return;
		//
		//if (!ZoneSystem()->)
		//auto&& find = m_syncs.find(zdo->GetNetID());
		//if (find != m_syncs.end())
		//
		//if (m_syncs.contains(sync->m_uid) 
		//	&& !ZNetScene.instance.InActiveArea(sync->GetSector(), m_peer->m_pos)) {
		//	m_invalidSector.insert(sync.m_uid);
		//	m_syncs.erase(sync->m_uid);
		//}
	}

	void ForceSendZDO(NetID id) {
		m_forceSend.insert(id);
	}

	// Returns whether the sync is outdated
	//	tests if the peer does not have a copy
	//	tests if the zdo is outdated in owner and data
	bool ShouldSend(ZDO* netSync) {
		auto find = m_zdos.find(netSync->ID());

		return find == m_zdos.end()
			|| netSync->m_rev.m_ownerRev > find->second.m_ownerRev
			|| netSync->m_rev.m_dataRev > find->second.m_dataRev;
	}
};
