#pragma once

#include "NetPeer.hpp"
#include "MyRenderInterface.hpp"
#include "MySystemInterface.hpp"
#include "MyFileInterface.hpp"

enum ConnectionStatus
{
	None,
	Connecting,
	Connected,
	ErrorVersion,
	ErrorDisconnected,
	ErrorConnectFailed,
	ErrorPassword,
	ErrorAlreadyConnected,
	ErrorBanned,
	ErrorFull,
};

class Client {
	static constexpr const char* port = "2456";

	std::thread m_ctxThread;
	asio::io_context m_ctx;

	std::unique_ptr<Peer> m_peer;

	void StopIOThread();

	void OnNewConnection();
	void SendPeerInfo(std::string& password);

	void RPC_ClientHandshake(Rpc* rpc, bool needPassword);
	void RPC_PeerInfo(Rpc* rpc, Package pkg);
	void RPC_Disconnect(Rpc* rpc);

public:
	void Connect(std::string host, std::string port);
	void Disconnect();
	void Update();
};
