#include "ZRpc.h"
#include "ValhallaServer.h"

ZRpc::ZRpc(ISocket::Ptr socket)
	: m_socket(socket), m_lastPing(std::chrono::steady_clock::now() + 3s) {

	// pinger
	this->m_pingTask = Valhalla()->RunTaskLaterRepeat([this](Task* task) {
		//ZPackage pkg;
		auto pkg(PKG());
		pkg->Write<int32_t>(0);
		pkg->Write(true);
		SendPackage(pkg);
	}, 3s, 1s);
}

ZRpc::~ZRpc() {
	LOG(DEBUG) << "~ZRpc()";
	if (m_pingTask)
		this->m_pingTask->Cancel();
}

void ZRpc::Register(const char* name, ZMethodBase<ZRpc*>* method) {
	auto stableHash = Utils::GetStableHashCode(name);

	assert(!m_methods.contains(stableHash)
		&& "runtime rpc hash collision");

	m_methods.insert({ stableHash, std::unique_ptr<ZMethodBase<ZRpc*>>(method) });
}

void ZRpc::Update() {
	auto now(std::chrono::steady_clock::now());

	// Process up to 20 packets at a time
	for (int _c = 0; _c < 20 && m_socket->HasNewData(); _c++) {
		auto pkg = m_socket->Recv();
		auto hash = pkg->Read<int32_t>();
		if (hash == 0) {
			if (pkg->Read<bool>()) {
				// Reply to the server with a pong
				pkg->GetStream().Clear();
				pkg->Write<int32_t>(0);
				pkg->Write(false);
				SendPackage(pkg);
			}
			else {
				m_lastPing = now;
			}
		}
		else {
#if TRUE
			std::string name = pkg->Read<std::string>();
#endif
			auto&& find = m_methods.find(hash);
			if (find != m_methods.end()) {
				find->second->Invoke(this, pkg);
			}
			else {
#if TRUE
				LOG(INFO) << "Client tried invoking unknown RPC handler: " << name;
#else
				LOG(INFO) << "Client tried invoking unknown RPC handler";
#endif
				m_socket->Close();
			}
		}
	}
	
	if (now - m_lastPing > 30s) {
		LOG(INFO) << "Client timeout";
		m_socket->Close();
	}

}

void ZRpc::SendPackage(ZPackage::Ptr pkg) {
	//this.m_sentPackages++;
	//this.m_sentData += pkg.Size();
	m_socket->Send(pkg);
}
