#include "NetPeer.h"
#include "NetManager.h"
#include <memory>

//NetPeer::NetPeer(ISocket::Ptr sock) :
//	m_socket(sock),
//	m_rpc(std::make_unique<NetRpc>(sock))
//{}

void NetPeer::Kick() {
	LOG(INFO) << "Kicking " << m_name;

	SendDisconnect();
	Disconnect();
}

void NetPeer::SendDisconnect() {
	m_rpc->Invoke("Disconnect");
}

void NetPeer::Disconnect() {
	m_rpc->m_socket->Close();
}