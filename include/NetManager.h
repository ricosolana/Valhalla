#pragma once

#include <ranges>

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
    //robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>> m_peers;
    std::list<std::unique_ptr<Peer>> m_peers;

    //World* m_world;

    bool m_hasPassword = false;
    std::string m_salt;
    std::string m_saltedPassword;

private:
    void InitPassword();

    // Kick a player by name
    bool Kick(std::string user, const std::string& reason);

    // Ban a player by name
    bool Ban(std::string user, const std::string& reason);

    // Unban a player by name
    bool Unban(const std::string& user);

    //void SendDisconnect(Peer* peer);

    void SendDisconnect();

    void SendPlayerList();

    void SendNetTime();

    void SendPeerInfo(Peer *peer);

    void RPC_PeerInfo(NetRpc* rpc, BYTES_t bytes);

public:
    void Init();
    void Update();
    void Uninit();

    Peer* GetPeer(const std::string& name);
    Peer* GetPeer(OWNER_t uuid);
    std::vector<Peer*> GetPeers(const std::string &addr);

    //const robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>>& GetPeers();

    const auto &GetPeers() {
        return m_peers;
    }
};

// Manager class for everything related to networking at a mildly abstracted level
INetManager* NetManager();
