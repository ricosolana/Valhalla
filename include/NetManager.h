#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <memory>
#include <utility>

#include "Peer.h"
#include "NetAcceptor.h"

class INetManager {
    friend class IModManager;

private:
    avledet::util::Map<std::string, std::int32_t, ankerl::unordered_dense::string_hash> m_sessionIndexes;    
    std::vector<std::unique_ptr<Peer>> m_connectedPeers;
    std::vector<Peer*> m_onlinePeers;    
    std::list<std::pair<std::string, std::pair<std::chrono::nanoseconds, std::chrono::nanoseconds>>> m_sortedSessions;
    std::unique_ptr<IAcceptor> m_acceptor;    

public:
    std::string m_passwordHash;
    std::string m_passwordSalt;

private:
    void SendDisconnect();
    void SendPlayerList();
    void SendNetTime();
    void SendPeerInfo(Peer &peer);

    void OnPeerQuit(Peer& peer);
    void OnPeerDisconnect(Peer& peer);

public:
    void PostInit();
    void Update();
    void Uninit();
    
    void OnConfigLoad(bool reloading);

    // Finds a peer by either name, uuid or host
    Peer* GetPeer(std::string_view any);
    Peer* GetPeerByUserID(avledet::util::UserID uuid);
    Peer* GetPeerByName(std::string_view name);
    Peer* GetPeerByHost(std::string_view host);

    void OnPeerConnect(Peer& peer);

    // Kick a player by identifier
    Peer* Kick(std::string_view user);

    // Ban a player by identifier
    Peer* Ban(std::string_view user);

    // Unban a player by identifier
    bool Unban(std::string_view user);

    const auto& GetPeers() {
        return m_onlinePeers;
    }
};

// Manager class for everything related to networking at a mildly abstracted level
INetManager* NetManager();
