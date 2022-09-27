#pragma once

#include "ZNetPeer.h"
#include "ZDOMan.h"
#include "ZRoutedRpc.h"
#include "World.h"
#include "ZAcceptor.h"

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
	//static constexpr const char* VALHEIM_PORT = "2456";

	asio::io_context m_ctx;
	std::unique_ptr<IAcceptor> m_acceptor;
	std::vector<std::unique_ptr<ZNetPeer>> m_peers;

	//ConnectionStatus m_connectionStatus = ConnectionStatus::None;

	void RPC_ServerHandshake(ZRpc* rpc);
	void RPC_PeerInfo(ZRpc* rpc, ZPackage pkg);

	void RPC_Disconnect(ZRpc* rpc);

	void SendPeerInfo(ZRpc* rpc);
public:
	ZNet(uint16_t port);

	std::unique_ptr<ZRoutedRpc> m_routedRpc;
	std::unique_ptr<ZDOMan> m_zdoMan;
	std::unique_ptr<World> m_world;
	double m_netTime;

	void Listen();
	void Update();

	ZNetPeer* GetPeer(ZRpc* rpc);
	void SendPeerInfo();
	int64_t GetUID();
};
