#pragma once

#include "ZSocket.hpp"
#include "ZRpc.hpp"
#include "ZDOID.hpp"

struct ZNetPeer {
	ZNetPeer(ZSocket2::Ptr socket);

	bool IsReady() {
		return m_uid != 0L;
	}

	std::unique_ptr<ZRpc> m_rpc;
	ZSocket2::Ptr m_socket;
	int64_t m_uid = 0;
	bool m_server = false;
	Vector3 m_refPos;
	bool m_publicRefPos = false;
	ZDOID m_characterID = ZDOID::NONE;
	std::string m_playerName;
};
