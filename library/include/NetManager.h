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
    std::vector<std::unique_ptr<Peer>> m_connectedPeers;
    std::vector<Peer *> m_onlinePeers;
    std::list<std::pair<std::string, std::pair<std::chrono::nanoseconds, std::chrono::nanoseconds>>>
            m_sortedSessions;
    std::unique_ptr<IAcceptor> m_acceptor;

  public:
    std::string m_passwordHash;
    std::string m_passwordSalt;

    /*
        packet statistical capture
    */

    std::jthread m_adetect_writer;


  private:
    void SendDisconnect();
    void SendPlayerList();
    void SendNetTime();
    void SendPeerInfo(Peer &peer);

    void OnPeerQuit(Peer &peer);
    void OnPeerDisconnect(Peer &peer);

  public:
    void PostInit();
    void Update();
    void Uninit();

    void OnConfigLoad(bool reloading);

    // Finds a peer by either name, uuid or host
    Peer *FindPeer(std::string_view any);
    Peer *FindPeerByUserID(avledet::util::UserID uuid);
    Peer *FindPeerByName(std::string_view name);
    Peer *FindPeerByHost(std::string_view host);

    void OnPeerConnect(Peer &peer);

    // Kick a player by identifier
    Peer *Kick(std::string_view user);

    // Ban a player by identifier
    Peer *Ban(std::string_view user);

    // Unban a player by identifier
    bool Unban(std::string_view user);

    auto const &GetPeers()
    {
        return m_onlinePeers;
    }
};

// Manager class for everything related to networking at a mildly abstracted level
INetManager *NetManager();
