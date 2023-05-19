#include "NetManager.h"
#include "ValhallaServer.h"
#include "WorldManager.h"
#include "VUtilsRandom.h"
#include "Hashes.h"
#include "ZDOManager.h"
#include "RouteManager.h"
#include "ZoneManager.h"
#include "VUtilsResource.h"

using namespace std::chrono;

// TODO use netmanager instance instead

auto NET_MANAGER(std::make_unique<INetManager>());
INetManager* NetManager() {
    return NET_MANAGER.get();
}



Peer* INetManager::Kick(std::string_view user) {
    auto&& peer = GetPeer(user);
    if (peer) {
        peer->Kick();
    }

    return peer;
}

Peer* INetManager::Ban(std::string_view user) {
    auto&& peer = GetPeer(user);

    if (peer) {
        Valhalla()->m_blacklist.insert(peer->m_socket->GetHostName());
        peer->Close(ConnectionStatus::ErrorBanned);
    } else    
        Valhalla()->m_blacklist.insert(user);

    return peer;
}

bool INetManager::Unban(std::string_view user) {
    return Valhalla()->m_blacklist.erase(user);
}



void INetManager::SendDisconnect() {
    LOG_INFO(LOGGER, "Sending disconnect msg");

    for (auto&& peer : m_connectedPeers) {
        peer->SendDisconnect();
    }
}



void INetManager::SendPlayerList() {
    if (!m_onlinePeers.empty()) {
        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(Hashes::Rpc::S2C_UpdatePlayerList);

        writer.SubWrite([this](DataWriter& writer) {
            writer.Write<int32_t>(m_onlinePeers.size());

            for (auto&& peer : m_onlinePeers) {
                writer.Write(peer->m_name);
                writer.Write(peer->m_socket->GetHostName());
                writer.Write(peer->m_characterID);
                writer.Write(peer->m_visibleOnMap || VH_SETTINGS.playerListForceVisible);
                if (peer->m_visibleOnMap || VH_SETTINGS.playerListForceVisible) {
                    if (VH_SETTINGS.playerListSendInterval >= 2s)
                        writer.Write(peer->m_pos);
                    else {
                        auto&& zdo = peer->GetZDO();
                        if (zdo)
                            writer.Write(zdo->Position());
                        else
                            writer.Write(peer->m_pos);
                    }
                }
            }
        });

        for (auto&& peer : m_onlinePeers) {
            peer->Send(bytes);
        }
    }
}

void INetManager::SendNetTime() {
    for (auto&& peer : m_onlinePeers) {
        peer->Invoke(Hashes::Rpc::S2C_UpdateTime, Valhalla()->GetWorldTime());
    }
}



void INetManager::SendPeerInfo(Peer& peer) {
    peer.SubInvoke(Hashes::Rpc::PeerInfo, [](DataWriter& writer) {
        writer.Write(Valhalla()->ID());
        writer.Write(VConstants::GAME);
        writer.Write(VConstants::NETWORK);
        writer.Write(Vector3f::Zero()); // dummy
        writer.Write(""); // dummy

        auto world = WorldManager()->GetWorld();

        writer.Write(world->m_name);
        writer.Write(world->m_seed);
        writer.Write(world->m_seedName); // Peer does not seem to use
        writer.Write(world->m_uid);
        writer.Write(world->m_worldGenVersion);
        writer.Write(Valhalla()->GetWorldTime());
    });
}



