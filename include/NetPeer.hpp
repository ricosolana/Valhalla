#pragma once

#include "NetSocket.hpp"
#include "NetRpc.hpp"

class Peer {
public:
	NOTNULL Socket2::Ptr m_socket;
	NULLABLE std::unique_ptr<Rpc> m_rpc;

	UID m_uid = 0;
	std::string m_name;
	bool m_authorized = false;

	Peer(Socket2::Ptr socket);
};
