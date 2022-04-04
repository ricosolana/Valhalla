#include "NetRpc.hpp"
#include "NetPeer.hpp"

namespace Alchyme {
	namespace Net {
		Rpc::Rpc(AsioSocket::Ptr socket)
			: m_socket(socket) {
			this->not_garbage = 69420;
		}

		Rpc::~Rpc() {
			LOG(DEBUG) << "~Rpc()";
		}

		void Rpc::Register(const char* name, IMethod* method) {
			auto hash = static_cast<RpcHash>(Utils::StrHash(name));

#ifndef _NDEBUG
			if (m_methods.find(hash) != m_methods.end())
				throw std::runtime_error("Hash collision, or most likely duplicate RPC name registered");
#endif

			m_methods.insert({ hash, std::unique_ptr<IMethod>(method) });
		}

		void Rpc::Update(Peer* peer) {
			int max = 100;
			std::unique_ptr<Packet> packet;
			while (packet = std::unique_ptr<Packet>(m_socket->NextPacket())) {
				RpcHash hash; packet->Read(hash);
				// find method in stored
				auto&& find = m_methods.find(hash);
				if (find != m_methods.end()) {
					find->second->Invoke(peer, packet.get());
				}
				else {
					LOG(DEBUG) << "Remote tried invoking unknown function (corrupt or malicious)";
					m_socket->Close();
				}
			}
		}
	}
}
