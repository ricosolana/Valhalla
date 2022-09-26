#include "ZRpc.h"
#include "ValhallaServer.h"

ZRpc::ZRpc(ZSocket2::Ptr socket)
	: m_socket(socket), m_lastPing(std::chrono::steady_clock::now() + 3s) {

	// pinger
	this->m_pingTask = Valhalla()->RunTaskLaterRepeat([this](Task* task) {
		ZPackage pkg;
		pkg.Write<int32_t>(0);
		pkg.Write(false);
		SendPackage(std::move(pkg));
	}, 3s, 1s);
}

ZRpc::~ZRpc() {
	LOG(DEBUG) << "~ZRpc()";
	if (m_pingTask)
		this->m_pingTask->Cancel();
}

bool ZRpc::IsConnected() {
	return m_socket->IsConnected();
}

void ZRpc::Register(const char* name, ZMethodBase<ZRpc*>* method) {
	auto stableHash = Utils::GetStableHashCode(name);

#ifndef _NDEBUG
	if (m_methods.find(stableHash) != m_methods.end())
		throw std::runtime_error("Hash collision");
#endif

	m_methods.insert({ stableHash, std::unique_ptr<ZMethodBase<ZRpc*>>(method) });
}

void ZRpc::Update() {
	auto now(std::chrono::steady_clock::now());

	while (m_socket->HasNewData()) {
		auto pkg = m_socket->Recv();
		auto hash = pkg.Read<int32_t>();
		if (hash == 0) {
			if (pkg.Read<bool>()) {
				// Reply to the server with a pong
				pkg.GetStream().Clear();
				pkg.Write<int32_t>(0);
				pkg.Write(false);
				SendPackage(std::move(pkg));
			}
			else {
				m_lastPing = now;
			}
		}
		else {
			auto&& find = m_methods.find(hash);
			if (find != m_methods.end()) {
				find->second->Invoke(this, pkg);
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

void ZRpc::SendPackage(ZPackage pkg) {
	//this.m_sentPackages++;
	//this.m_sentData += pkg.Size();
	m_socket->Send(std::move(pkg));
}
