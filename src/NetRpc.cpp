#include <optick.h>

#include "NetRpc.h"
#include "ValhallaServer.h"
#include "NetManager.h"
#include "ModManager.h"

using namespace std::chrono;

NetRpc::NetRpc(ISocket::Ptr socket)
	: m_socket(std::move(socket)), m_lastPing(steady_clock::now()) {
}

NetRpc::~NetRpc() {
	LOG(DEBUG) << "~NetRpc()";
}

void NetRpc::Register(HASH_t hash, IMethod<NetRpc*>* method) {
	assert(!m_methods.contains(hash)
		&& "runtime rpc hash collision");

	m_methods.insert({ hash, std::unique_ptr<IMethod<NetRpc*>>(method) });
}

void NetRpc::Update() {
	OPTICK_EVENT();

	auto now(steady_clock::now());

	// Send packet data
	m_socket->Update();

	if (m_closeEventually) {
		return;
	}

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
				pkg.Write(false);
				SendPackage(pkg);
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
				find->second->Invoke(this, pkg,
                                     ModManager::GetCallbacks().m_onRpc[hash]);
			}
			else {
#ifdef RPC_DEBUG
				LOG(INFO) << "Client tried invoking unknown RPC: " << name;
#else
				LOG(INFO) << "Client tried invoking unknown RPC: " << hash;
#endif
				//m_socket->Close();
			}
		}
	}
	
	if (now - m_lastPing > Valhalla()->Settings().socketTimeout) {
		LOG(INFO) << "Client RPC timeout";
		m_socket->Close(false);
	}
}

void NetRpc::SendError(ConnectionStatus status) {
	LOG(INFO) << "Client error: " << STATUS_STRINGS[(int)status];
	Invoke("Error", status);
	m_closeEventually = true;
	//m_socket->Close();
}
