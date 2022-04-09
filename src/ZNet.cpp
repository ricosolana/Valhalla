#include "ZNet.hpp"
#include "Game.hpp"

void ZNet::StopIOThread() {
	m_ctx.stop();

	if (m_ctxThread.joinable())
		m_ctxThread.join();

	m_ctx.restart();
}

void ZNet::OnNewConnection() {

}


void ZNet::SendPeerInfo(std::string_view password) {
	//auto pkg = new ZPackage();
	//pkg->Write(this.GetUID());
	//pkg->Write(Game::VERSION);
	//pkg->Write(this.m_referencePosition);
	//pkg->Write(Game.instance.GetPlayerProfile().GetName());
	//
	//string data = (string.IsNullOrEmpty(password) ? "" : ZNet.HashPassword(password));
	//zpackage.Write(data);
	//byte[] dummyTicket = new byte[1024];
	//zpackage.Write(dummyTicket);
	//
	//rpc.Invoke("PeerInfo", new object[]{ zpackage });
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
				//m_peer->m_rpc->Register("PeerInfo", new ZRpcMethod(this, &ZNet::RPC_PeerInfo));
				m_peer->m_rpc->Register("Disconnect", new ZRpcMethod(this, &ZNet::RPC_Disconnect));
				//m_peer->m_rpc->Register("Error", new Method(this, &RPC_Error));
				m_peer->m_rpc->Register("ClientHandshake", new ZRpcMethod(this, &ZNet::RPC_ClientHandshake));
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

	}
	else {
		SendPeerInfo("");
	}
}

void ZNet::RPC_PeerInfo(ZRpc* rpc, ZPackage pkg) {

}

void ZNet::RPC_Disconnect(ZRpc *rpc) {
	LOG(INFO) << "RPC_Disconnect";
	Disconnect();
}
