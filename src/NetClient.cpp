#include "NetClient.hpp"
#include "ValhallaGame.hpp"

void Client::StopIOThread() {
	m_ctx.stop();

	if (m_ctxThread.joinable())
		m_ctxThread.join();

	m_ctx.restart();
}

void Client::OnNewConnection() {

}


void Client::SendPeerInfo(std::string& password) {

}

void Client::Connect(std::string host, std::string port) {
	if (m_peer)
		return;

	m_peer = std::make_unique<Peer>(std::make_shared<Socket2>(m_ctx));

	LOG(INFO) << "Non garbage: " << m_peer->m_rpc->not_garbage;

	asio::ip::tcp::resolver resolver(m_ctx);
	auto endpoints = resolver.resolve(asio::ip::tcp::v4(), host, port);

	LOG(INFO) << "Connecting...";

	asio::async_connect(
		m_peer->m_socket->GetSocket(), endpoints.begin(), endpoints.end(),
		[this](const asio::error_code& ec, asio::ip::tcp::resolver::results_type::iterator it) {

		if (!ec) {
			m_peer->m_socket->Accept();

			Game::Get().RunTask([this] {
				m_peer->m_rpc->Register("PeerInfo", new Method(this, &Client::RPC_PeerInfo));
				m_peer->m_rpc->Register("Disconnect", new Method(this, &Client::RPC_Disconnect));
				//m_peer->m_rpc->Register("Error", new Method(this, &RPC_Error));
				m_peer->m_rpc->Register("ClientHandshake", new Method(this, &Client::RPC_ClientHandshake));
				m_peer->m_rpc->Invoke("ServerHandshake");
			});
		}
		else {
			LOG(ERROR) << "Failed to connect: " << ec.message();

			Game::Get().RunTask([this]() {
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

void Client::Disconnect() {
	m_peer->m_socket->Close();

	StopIOThread();
}

void Client::Update() {
	if (m_peer) {
		if (m_peer->m_rpc->IsConnected()) {
			m_peer->m_rpc->Update();
		}
		else {
			m_peer.reset();
		}
	}
}

void Client::RPC_ClientHandshake(Rpc* rpc, bool needPassword) {
	LOG(INFO) << "Client Handshake";
}

void Client::RPC_PeerInfo(Rpc* rpc, Package pkg) {

}

void Client::RPC_Disconnect(Rpc *rpc) {
	LOG(INFO) << "RPC_Disconnect";
	Disconnect();
}