//void INetManager::OnNewClient(ISocket::Ptr socket, OWNER_t uuid, const std::string &name, const Vector3f &pos) {
void INetManager::OnPeerConnect(Peer& peer) {
    peer.m_admin = Valhalla()->m_admin.contains(peer.m_socket->GetHostName());

    VH_DISPATCH_WEBHOOK(peer.m_name + " has joined");

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdatePos, [this](Peer* peer, Vector3f pos, bool publicRefPos) {
        peer->m_pos = pos;
        peer->m_visibleOnMap = publicRefPos; // stupid name
    });

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdateID, [this](Peer* peer, ZDOID characterID) {
        // Notes:
        //  Peer sends 0,0 on death
        //  ZDO is not guaranteed to exist when this is called (about 100% of the time)
        
        if (peer->m_characterID)
            VH_DISPATCH_WEBHOOK(peer->m_name + " has died");

        //LOG_INFO(LOGGER, "Player ZDO exists: {}", (ZDOManager()->GetZDO(characterID) == nullptr ? "false" : "true"));

        peer->m_characterID = characterID;

        LOG_INFO(LOGGER, "Got CharacterID from {} ({})", peer->m_name, characterID);
    });

    peer.Register(Hashes::Rpc::C2S_RequestKick, [this](Peer* peer, std::string_view user) {
        // TODO maybe permissions tree in future?
        //  lua? ...
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        if (Kick(user)) {
            peer->ConsoleMessage("Kicked '" + std::string(user) + "'");
            VH_DISPATCH_WEBHOOK(std::string(user) + " was kicked");
        }
        else {
            peer->ConsoleMessage("Player not found");
        }
    });

    peer.Register(Hashes::Rpc::C2S_RequestBan, [this](Peer* peer, std::string_view user) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        if (Ban(user)) {
            peer->ConsoleMessage("Banned '" + std::string(user) + "'");
            VH_DISPATCH_WEBHOOK(std::string(user) + " was banned");
        }
        else {
            peer->ConsoleMessage("Player not found");
        }
    });

    peer.Register(Hashes::Rpc::C2S_RequestUnban, [this](Peer* peer, std::string_view user) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        // devcommands requires an exact format...
        Unban(user);

        peer->ConsoleMessage("Unbanning user " +  std::string(user));
    });

    peer.Register(Hashes::Rpc::C2S_RequestSave, [](Peer* peer) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        //WorldManager()->WriteFileWorldDB(true);

        WorldManager()->GetWorld()->WriteFiles();

        peer->ConsoleMessage("Saved the world");
    });

    peer.Register(Hashes::Rpc::C2S_RequestBanList, [this](Peer* peer) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        if (Valhalla()->m_blacklist.empty())
            peer->ConsoleMessage("Banned users: (none)");
        else {
            peer->ConsoleMessage("Banned users:");
            for (auto&& banned : Valhalla()->m_blacklist) {
                peer->ConsoleMessage(banned);
            }
        }

        if (!VH_SETTINGS.playerWhitelist)
            peer->ConsoleMessage("Whitelist is disabled");
        else {
            if (Valhalla()->m_whitelist.empty())
                peer->ConsoleMessage("Whitelisted users: (none)");
            else {
                peer->ConsoleMessage("Whitelisted users:");
                for (auto&& banned : Valhalla()->m_whitelist) {
                    peer->ConsoleMessage(banned);
                }
            }
        }
    });

    SendPeerInfo(peer);

    ZDOManager()->OnNewPeer(peer);
    RouteManager()->OnNewPeer(peer);
    ZoneManager()->OnNewPeer(peer);

    m_onlinePeers.push_back(&peer);
}

Peer* INetManager::GetPeer(std::string_view any) {
    Peer* peer = GetPeerByHost(any);
    if (!peer) peer = GetPeerByName(any);
    if (!peer) peer = GetPeerByUUID(std::atoll(any.data()));
    return peer;
}

// Return the peer or nullptr
Peer* INetManager::GetPeerByName(std::string_view name) {
    for (auto&& peer : m_onlinePeers) {
        if (peer->m_name == name)
            return peer;
    }
    return nullptr;
}

// Return the peer or nullptr
Peer* INetManager::GetPeerByUUID(OWNER_t uuid) {
    for (auto&& peer : m_onlinePeers) {
        if (peer->m_uuid == uuid)
            return peer;
    }
    return nullptr;
}

Peer* INetManager::GetPeerByHost(std::string_view host) {
    for (auto&& peer : m_onlinePeers) {
        if (peer->m_socket->GetHostName() == host)
            return peer;
    }
    return nullptr;
}

