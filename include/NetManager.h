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

class INetManager {
private:
    std::unique_ptr<IAcceptor> m_acceptor;

    std::vector<std::unique_ptr<NetRpc>> m_joining; // used to temporarily connecting peers (until PeerInfo)
    std::vector<std::unique_ptr<NetPeer>> m_peers;

    World* m_world;

    bool m_hasPassword = false;
    std::string m_salt;
    std::string m_saltedPassword;

private:
    void InitPassword();

    void RemotePrint(NetRpc* rpc, const std::string& s);

    void Kick(NetPeer* peer);

    void Kick(const std::string& user);

    void Ban(const std::string& user);

    void Unban(const std::string& user);

    void SendDisconnect(NetPeer* peer);

    void SendDisconnect();

    void SendPlayerList();

    void SendNetTime();

    void SendPeerInfo(NetRpc* rpc);

    void RPC_PeerInfo(NetRpc* rpc, NetPackage pkg);

public:
    void RemotePrint(NetRpc* rpc, const std::string& text);

    void Init();
    void Update();
    void Close();

    NetPeer* GetPeer(NetRpc* rpc);
    NetPeer* GetPeer(const std::string& name);
    NetPeer* GetPeer(OWNER_t uuid);

    const std::vector<std::unique_ptr<NetPeer>>& GetPeers();

    //static constexpr Vector3 REFERENCE_POS(1000000, 0, 1000000);
};

INetManager* NetManager();
