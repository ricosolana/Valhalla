#include "ZDOPeer.h"
#include "NetPeer.h"
#include "ZoneManager.h"

ZDOPeer::ZDOPeer(NetPeer* peer) : m_peer(peer) {}

void ZDOPeer::ZDOSectorInvalidated(ZDO* zdo) {
	if (zdo->m_owner == m_peer->m_uuid)
		return;

	if (!ZoneManager()->ZonesOverlap(zdo->Sector(), m_peer->m_pos)) {
		if (m_zdos.erase(zdo->ID())) {
			m_invalidSector.insert(zdo->ID());
		}
	}
}
