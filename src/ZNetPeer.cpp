#include "ZNetPeer.hpp"
#include "ZNet.hpp"
#include <memory>

ZNetPeer::ZNetPeer(ZSocket2::Ptr socket) :
	m_socket(socket),
	m_rpc(std::make_unique<ZRpc>(socket))
{}
