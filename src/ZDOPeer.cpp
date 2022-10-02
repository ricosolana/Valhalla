#include "ZDOPeer.h"
#include "ZDO.h"
//#include "ZDOID.hpp"
//#include "ZNetPeer.h"

void ZDOPeer::ZDOSectorInvalidated(ZDO* zdo) {
	throw std::runtime_error("Not implemented");
	//if (zdo->m_owner == m_peer->m_uid)
	//{
	//	return;
	//}
	//if (m_zdos.ContainsKey(zdo->m_uid) && !ZNetScene.instance.InActiveArea(zdo->GetSector(), m_peer->GetRefPos()))
	//{
	//	m_invalidSector.Add(zdo.m_uid);
	//	m_zdos.Remove(zdo.m_uid);
	//}
}

void ZDOPeer::ForceSendZDO(ZDOID id)
{
	m_forceSend.insert(id);
}

bool ZDOPeer::ShouldSend(ZDO *zdo)
{
	throw std::runtime_error("Not implemented");

	//auto&& find = m_zdos.find(zdo->m_uid);
	//if (find != m_zdos.end()) {
	//	//zdo->
	//	return true;
	//}
	//
	//return false;

	//PeerZDOInfo *peerZDOInfo;
	//return !this.m_zdos.TryGetValue(zdo.m_uid, out peerZDOInfo) || (ulong)zdo.m_ownerRevision > (ulong)peerZDOInfo.m_ownerRevision || zdo.m_dataRevision > peerZDOInfo.m_dataRevision;
}

PeerZDOInfo::PeerZDOInfo(unsigned int dataRevision, unsigned int ownerRevision, float syncTime) {
	m_dataRevision = dataRevision;
	m_ownerRevision = (long)((unsigned long)ownerRevision);
	m_syncTime = syncTime;
}
