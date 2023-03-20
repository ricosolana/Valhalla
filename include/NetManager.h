#pragma once

#include <ranges>

#include "Peer.h"
#include "NetAcceptor.h"
#include "Vector.h"
#include "WorldManager.h"
#include "NetRpc.h"
#include "HashUtils.h"

class INetManager {
    friend class IModManager;

private:
    std::unique_ptr<IAcceptor> m_acceptor;

    std::list<std::unique_ptr<NetRpc>> m_rpcs; // used to temporarily connecting peers (until PeerInfo)
    //robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>> m_peers;
    std::list<std::unique_ptr<Peer>> m_peers;

private:
    // Kick a player by name
    bool Kick(std::string user);

    // Ban a player by name
    bool Ban(std::string user);

    // Unban a player by name
    bool Unban(const std::string& user);

    void SendDisconnect();
    void SendPlayerList();
    void SendNetTime();
    void SendPeerInfo(Peer &peer);

    void CleanupPeer(Peer& peer);

    //void RPC_PeerInfo(NetRpc* rpc, BYTES_t bytes);

public:
    void Init();
    void Update();
    void Uninit();

    Peer* GetPeer(const std::string& name);
    Peer* GetPeer(OWNER_t uuid);
    std::vector<Peer*> GetPeers(const std::string &addr);

    //const robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>>& GetPeers();

    //void OnNewPeer(std::unique_ptr<Peer> peer);

    void OnNewClient(ISocket::Ptr socket, OWNER_t uuid, const std::string& name, const Vector3 &pos);

    //void OnNewClient(NetRpc* rpc, OWNER_t uuid, const std::string& name, const Vector3& pos);

    const auto& GetPeers() {
        return m_peers;
    }
};

// Manager class for everything related to networking at a mildly abstracted level
INetManager* NetManager();
