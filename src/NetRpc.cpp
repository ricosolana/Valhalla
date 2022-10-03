#include "NetRpc.h"
#include "ValhallaServer.h"
#include "NetManager.h"

using namespace std::chrono;

NetRpc::NetRpc(ISocket::Ptr socket)
	: m_socket(socket), m_lastPing(steady_clock::now() + 3s) {

	// pinger
	this->m_pingTask = Valhalla()->RunTaskLaterRepeat([this](Task* task) {
		//NetPackage pkg;
		auto pkg(PKG());
		pkg->Write<int32_t>(0);
		pkg->Write(true);
		SendPackage(pkg);
	}, 3s, 1s);
}

NetRpc::~NetRpc() {
	LOG(DEBUG) << "~NetRpc()";
	if (m_pingTask)
		this->m_pingTask->Cancel();
}

void NetRpc::Register(const char* name, ZMethodBase<NetRpc*>* method) {
	auto stableHash = Utils::GetStableHashCode(name);

	assert(!m_methods.contains(stableHash)
		&& "runtime rpc hash collision");

	m_methods.insert({ stableHash, std::unique_ptr<ZMethodBase<NetRpc*>>(method) });
}

void NetRpc::Update() {
	if (m_ignore)
		return;

	auto now(steady_clock::now());

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
				//m_ping = duration_cast<milliseconds>(now - m_lastPing);
				m_lastPing = now;
			}
		}
		else {
#ifdef RPC_DEBUG
			std::string name = pkg->Read<std::string>();
#endif
			auto&& find = m_methods.find(hash);
			if (find != m_methods.end()) {
				try {
					find->second->Invoke(this, pkg);
				}
				catch (std::exception& e) {
					// close socket because it might have
				}
			}
			else {
#ifdef RPC_DEBUG
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

std::chrono::milliseconds NetRpc::GetPing() {
	//return m_ping.count();
	auto now(steady_clock::now());
	return duration_cast<milliseconds>(now - m_lastPing);
}

void NetRpc::SendError(ConnectionStatus status) {
	LOG(INFO) << "Client error: " << STATUS_STRINGS[(int)status];
	Invoke("Error", status);
	// then disconnect later
	m_ignore = true;
	Valhalla()->RunTaskLater([this](Task*) { m_socket->Close(); }, GetPing() * 2);
}

void NetRpc::SendPackage(NetPackage::Ptr pkg) {
	//this.m_sentPackages++;
	//this.m_sentData += pkg.Size();
	m_socket->Send(pkg);
}
