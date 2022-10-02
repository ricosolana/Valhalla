#pragma once

#include "NetPeer.h"
#include "ZDOMan.h"
#include "NetRpcManager.h"
#include "World.h"
#include "NetAcceptor.h"

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

namespace NetManager {

	void RemotePrint(ZRpc* rpc, const std::string &text);

	void Listen(uint16_t port);
	void Update(double delta);
	void Close();

	ZNetPeer::Ptr GetPeer(ZRpc* rpc);
	ZNetPeer::Ptr GetPeer(const std::string& name);
	ZNetPeer::Ptr GetPeer(uuid_t uuid);

	const std::vector<ZNetPeer::Ptr> &GetPeers();
}
