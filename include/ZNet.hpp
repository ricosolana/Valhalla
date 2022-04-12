#pragma once

#include "ZNetPeer.hpp"
#include "MyRenderInterface.hpp"
#include "MySystemInterface.hpp"
#include "MyFileInterface.hpp"
#include "ZDOMan.hpp"

enum class ConnectionStatus : int32_t
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
	MAX // 10
};

static const char* STATUS_STRINGS[] = { "None", "Connecting", "Connected", 
	"ErrorVersion", "ErrorDisconnected", "ErrorConnectFailed", "ErrorPassword", 
	"ErrorAlreadyConnected", "ErrorBanned", "ErrorFull"};

class ZNet {
	static constexpr const char* VALHEIM_PORT = "2456";

	std::thread m_ctxThread;
	asio::io_context m_ctx;

	ConnectionStatus m_connectionStatus = ConnectionStatus::None;
	std::unique_ptr<ZNetPeer> m_peer;
	std::unique_ptr<ZDOMan> m_zdoMan;

	void StopIOThread();

	void RPC_ClientHandshake(ZRpc* rpc, bool needPassword);
	void RPC_PeerInfo(ZRpc* rpc, ZPackage pkg);

	void RPC_Error(ZRpc* rpc, int32_t error);
	void RPC_Disconnect(ZRpc* rpc);


public:
	void Connect(std::string host, std::string port);
	void Disconnect();
	void Update();

	void SendPeerInfo(std::string_view password);
	int64_t GetUID();
};
