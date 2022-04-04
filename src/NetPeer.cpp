#include "NetPeer.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <memory>
#include "NetRpc.hpp"

namespace Alchyme {
	namespace Net {
		Peer::Peer(AsioSocket::Ptr socket)
			: m_isServer(Game::Get()->m_isServer),
			m_socket(socket),
			m_rpc(std::make_unique<Rpc>(socket))
		{}

		void Peer::Update() {
			m_rpc->Update(this);
		}

		void Peer::Disconnect() {
			m_socket->Close();
		}

		void Peer::DisconnectLater() {
			Game::Get()->DisconnectLater(this);
		}

		bool Peer::IsOnline() {
			return m_socket->IsOnline();
		}
	}
}
