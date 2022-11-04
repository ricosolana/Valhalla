#include "NetPeer.h"
#include "NetManager.h"

void NetPeer::Kick() {
	LOG(INFO) << "Kicking " << m_name;

	SendDisconnect();
	Disconnect();
}

void NetPeer::SendDisconnect() {
	m_rpc->Invoke("Disconnect");
}

void NetPeer::Disconnect() {
	m_rpc->m_socket->Close(true);
}