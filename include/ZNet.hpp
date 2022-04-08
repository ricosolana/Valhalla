#pragma once

#include "ZNetPeer.hpp"
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

class ZNet {
	static constexpr const char* VALHEIM_PORT = "2456";

	std::thread m_ctxThread;
	asio::io_context m_ctx;

	std::unique_ptr<ZNetPeer> m_peer;

	void StopIOThread();

	void OnNewConnection();
	void SendPeerInfo(std::string_view password);

	void RPC_ClientHandshake(ZRpc* rpc, bool needPassword);
	void RPC_PeerInfo(ZRpc* rpc, ZPackage pkg);
	void RPC_Disconnect(ZRpc* rpc);

public:
	void Connect(std::string host, std::string port);
	void Disconnect();
	void Update();
};
