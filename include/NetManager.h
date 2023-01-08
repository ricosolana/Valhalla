#pragma once

#include "NetPeer.h"
#include "NetAcceptor.h"
#include "Vector.h"

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
	void Update();
	void Close();

	NetPeer* GetPeer(NetRpc* rpc);
	NetPeer* GetPeer(const std::string& name);
	NetPeer* GetPeer(OWNER_t uuid);

	const std::vector<std::unique_ptr<NetPeer>> &GetPeers();

    //static constexpr Vector3 REFERENCE_POS(1000000, 0, 1000000);
}