void INetManager::PostInit() {
    LOG_INFO(LOGGER, "Initializing NetManager");

    m_acceptor = std::make_unique<NetAcceptor>();
    m_acceptor->Listen();
}

void INetManager::Update() {
    // Accept new connections
    while (auto socket = m_acceptor->Accept()) {
        m_connectedPeers.push_back(
            std::make_unique<Peer>(std::move(socket))
        );
    }



    // Send periodic data (2s)
    PERIODIC_NOW(2s, {
        SendNetTime();
    });

    if (VH_SETTINGS.playerListSendInterval > 0s) {
        PERIODIC_NOW(VH_SETTINGS.playerListSendInterval, {
            SendPlayerList();
        });
    }

    // Send periodic pings (1s)
    PERIODIC_NOW(1s, {
        BYTES_t bytes;
        DataWriter writer(bytes);
        writer.Write<HASH_t>(0);
        writer.Write(true);

        for (auto&& peer : m_connectedPeers) {
            peer->Send(bytes);
        }
    });

    // Update peers
    for (auto&& peer : m_connectedPeers) {
        try {
            peer->Update();
        }
        catch (const std::runtime_error& e) {
            LOG_WARNING(LOGGER, "Peer error");
            LOG_WARNING(LOGGER, "{}", e.what());
            peer->m_socket->Close(false);
        }
    }


    /*
    // Pump steam callbacks
    if (VH_SETTINGS.serverDedicated)
        SteamGameServer_RunCallbacks();
    else
        SteamAPI_RunCallbacks();
        */
    // doesnt seem to work
    //AcceptorSteam::STEAM_NETWORKING_SOCKETS->RunCallbacks();



    // Cleanup
    {
        for (auto&& itr = m_onlinePeers.begin(); itr != m_onlinePeers.end(); ) {
            Peer& peer = *(*itr);

            if (!peer.m_socket->Connected()) {
                OnPeerQuit(peer);

                itr = m_onlinePeers.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }

    {
        for (auto&& itr = m_connectedPeers.begin(); itr != m_connectedPeers.end(); ) {
            Peer& peer = *(*itr);

            if (!peer.m_socket->Connected()) {
                OnPeerDisconnect(peer);

                itr = m_connectedPeers.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }
}

void INetManager::OnPeerQuit(Peer& peer) {
    LOG_INFO(LOGGER, "Cleaning up peer");
    VH_DISPATCH_WEBHOOK(peer.m_name + " has quit");

    ZDOManager()->OnPeerQuit(peer);

    if (peer.m_admin)
        Valhalla()->m_admin.insert(peer.m_socket->GetHostName());
    else
        Valhalla()->m_admin.erase(peer.m_socket->GetHostName());
}

void INetManager::OnPeerDisconnect(Peer& peer) {
    peer.SendDisconnect();

    LOG_INFO(LOGGER, "{} has disconnected", peer.m_socket->GetHostName());
}

void INetManager::Uninit() {
    SendDisconnect();

    for (auto&& peer : m_onlinePeers) {
        OnPeerQuit(*peer);
    }

    for (auto&& peer : m_connectedPeers) {
        OnPeerDisconnect(*peer);
    }

    m_acceptor.reset();
}

void INetManager::OnConfigLoad(bool reloading) {
    bool hasPassword = !VH_SETTINGS.serverPassword.empty();

    if (hasPassword) {
        VUtils::Random::GenerateAlphaNum(m_passwordSalt.data(), m_passwordSalt.size());

        const auto merge = VH_SETTINGS.serverPassword + std::string(m_passwordSalt.data(), m_passwordSalt.size());

        // Hash a salted password
        //m_password.resize(16);
        VUtils::md5(merge.c_str(), merge.size(), reinterpret_cast<uint8_t*>(m_passwordHash.data()));
        //MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
            //merge.size(), reinterpret_cast<uint8_t*>(m_password.data()));

        VUtils::String::FormatAscii(m_passwordHash.data(), m_passwordHash.size());
    }
}
