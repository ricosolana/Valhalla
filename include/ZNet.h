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
	std::vector<ZNetPeer::Ptr> m_peers;

	//ConnectionStatus m_connectionStatus = ConnectionStatus::None;

	void SendPeerInfo(ZRpc* rpc);

	void RPC_PeerInfo(ZRpc* rpc, ZPackage::Ptr pkg);
	void RPC_Disconnect(ZRpc* rpc);
	void RPC_ServerHandshake(ZRpc* rpc);

	void RPC_RefPos(ZRpc* rpc, Vector3 pos, bool publicRefPos);
	//void RPC_PlayerList(ZRpc* rpc, ZPackage::Ptr pkg);
	//void RPC_RemotePrint(ZRpc* rpc, std::string s);
	void RPC_CharacterID(ZRpc* rpc, ZDOID characterID);
	void RPC_Kick(ZRpc* rpc, std::string user);
	void RPC_Ban(ZRpc* rpc, std::string user);
	void RPC_Unban(ZRpc* rpc, std::string user);
	void RPC_Save(ZRpc* rpc);
	void RPC_PrintBanned(ZRpc* rpc);
	
	void Kick(std::string user);
	void Kick(ZNetPeer::Ptr peer);
	void Ban(std::string user);
	void Ban(ZNetPeer::Ptr peer);
	void Unban(std::string user);
	void Unban(ZNetPeer::Ptr peer);

	void SendPlayerList();
	void SendNetTime();
	void RemotePrint(ZRpc* rpc, std::string &s);
	void SendDisconnect();
	void SendDisconnect(ZNetPeer::Ptr peer);
	void Disconnect(ZNetPeer::Ptr peer);
	
public:
	ZNet(uint16_t port);

	std::unique_ptr<ZRoutedRpc> m_routedRpc;
	std::unique_ptr<ZDOMan> m_zdoMan;
	std::unique_ptr<World> m_world;
	double m_netTime;

	void Listen();
	void Update();

	ZNetPeer::Ptr GetPeer(ZRpc* rpc);
	ZNetPeer::Ptr GetPeer(std::string& name);
	ZNetPeer::Ptr GetPeer(UUID uuid);
	//ZNetPeer::Ptr GetPeer(std::string &ip) ; // get by address
												// might return multiple peers

	//ZNetPeer* GetJoining(ZRpc* rpc);
	int64_t GetUID();
};
