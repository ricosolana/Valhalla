#pragma once

#include "NetPeer.h"
#include "NetRouteManager.h"
#include "NetAcceptor.h"

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

	void Init();
	void Update(double delta);
	void Close();

	NetPeer* GetPeer(NetRpc* rpc);
	NetPeer* GetPeer(const std::string& name);
	NetPeer* GetPeer(OWNER_t uuid);

	const std::vector<std::unique_ptr<NetPeer>> &GetPeers();
}
