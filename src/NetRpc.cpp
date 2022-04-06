#include "NetRpc.hpp"
#include "NetPeer.hpp"

Rpc::Rpc(Socket2::Ptr socket)
	: m_socket(socket) {
	this->not_garbage = 69420;
}

Rpc::~Rpc() {
	LOG(DEBUG) << "~Rpc()";
}

bool Rpc::IsConnected() {
	return m_socket->IsOnline();
}

void Rpc::Register(const char* name, IMethod* method) {
	auto stableHash = Utils::GetStableHashCode(name);

#ifndef _NDEBUG
	if (m_methods.find(stableHash) != m_methods.end())
		throw std::runtime_error("Hash collision, or most likely duplicate RPC name registered");
#endif

	m_methods.insert({ stableHash, std::unique_ptr<IMethod>(method) });
}

void Rpc::Update() {
	std::unique_ptr<Package> pkg; // m_rpc->Update(this);
	while (pkg = std::unique_ptr<Package>(m_socket->Recv())) {
		int hash = pkg->Read<int>();
		if (hash == 0) {
			pkg->Read<Package>();
		}
		else {
			auto&& find = m_methods.find(hash);
			if (find != m_methods.end()) {
				find->second->Invoke(this, pkg.get());
			}
			else {
				LOG(DEBUG) << "Remote tried invoking unknown function (corrupt or malicious)";
				m_socket->Close();
			}
		}
	}
}
