#pragma once

#include <ranges>

#include "Peer.h"
#include "NetAcceptor.h"
#include "Vector.h"
#include "WorldManager.h"
#include "HashUtils.h"

class INetManager {
    friend class IModManager;

private:
    std::unique_ptr<IAcceptor> m_acceptor;

    //std::list<std::unique_ptr<Peer>> m_rpcs; // used to temporarily connecting peers (until PeerInfo)
    //std::list<std::unique_ptr<Peer>> m_onlinePeers;

    std::vector<std::unique_ptr<Peer>> m_connectedPeers;
    std::vector<Peer*> m_onlinePeers;

    std::list<std::pair<std::string, std::pair<nanoseconds, nanoseconds>>> m_sortedSessions;
    UNORDERED_MAP_t<std::string, int32_t, ankerl::unordered_dense::string_hash> m_sessionIndexes;

public:
    //std::string m_password;
    //std::string m_salt;

    std::array<char, 16> m_passwordHash;
    std::array<char, 16> m_passwordSalt;

private:
    void send_disconnect();
    void send_player_list();
    void send_net_time();
    void send_peer_info(Peer &peer);

    void on_peer_quit(Peer& peer);
    void on_peer_disconnect(Peer& peer);

public:
    void post_init();
    void on_update();
    void uninit();
    
    void on_config_load(bool reloading);

    // Finds a peer by either name, uuid or host
    Peer* get_peer(std::string_view any);
    Peer* get_peer_by_userid(USER_ID_t uuid);
    Peer* get_peer_by_name(std::string_view name);
    Peer* get_peer_by_host(std::string_view host);

    void on_peer_connect(Peer& peer);

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
