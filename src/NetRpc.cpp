#include <optick.h>

#include "NetRpc.h"
#include "VServer.h"
#include "NetManager.h"

using namespace std::chrono;

NetRpc::NetRpc(ISocket::Ptr socket)
	: m_socket(std::move(socket)), m_lastPing(steady_clock::now()) {
}

NetRpc::~NetRpc() {
	LOG(DEBUG) << "~NetRpc()";
}

void NetRpc::Register(HASH_t hash, MethodPtr method) {
	assert(!m_methods.contains(hash));
	m_methods[hash] = std::move(method);
}

//void NetRpc::InvokeRaw(HASH_t hash, NetPackage params) {
//    if (!m_socket->Connected())
//        return;
//
//    NetPackage pkg; // TODO make into member to optimize; or make static
//    pkg.Write(hash);
//    pkg.m_stream.Write(params.m_stream.m_buf);
//
//    SendPackage(std::move(pkg));
//}

void NetRpc::Update() {
	OPTICK_EVENT();

	auto now(steady_clock::now());

	// Send packet data
	m_socket->Update();

	// Read packets
	while (auto opt = m_socket->Recv()) {
		//assert(pkg && "Got null package and executing!");
		auto&& pkg = opt.value();

		auto hash = pkg.Read<HASH_t>();
		if (hash == 0) {
			if (pkg.Read<bool>()) {
				// Reply to the server with a pong
				pkg.m_stream.Clear();
				pkg.Write<HASH_t>(0);
				pkg.Write<bool>(false);
				SendPackage(std::move(pkg));
			}
			else {
				m_lastPing = now;
			}
		}
		else {
#ifdef RPC_DEBUG
			std::string name = pkg->Read<std::string>();
#endif
			auto&& find = m_methods.find(hash);
			if (find != m_methods.end()) {
				find->second->Invoke(this, pkg);
			}
		}
	}
	
	if (now - m_lastPing > SERVER_SETTINGS.socketTimeout) {
		LOG(INFO) << "Client RPC timeout";
		m_socket->Close(false);
	}
}

void NetRpc::SendError(ConnectionStatus status) {
	LOG(INFO) << "Client error: " << STATUS_STRINGS[(int)status];
	Invoke("Error", status);
	m_socket->Close(true);
}
