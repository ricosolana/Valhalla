#pragma once

#include "ZSocket.h"
#include "ZRpc.h"
#include "ZDOID.h"

struct ZNetPeer {
	//ZNetPeer(ISocket::Ptr sock);
	ZNetPeer(std::unique_ptr<ZRpc> rpc);

	//bool IsReady() {
	//	return m_uid != 0L;
	//}

	std::unique_ptr<ZRpc> m_rpc;
	//ISocket::Ptr m_socket;

	int64_t m_uid = 0;
	bool m_server = false;
	Vector3 m_refPos;
	bool m_publicRefPos = false;
	ZDOID m_characterID = ZDOID::NONE;
	std::string m_playerName;
};
