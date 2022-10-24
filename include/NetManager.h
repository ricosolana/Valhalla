#pragma once

#include "NetPeer.h"
#include "NetRouteManager.h"
#include "NetAcceptor.h"
#include "ServerSettings.h"

enum class ConnectionStatus : int32_t {
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

namespace NetManager {
	void RemotePrint(NetRpc* rpc, const std::string &text);

	void Start(const ServerSettings& settings);
	void Update(double delta);
	void Close();

	NetPeer::Ptr GetPeer(NetRpc* rpc);
	NetPeer::Ptr GetPeer(const std::string& name);
	NetPeer::Ptr GetPeer(UUID_t uuid);

	const std::vector<NetPeer::Ptr> &GetPeers();
}
