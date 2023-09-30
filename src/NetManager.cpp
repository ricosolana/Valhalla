#include <openssl/md5.h>
#include <openssl/rand.h>
#include <isteamgameserver.h>

#include "NetManager.h"
#include "ValhallaServer.h"
#include "WorldManager.h"
#include "VUtilsRandom.h"
#include "Hashes.h"
#include "ZDOManager.h"
#include "RouteManager.h"
#include "ZoneManager.h"
#include "VUtilsResource.h"
#include "DiscordManager.h"

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
        if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::PlayerList))
            return;

        BYTES_t bytes;
        DataWriter writer(bytes);

        writer.Write(Hashes::Rpc::S2C_UpdatePlayerList);

        writer.SubWrite([this](DataWriter& writer) {
            writer.Write<int32_t>(m_onlinePeers.size());

            for (auto&& peer : m_onlinePeers) {
                writer.Write(peer->m_name);
                writer.Write(peer->m_socket->GetHostName());
                writer.Write(peer->m_characterID);
                writer.Write(peer->IsMapVisible() || VH_SETTINGS.playerListForceVisible);
                if (peer->IsMapVisible() || VH_SETTINGS.playerListForceVisible) {
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
    peer.SetAdmin(Valhalla()->m_admin.contains(peer.m_socket->GetHostName()));

    if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::Join, peer)) {
        return peer.Disconnect();
    }

    VH_DISPATCH_WEBHOOK(peer.m_name + " has joined");

    // Important
    peer.Register(Hashes::Rpc::C2S_PlayerData, [this](Peer* peer, BYTE_VIEW_t pkg) {
        DataReader reader(pkg);

        peer->m_pos = reader.Read<Vector3f>();
        peer->SetMapVisible(reader.Read<bool>());
        
        auto count = reader.Read<int32_t>();
        for (int i = 0; i < count; i++) {
            // Read player event data (only 2):
            //  'possibleEvents'
            //  'baseValue' // used to be a zdo member
            auto key = reader.Read<std::string_view>(); // key
            peer->m_syncData[key] = reader.Read<std::string>(); // value
        }
    });

    // isnt 'ban' a command?
    //  it should be part of RemoteCommand
    peer.Register(Hashes::Rpc::C2S_RemoteCommand, [](Peer* peer, std::string_view command) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        // TODO run commands or something?
        //  this is still in beta and subject to change
        //  although unlikely because this commands gets funneled to 
        //  valheim commands, which have existed for a while.
        //  The only difference is that some commands are now classified as remote vs local.
    });

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdateID, [this](Peer* peer, ZDOID characterID) {
        // Peer sends 0,0 on death
        
        if (peer->m_characterID)
            VH_DISPATCH_WEBHOOK(peer->m_name + " has died");

        peer->m_characterID.SetUID(characterID.GetUID());

        LOG_INFO(LOGGER, "Got CharacterID from {} ({})", peer->m_name, characterID);
        });

    peer.Register(Hashes::Rpc::C2S_RequestKick, [this](Peer* peer, std::string_view user) {
        // TODO maybe permissions tree in future?
        //  lua? ...
        if (!peer->IsAdmin())
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
        if (!peer->IsAdmin())
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
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        // devcommands requires an exact format...
        Unban(user);

        peer->ConsoleMessage("Unbanning user " +  std::string(user));
    });

    peer.Register(Hashes::Rpc::C2S_RequestSave, [](Peer* peer) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        //WorldManager()->WriteFileWorldDB(true);

        WorldManager()->GetWorld()->WriteFiles();

        peer->ConsoleMessage("Saved the world");
        });

    peer.Register(Hashes::Rpc::C2S_RequestBanList, [this](Peer* peer) {
        if (!peer->IsAdmin())
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

    if (VH_SETTINGS.discordAccountLinking) {
        peer.SetGated(!DiscordManager()->m_linkedAccounts.contains(peer.m_socket->GetHostName()));
        if (peer.IsGated()) {
            DiscordManager()->m_tempLinkingKeys[peer.m_socket->GetHostName()] = { VUtils::Random::GenerateAlphaNum(4), Valhalla()->Nanos() };
        }
    }

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
        if (peer->m_characterID.GetOwner() == uuid)
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

    m_acceptor = std::make_unique<AcceptorSteam>();
    m_acceptor->Listen();
}

void INetManager::Update() {
    ZoneScoped;

    // Accept new connections
    while (auto sock = m_acceptor->Accept()) {
        auto&& ptr = std::make_unique<Peer>(std::move(sock));
        if (VH_DISPATCH_MOD_EVENT(IModManager::Events::Connect, ptr.get())) {
            m_connectedPeers.insert(m_connectedPeers.end(), std::move(ptr));
        }
    }

    // Send periodic data (2s)
    if (VUtils::run_periodic<struct send_peer_time>(2s)) {
        SendNetTime();
    }

    if (VH_SETTINGS.playerListSendInterval > 0s) {
        if (VUtils::run_periodic<struct periodic_player_list>(VH_SETTINGS.playerListSendInterval)) {
            SendPlayerList();
        }
    }

    // Send periodic pings (1s)
    if (VUtils::run_periodic<struct periodic_peer_pings>(1s)) {
        BYTES_t bytes;
        DataWriter writer(bytes);
        writer.Write<HASH_t>(0);
        writer.Write(true);

        for (auto&& peer : m_connectedPeers) {
            peer->Send(bytes);
        }
    }

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



    // Pump steam callbacks
    if (VH_SETTINGS.serverDedicated)
        SteamGameServer_RunCallbacks();
    else
        SteamAPI_RunCallbacks();

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
    VH_DISPATCH_WEBHOOK(peer.m_name + " has quit");

    LOG_INFO(LOGGER, "Cleaning up peer");
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Quit, peer);
    ZDOManager()->OnPeerQuit(peer);

    if (peer.IsAdmin())
        Valhalla()->m_admin.insert(peer.m_socket->GetHostName());
    else
        Valhalla()->m_admin.erase(peer.m_socket->GetHostName());
}

void INetManager::OnPeerDisconnect(Peer& peer) {
#if VH_IS_ON(VH_USE_MODS)
    ModManager()->CallEvent(IModManager::Events::Disconnect, peer);
#endif

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
        VUtils::md5(merge.c_str(), merge.size(), reinterpret_cast<uint8_t*>(m_passwordHash.data()));

        VUtils::String::FormatAscii(m_passwordHash.data(), m_passwordHash.size());
    }
}
