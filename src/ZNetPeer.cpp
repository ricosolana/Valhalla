#include "ZNetPeer.h"
#include "ZNet.h"
#include <memory>

//ZNetPeer::ZNetPeer(ISocket::Ptr sock) :
//	m_socket(sock),
//	m_rpc(std::make_unique<ZRpc>(sock))
//{}

void ZNetPeer::Kick() {
	SendDisconnect();
	Disconnect();
}

void ZNetPeer::SendDisconnect() {
	m_rpc->Invoke("Disconnect");
}

void ZNetPeer::Disconnect() {
	m_rpc->m_socket->Close();
}