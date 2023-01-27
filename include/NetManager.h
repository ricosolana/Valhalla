#pragma once

#include "Peer.h"
#include "NetAcceptor.h"
#include "Vector.h"
#include "WorldManager.h"
#include "NetRpc.h"
#include "HashUtils.h"

class INetManager {
private:
    std::unique_ptr<IAcceptor> m_acceptor;

    std::list<std::unique_ptr<NetRpc>> m_rpcs; // used to temporarily connecting peers (until PeerInfo)
    robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>> m_peers;

    //World* m_world;

    bool m_hasPassword = false;
    std::string m_salt;
    std::string m_saltedPassword;

private:
    void InitPassword();

    void Kick(const std::string& user);

    void Ban(const std::string& user);

    void Unban(const std::string& user);

    void SendDisconnect(Peer* peer);

    void SendDisconnect();

    void SendPlayerList();

    void SendNetTime();

    void SendPeerInfo(Peer *peer);

    void RPC_PeerInfo(NetRpc* rpc, NetPackage pkg);

public:
    void Init();
    void Update();
    void Close();

    Peer* GetPeer(const std::string& name);
    Peer* GetPeer(OWNER_t uuid);

    const robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>>& GetPeers();
};

INetManager* NetManager();
