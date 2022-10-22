#pragma once

#include "NetPeer.h"
#include "NetRpcManager.h"
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

	void Start(const std::string& name,
		const std::string& password,
		uint16_t port,
		bool isPublic,
		float timeout);
	void Update(double delta);
	void Close();

	NetPeer::Ptr GetPeer(NetRpc* rpc);
	NetPeer::Ptr GetPeer(const std::string& name);
	NetPeer::Ptr GetPeer(UUID_t uuid);

	const std::vector<NetPeer::Ptr> &GetPeers();
}
