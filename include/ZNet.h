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
	std::vector<std::unique_ptr<ZRpc>> m_joining;
	std::vector<std::unique_ptr<ZNetPeer>> m_peers;

	//ConnectionStatus m_connectionStatus = ConnectionStatus::None;

	void SendPeerInfo(ZRpc* rpc);

	void RPC_PeerInfo(ZRpc* rpc, ZPackage pkg);
	void RPC_Disconnect(ZRpc* rpc);
	void RPC_ServerHandshake(ZRpc* rpc);

	void RPC_RefPos(ZRpc* rpc, Vector3 pos, bool publicRefPos);
	void RPC_PlayerList(ZRpc* rpc, ZPackage pkg);
	void RPC_RemotePrint(ZRpc* rpc, std::string s);
	void RPC_CharacterID(ZRpc* rpc, ZDOID characterID);
	void RPC_Kick(ZRpc* rpc, std::string user);
	void RPC_Ban(ZRpc* rpc, std::string user);
	void RPC_Unban(ZRpc* rpc, std::string user);
	void RPC_Save(ZRpc* rpc);
	void RPC_PrintBanned(ZRpc* rpc);
	
	void SendPlayerList();
	void UpdatePlayerList();

	
public:
	ZNet(uint16_t port);

	std::unique_ptr<ZRoutedRpc> m_routedRpc;
	std::unique_ptr<ZDOMan> m_zdoMan;
	std::unique_ptr<World> m_world;
	double m_netTime;

	void Listen();
	void Update();

	ZNetPeer* GetPeer(ZRpc* rpc);
	int64_t GetUID();
};
