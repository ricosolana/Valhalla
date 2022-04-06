#include "NetPeer.hpp"
#include "NetClient.hpp"
#include <memory>
#include "NetRpc.hpp"

Peer::Peer(Socket2::Ptr socket) :
	m_socket(socket),
	m_rpc(std::make_unique<Rpc>(socket))
{}
