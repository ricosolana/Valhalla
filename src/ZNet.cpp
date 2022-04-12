#include "ZNet.hpp"
#include "Game.hpp"
#include "ScriptManager.hpp"
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
	pkg.Write(Game::VERSION);
	pkg.Write(Vector3());
	pkg.Write(Game::Get()->m_playerProfile->m_playerName);
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

			Game::Get()->RunTaskLater([this](Task *task) {
				m_peer->m_rpc->Register("PeerInfo", new ZRpcMethod(this, &ZNet::RPC_PeerInfo));
				m_peer->m_rpc->Register("Disconnect", new ZRpcMethod(this, &ZNet::RPC_Disconnect));
				m_peer->m_rpc->Register("Error", new ZRpcMethod(this, &ZNet::RPC_Error));
				m_peer->m_rpc->Register("ClientHandshake", new ZRpcMethod(this, &ZNet::RPC_ClientHandshake));

				Game::Get()->m_playerProfile = std::make_unique<PlayerProfile>();
				m_zdoMan = std::make_unique<ZDOMan>();

				m_peer->m_rpc->Invoke("ServerHandshake");
			}, 1s);
		}
		else {
			LOG(ERROR) << "Failed to connect: " << ec.message();

			Game::Get()->RunTask([this](Task* task) {
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
	m_peer->m_socket->Close();

	StopIOThread();
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

void ZNet::RPC_PeerInfo(ZRpc* rpc, ZPackage pkg) {
	auto num = pkg.Read<UID_t>();
	auto ver = pkg.Read<std::string>();
	auto endPointString = std::string(); // = m_peer->m_socket->GetEndPointString();
	auto hostName = m_peer->m_socket->GetHostName();
	LOG(INFO) << "VERSION check their:" << ver << "  mine:" << Game::VERSION;
	if (ver != std::string(Game::VERSION)) {
		m_connectionStatus = ConnectionStatus::ErrorVersion;
		
		LOG(INFO) << "Incompatible versions";
		return;
	}
	Vector3 refPos = pkg.Read<Vector3>();
	std::string name = pkg.Read<std::string>();

	//ZNet.m_world = new World();
	//ZNet.m_world.m_name = pkg.ReadString();
	//ZNet.m_world.m_seed = pkg.ReadInt();
	//ZNet.m_world.m_seedName = pkg.ReadString();
	//ZNet.m_world.m_uid = pkg.ReadLong();
	//ZNet.m_world.m_worldGenVersion = pkg.ReadInt();
	//WorldGenerator.Initialize(ZNet.m_world);
	//this.m_netTime = pkg.ReadDouble();
	//
	//peer.m_refPos = refPos;
	//peer.m_uid = num;
	//peer.m_playerName = text2;
	//rpc->Register("RefPos", new ZRpcMethod(this, &ZNet::RPC_RefPos));
	//rpc->Register("PlayerList", new ZRpcMethod(this, &ZNet::RPC_PlayerList));
	//rpc->Register("RemotePrint", new ZRpcMethod(this, &ZNet::RPC_RemotePrint));
	//
	//rpc.Register<double>("NetTime", new Action<ZRpc, double>(this.RPC_NetTime));
	
	m_connectionStatus = ConnectionStatus::Connected;
	
	//m_zdoMan.AddPeer(peer);
	//m_routedRpc.AddPeer(peer);
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
