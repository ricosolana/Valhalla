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

struct ban_info {
	std::chrono::steady_clock::time_point issued;
	std::chrono::steady_clock::time_point expiry;
	std::string reason;
};

class ZNet {
	asio::io_context m_ctx;
	std::unique_ptr<IAcceptor> m_acceptor;

	std::vector<std::unique_ptr<ZRpc>> m_joining;
	std::vector<ZNetPeer::Ptr> m_peers;

	robin_hood::unordered_map<std::string, ban_info> m_banList;

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
	
	void Kick(const std::string &user);
	void Kick(ZNetPeer::Ptr peer);
	void Ban(const std::string &user, std::chrono::minutes dur = 10000h, const std::string &reason = "Banned for misuse");
	//void Ban(ZNetPeer::Ptr peer, std::chrono::minutes dur = 10000h, std::string reason = "Banned for misuse");
	void Unban(const std::string &user);
	//void Unban(ZNetPeer::Ptr peer);

	void SendPlayerList();
	void SendNetTime();
	void RemotePrint(ZRpc* rpc, const std::string &s);
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
	ZNetPeer::Ptr GetPeer(const std::string& name);
	ZNetPeer::Ptr GetPeer(uuid_t uuid);
	//ZNetPeer::Ptr GetPeer(std::string &ip) ; // get by address
												// might return multiple peers

	//ZNetPeer* GetJoining(ZRpc* rpc);
	uuid_t GetUID();
};
