#pragma once

#include "NetSocket.hpp"
#include "NetRpc.hpp"

enum class ConnectMode : uint8_t {
	NONE,
	STATUS,
	LOGIN,
};

namespace Alchyme {
	namespace Net {
		struct Peer {
			NOTNULL AsioSocket::Ptr m_socket;
			NULLABLE std::unique_ptr<Rpc> m_rpc;

			const bool m_isServer;
			UID m_uid = 0;
			std::string m_name;
			bool m_authorized = false;

			Peer(AsioSocket::Ptr socket);

			void Update();

			void Disconnect();
			void DisconnectLater();
			bool IsOnline();
		};
	}
}
