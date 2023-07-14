#pragma once

#include <ranges>

#include "Peer.h"
#include "NetAcceptor.h"
#include "Vector.h"
#include "WorldManager.h"
#include "HashUtils.h"

class INetManager {
    friend class IModManager;
    friend class TCPSocket;
    friend class AcceptorTCP;

private:
    std::unique_ptr<IAcceptor> m_acceptor;

#if VH_IS_ON(VH_PACKET_REDIRECTION)
    //  TODO dont use an acceptor as an asio context...
    //      it confuses the meaning of the code
    // We'll just use a tcp acceptor for now, although only
    // asio specific setups are being used from it
    std::unique_ptr<AcceptorTCP> m_ctxAcceptor;
#endif

    //std::list<std::unique_ptr<Peer>> m_rpcs; // used to temporarily connecting peers (until PeerInfo)
    //std::list<std::unique_ptr<Peer>> m_onlinePeers;

    std::vector<std::unique_ptr<Peer>> m_connectedPeers;
    std::vector<Peer*> m_onlinePeers;

#if VH_IS_ON(VH_PLAYER_CAPTURE)
    std::list<std::pair<std::string, std::pair<nanoseconds, nanoseconds>>> m_sortedSessions;
    UNORDERED_MAP_t<std::string, int32_t, ankerl::unordered_dense::string_hash> m_sessionIndexes;
#endif

//#if VH_IS_ON(VH_PACKET_REDIRECTION_STEAM)
    //std::vector<TCPSocket::Ptr> m_proxies;
    // Plan:
    //  Receive packets from clients (As steam)
    //  Send those packets to the backend server
    //  The backend server will process them
    //  The backend server will send any packets it generates to the Steam server
//#endif

public:
    //std::string m_password;
    //std::string m_salt;

    std::array<char, 16> m_passwordHash;
    std::array<char, 16> m_passwordSalt;

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
    Peer* GetPeerByUUID(OWNER_t uuid);
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
