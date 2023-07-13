#include "NetSocket.h"

ProxySocket::ProxySocket(ISocket::Ptr frontendSocket, ISocket::Ptr backendSocket) 
	: m_frontendSocket(std::move(frontendSocket)), m_backendSocket(std::move(backendSocket)) {
	
}

void ProxySocket::Close(bool flush) {
	if (m_frontendSocket)
		m_frontendSocket->Close(flush);

	if (m_backendSocket)
		m_backendSocket->Close(flush);
}

void ProxySocket::Update() {
	if (m_frontendSocket)
		m_frontendSocket->Update();

	if (m_backendSocket)
		m_backendSocket->Update();
}

// Where should this packet go?
//	Am I the frontend or the backend?
//	This is the problem with a single application approach
//	2 Isolated applications would fix this
void ProxySocket::Send(BYTES_t bytes) {
	assert(false);

	// send to client???
	//aahahahsk

}

std::optional<BYTES_t> ProxySocket::Recv() {
	assert(false);
	return {};
}
