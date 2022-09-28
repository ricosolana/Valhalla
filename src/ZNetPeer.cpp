#include "ZNetPeer.h"
#include "ZNet.h"
#include <memory>

//ZNetPeer::ZNetPeer(ISocket::Ptr sock) :
//	m_socket(sock),
//	m_rpc(std::make_unique<ZRpc>(sock))
//{}

ZNetPeer::ZNetPeer(std::unique_ptr<ZRpc> rpc) :
	m_rpc(std::move(rpc))
{}
