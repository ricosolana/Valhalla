#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "NetAcceptor.h"
#include "Peer.h"
#include "Types.h"

class INetManager
{
    friend class IScriptManager;

  private:
    avledet::util::Map<std::string, std::int32_t, ankerl::unordered_dense::string_hash> m_sessionIndexes;
    std::vector<Peer::Ptr>
            m_connectedPeers;// TODO condense this down to 1 list, use m_online BOOL instead / flag
    std::vector<Peer::Ptr> m_onlinePeers;// TODO remove this (use above^)
    std::list<std::pair<std::string, std::pair<std::chrono::nanoseconds, std::chrono::nanoseconds>>>
            m_sortedSessions;
    std::unique_ptr<IAcceptor> m_acceptor;

  public:
    std::string m_passwordHash;
    std::string m_passwordSalt;

  private:
    void SendDisconnect();
    void SendPlayerList();
    void SendNetTime();
    void SendPeerInfo(Peer::Ptr peer);

    void OnPeerQuit(Peer::Ptr peer);
    void OnPeerDisconnect(Peer::Ptr peer);

  public:
    void PostInit();
    void Update();
    void Uninit();

    void OnConfigLoad(bool reloading);

    // Finds a peer by either name, uuid or host
    Peer::Ptr FindPeer(std::string_view any);
    Peer::Ptr FindPeerByUserID(avledet::util::UserID uuid);
    Peer::Ptr FindPeerByName(std::string_view name);
    Peer::Ptr FindPeerByHost(std::string_view host);

    void OnPeerConnect(Peer::Ptr peer);

    // Kick a player by identifier
    Peer::Ptr Kick(std::string_view user);

    // Ban a player by identifier
    Peer::Ptr Ban(std::string_view user);

    // Unban a player by identifier
    bool Unban(std::string_view user);

    auto const &GetPeers()
    {
        return m_onlinePeers;
    }
};

// Manager class for everything related to networking at a mildly abstracted level
INetManager *NetManager();
