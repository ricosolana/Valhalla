#include "ZNet.h"
#include "Game.h"
#include "ScriptManager.h"
#include <openssl/md5.h>

void ZNet::StopIOThread() {
	m_ctx.stop();

	if (m_ctxThread.joinable())
		m_ctxThread.join();

	m_ctx.restart();
}

void ZNet::SendPeerInfo(std::string_view password) {
	ZPackage pkg;
	pkg.Write(GetUID());
	pkg.Write(ValhallaServer::VERSION);
	pkg.Write(Vector3());
	pkg.Write(Valhalla()->m_playerProfile->m_playerName);
	//TODO hash not being written correctly or something
	// compare written bytes to received bytes with debug
	std::string pw = password.empty() ? "" : std::string(reinterpret_cast<char*>(
		MD5(reinterpret_cast<const unsigned char*>(password.data()), password.length(), nullptr)), 16);
	pkg.Write(pw);
	byte dummy[1024];
	pkg.Write(dummy, sizeof(dummy));
	//string data = (string.IsNullOrEmpty(password) ? "" : ZNet.HashPassword(password));
	//zpackage.Write(data);
	//byte[] dummyTicket = new byte[1024];
	//zpackage.Write(dummyTicket);

	m_peer->m_rpc->Invoke("PeerInfo", std::move(pkg));
}

void ZNet::Connect(std::string host, std::string port) {
	if (m_peer)
		return;

	if (port.empty()) port = VALHEIM_PORT;

	m_peer = std::make_unique<ZNetPeer>(std::make_shared<ZSocket2>(m_ctx));

	asio::ip::tcp::resolver resolver(m_ctx);
	auto endpoints = resolver.resolve(asio::ip::tcp::v4(), host, port);

	LOG(INFO) << "Connecting...";

	asio::async_connect(
		m_peer->m_socket->GetSocket(), endpoints.begin(), endpoints.end(),
		[this](const asio::error_code& ec, asio::ip::tcp::resolver::results_type::iterator it) {

		if (!ec) {
			m_peer->m_socket->Accept();

			auto m = new ZMethod(this, &ZNet::RPC_PeerInfo);

			Valhalla()->RunTaskLater([this](Task *task) {
				m_routedRpc = std::make_unique<ZRoutedRpc>(m_peer.get());
				m_zdoMan = std::make_unique<ZDOMan>(m_peer.get());

				m_routedRpc->SetUID(m_zdoMan->GetMyID());

				m_peer->m_rpc->Register("PeerInfo", new ZMethod(this, &ZNet::RPC_PeerInfo));
				m_peer->m_rpc->Register("Disconnect", new ZMethod(this, &ZNet::RPC_Disconnect));
				m_peer->m_rpc->Register("Error", new ZMethod(this, &ZNet::RPC_Error));
				m_peer->m_rpc->Register("ClientHandshake", new ZMethod(this, &ZNet::RPC_ClientHandshake));

				Valhalla()->m_playerProfile = std::make_unique<PlayerProfile>();

				m_peer->m_rpc->Invoke("ServerHandshake");
			}, 1s);
		}
		else {
			LOG(ERROR) << "Failed to connect: " << ec.message();

			Valhalla()->RunTask([this](Task* task) {
				m_peer.reset();
				StopIOThread();
			});
		}
	});

	m_ctxThread = std::thread([this]() {
		el::Helpers::setThreadName("io");
		m_ctx.run();
	});

	#if defined(_WIN32)// && !defined(_NDEBUG)
		void* pThr = m_ctxThread.native_handle();
		SetThreadDescription(pThr, L"IO Thread");
	#endif
}

void ZNet::Disconnect() {
	if (m_peer) {
		m_peer->m_socket->Close();

		StopIOThread();
	}
}

void ZNet::Update() {
	if (m_peer) {
		if (m_peer->m_rpc->IsConnected()) {
			m_peer->m_rpc->Update();
		}
		else {
			LOG(INFO) << "Disconnected";
			m_peer.reset();
			StopIOThread();
		}
	}
}

void ZNet::RPC_ClientHandshake(ZRpc* rpc, bool needPassword) {
	LOG(INFO) << "Client Handshake";
	if (needPassword) {
		// then display password screen
		ScriptManager::Event::OnPreLogin();
	}
	else {
		SendPeerInfo("");
	}
}

// The server send peer info once the client is completely validated
void ZNet::RPC_PeerInfo(ZRpc* rpc, ZPackage pkg) {
	auto num = pkg.Read<UID_t>();
	auto ver = pkg.Read<std::string>();
	auto endPointString = std::string(); // = m_peer->m_socket->GetEndPointString();
	auto hostName = m_peer->m_socket->GetHostName();
	LOG(INFO) << "VERSION check their:" << ver << "  mine:" << ValhallaServer::VERSION;
	if (ver != std::string(ValhallaServer::VERSION)) {
		m_connectionStatus = ConnectionStatus::ErrorVersion;
		
		LOG(INFO) << "Incompatible versions";
		return;
	}
	m_peer->m_refPos = pkg.Read<Vector3>();
	m_peer->m_uid = num;
	m_peer->m_playerName = pkg.Read<std::string>();

	//ZNet.m_world = new World();
	m_world.m_name = pkg.Read<std::string>();
	m_world.m_seed = pkg.Read<int32_t>();
	m_world.m_seedName = pkg.Read<std::string>();
	m_world.m_uid = pkg.Read<UID_t>();
	m_world.m_worldGenVersion = pkg.Read<int32_t>();
	//WorldGenerator.Initialize(ZNet.m_world);

	m_netTime = pkg.Read<double>();

	//rpc->Register("RefPos", new ZMethod(this, &ZNet::RPC_RefPos));
	//rpc->Register("PlayerList", new ZMethod(this, &ZNet::RPC_PlayerList));
	//rpc->Register("RemotePrint", new ZMethod(this, &ZNet::RPC_RemotePrint));
	
	//rpc->Register("NetTime", new ZMethod(this, &ZNet::RPC_NetTime));
	
	m_connectionStatus = ConnectionStatus::Connected;
}

void ZNet::RPC_Error(ZRpc* rpc, int32_t error) {
	m_connectionStatus = static_cast<ConnectionStatus>(error);
	const char* str = error < (int32_t)ConnectionStatus::MAX ? STATUS_STRINGS[error] : "invalid error code";
	LOG(ERROR) << "Got connection error: " << error << " (" << str << ")";
}

void ZNet::RPC_Disconnect(ZRpc *rpc) {
	LOG(INFO) << "RPC_Disconnect";
	Disconnect();
}

int64_t ZNet::GetUID()
{
	return m_zdoMan->GetMyID();
}
