#include "ZRpc.hpp"
#include "ZNetPeer.hpp"
#include "Game.hpp"

ZRpc::ZRpc(ZSocket2::Ptr socket)
	: m_socket(socket), m_lastPing(std::chrono::steady_clock::now() + 3s) {
	this->not_garbage = 69420;

	// pinger
	this->m_pingTask = Game::Get()->RunTaskLaterRepeat([this](Task* task) {
		auto pkg = new ZPackage();
		pkg->Write<int32_t>(0);
		pkg->Write(false);
		SendPackage(pkg);
	}, 3s, 1s);
}

ZRpc::~ZRpc() {
	LOG(DEBUG) << "~ZRpc()";
	if (m_pingTask)
		this->m_pingTask->Cancel();
}

bool ZRpc::IsConnected() {
	return m_socket->IsOnline();
}

void ZRpc::Register(const char* name, ZRpcMethodBase* method) {
	auto stableHash = Utils::GetStableHashCode(name);

#ifndef _NDEBUG
	if (m_methods.find(stableHash) != m_methods.end())
		throw std::runtime_error("Hash collision");
#endif

	m_methods.insert({ stableHash, std::unique_ptr<ZRpcMethodBase>(method) });
}

void ZRpc::Update() {
	auto now(std::chrono::steady_clock::now());

	std::unique_ptr<ZPackage> pkg;
	while (pkg = std::unique_ptr<ZPackage>(m_socket->Recv())) {
		auto hash = pkg->Read<int32_t>();
		if (hash == 0) {
			if (pkg->Read<bool>()) {
				// Reply to the server with a pong
				pkg->Write<int32_t>(0);
				pkg->Write(false);
				auto ptr = pkg.release();
				SendPackage(ptr);
			}
			else {
				m_lastPing = now;
			}
		}
		else {
			auto&& find = m_methods.find(hash);
			if (find != m_methods.end()) {
				find->second->Invoke(this, pkg.get());
			}
			else {
				LOG(DEBUG) << "Server tried remotely calling unknown RPC";
				m_socket->Close();
			}
		}
	}
	
	if (now - m_lastPing > 30s) {
		LOG(WARNING) << "Server timeout";
		m_socket->Close();
	}

}

void ZRpc::SendPackage(ZPackage* pkg) {
	//this.m_sentPackages++;
	//this.m_sentData += pkg.Size();
	m_socket->Send(pkg);
}
