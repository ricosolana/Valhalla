//wtf am i using openssl for
//#include <openssl/md5.h>
//#include <openssl/rand.h>
#include <isteamgameserver.h>
#include <string>
#include <string_view>
#include <vector>

#include "NetManager.h"
#include "Crypto.h"
#include "NetAcceptor.h"
#include "NetSocket.h"
#include "ValhallaServer.h"
#include "WorldManager.h"
#include "VUtilsRandom.h"
#include "Hashes.h"
#include "ZDOManager.h"
#include "RouteManager.h"
#include "ZoneManager.h"
#include "VUtilsResource.h"
#include "DiscordManager.h"

// TODO use netmanager instance instead

auto NET_MANAGER = std::make_unique<INetManager>();
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
    LOG_INFO(VH_LOGGER, "Sending disconnect msg");

    for (auto&& peer : m_connectedPeers) {
        peer->SendDisconnect();
    }
}



void INetManager::SendPlayerList() {
    if (!m_onlinePeers.empty()) {
        if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::PlayerList))
            return;

        DataWriter writer;

        writer.write(avledet::util::hashes::Rpc::S2C_UpdatePlayerList); // rpc hash

        //assert(false); //TODO
        writer.write([this](DataWriter& writer) {
            writer.write((std::uint32_t)m_onlinePeers.size());

            for (auto&& peer : m_onlinePeers) {
                writer.write(peer->m_name);
                writer.write(peer->m_characterID);
                writer.write("steam_" + peer->m_socket->GetHostName());
                auto&& platformItr = peer->m_syncData.find("platformDisplayName");
                auto&& platform = platformItr != peer->m_syncData.end() ? platformItr->second : "";
                writer.write(platform); // ...?
                auto forcedDisplayName = platform; //TODO the algo / usage is kinda weird / convoluted
                writer.write(forcedDisplayName); //TODO
                writer.write(peer->IsMapVisible() || VH_SETTINGS.playerListForceVisible);
                if (peer->IsMapVisible() || VH_SETTINGS.playerListForceVisible) {
                    if (VH_SETTINGS.playerListSendInterval >= 2s)
                        writer.write(peer->m_pos);
                    else { // quickly dynamic map
                        auto&& zdo = peer->GetZDO();
                        if (zdo)
                            writer.write(zdo->GetPosition());
                        else
                            writer.write(peer->m_pos);
                    }
                }
            }
        });

        for (auto&& peer : m_onlinePeers) {
            peer->Send(writer.get_buf());
        }
    }
}

void INetManager::SendNetTime() {
    for (auto&& peer : m_onlinePeers) {
        peer->Invoke(avledet::util::hashes::Rpc::S2C_UpdateTime, Valhalla()->GetWorldTime());
    }
}



void INetManager::SendPeerInfo(Peer& peer) {
    peer.SubInvoke(avledet::util::hashes::Rpc::PeerInfo, [](DataWriter& writer) {
        writer.write(Valhalla()->ID());
        writer.write(std::string_view(VConstants::GAME));
        writer.write(VConstants::NETWORK);
        writer.write(Vector3f::zero()); // dummy
        writer.write(std::string_view("")); // dummy

        auto world = WorldManager()->GetWorld();

        writer.write(world->m_name);
        writer.write(world->m_seed);
        writer.write(world->m_seedName); // Peer does not seem to use
        writer.write(world->m_uid);
        writer.write(world->m_worldGenVersion);
        writer.write(Valhalla()->GetWorldTime());
    });
}



//void INetManager::OnNewClient(ISocket::Ptr socket, avledet::util::UserID uuid, const std::string &name, const Vector3f &pos) {
void INetManager::OnPeerConnect(Peer& peer) {
    peer.SetAdmin(Valhalla()->m_admin.contains(peer.m_socket->GetHostName()));

    if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::Join, peer)) {
        return peer.Disconnect();
    }

    VH_DISPATCH_WEBHOOK(peer.m_name + " has joined");

    // Important
    peer.Register(avledet::util::hashes::Rpc::C2S_PlayerData, [this](Peer* peer, avledet::util::ByteView pkg) {
        //DataReader reader(pkg);
        auto reader = DataReader(std::vector<char>(pkg.begin(), pkg.end()));

        peer->m_pos = reader.read<Vector3f>();
        peer->SetMapVisible(reader.read<bool>());
        
        auto count = reader.read<std::int32_t>();
        for (int i = 0; i < count; i++) {
            // Read player event data (only 2):
            //  'possibleEvents'
            //  'baseValue' // used to be a zdo member
            auto key = reader.read<std::string_view>(); // key
            peer->m_syncData[key] = reader.read<std::string>(); // value
        }
    });

    // isnt 'ban' a command?
    //  it should be part of RemoteCommand
    peer.Register(avledet::util::hashes::Rpc::C2S_RemoteCommand, [](Peer* peer, std::string_view command) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        // TODO run commands or something?
        //  this is still in beta and subject to change
        //  although unlikely because this commands gets funneled to 
        //  valheim commands, which have existed for a while.
        //  The only difference is that some commands are now classified as remote vs local.
    });

    // Important
    peer.Register(avledet::util::hashes::Rpc::C2S_UpdateID, [this](Peer* peer, ZDOID characterID) {
        // Peer sends 0,0 on after death
        
        //TODO the player only sends this:
        //  on server join
        //  and on every after-death thereafter        
        //if (peer->m_characterID)
            //VH_DISPATCH_WEBHOOK(peer->m_name + " has died");

        //peer->m_characterID.set_id(characterID.get_id());
        peer->m_characterID = characterID;

        LOG_INFO(VH_LOGGER, "Got CharacterID from {} ({})", peer->m_name, characterID);
        });

    peer.Register(avledet::util::hashes::Rpc::C2S_RequestKick, [this](Peer* peer, std::string_view user) {
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

    peer.Register(avledet::util::hashes::Rpc::C2S_RequestBan, [this](Peer* peer, std::string_view user) {
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

    peer.Register(avledet::util::hashes::Rpc::C2S_RequestUnban, [this](Peer* peer, std::string_view user) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        // devcommands requires an exact format...
        Unban(user);

        peer->ConsoleMessage("Unbanning user " +  std::string(user));
    });

    peer.Register(avledet::util::hashes::Rpc::C2S_RequestSave, [](Peer* peer) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        //WorldManager()->WriteFileWorldDB(true);

        WorldManager()->GetWorld()->WriteFiles();

        peer->ConsoleMessage("Saved the world");
        });

    peer.Register(avledet::util::hashes::Rpc::C2S_RequestBanList, [this](Peer* peer) {
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

#if VH_IS_ON(VH_DISCORD_INTEGRATION)
    if (VH_SETTINGS.discordAccountLinking) {
        peer.SetGated(!DiscordManager()->m_linkedAccounts.contains(peer.m_socket->GetHostName()));
        if (peer.IsGated()) {
            DiscordManager()->m_tempLinkingKeys[peer.m_socket->GetHostName()] = { VUtils::Random::GenerateAlphaNum(4), Valhalla()->Nanos() };
        }
    }
#endif

    // TODO remove this for debug only
    peer.SetGated(VH_SETTINGS.playerGated);

    m_onlinePeers.push_back(&peer);
}

Peer* INetManager::GetPeer(std::string_view any) {
    Peer* peer = GetPeerByHost(any);
    if (!peer) peer = GetPeerByName(any);
    if (!peer) peer = GetPeerByUserID(std::atoll(any.data()));
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
Peer* INetManager::GetPeerByUserID(avledet::util::UserID uuid) {
    for (auto&& peer : m_onlinePeers) {
        if (peer->GetUserID() == uuid)
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
    LOG_INFO(VH_LOGGER, "Initializing NetManager");

    //m_acceptor = std::make_unique<AcceptorSteam>();
    //m_acceptor->Listen();
    m_acceptor = IAcceptor::steam_dedicated("0.0.0.0:" + std::to_string(VH_SETTINGS.serverPort)); // m_acceptor

    m_acceptor->start();
    m_acceptor->on_connect([this](ISocket::Ptr socket) {
        auto&& ptr = std::make_unique<Peer>(std::move(socket));
        if (VH_DISPATCH_MOD_EVENT(IModManager::Events::Connect, ptr.get())) {
            m_connectedPeers.insert(m_connectedPeers.end(), std::move(ptr));
        }
    });
}

void INetManager::Update() {
    ZoneScoped;

    // Send periodic data (2s)
    if (VUtils::run_periodic<struct periodic_peer_nettime>(2s)) {
        SendNetTime();
    }

    if (VH_SETTINGS.playerListSendInterval > 0s) {
        if (VUtils::run_periodic<struct periodic_peer_tablist>(VH_SETTINGS.playerListSendInterval)) {
            SendPlayerList();
        }
    }

    // Send periodic pings (1s)
    if (VUtils::run_periodic<struct periodic_peer_keepalive>(1s)) {
        DataWriter writer;
        writer.write((avledet::util::Hash)0);
        writer.write(true);

        for (auto&& peer : m_connectedPeers) {
            peer->Send(writer.get_buf());
        }
    }

    // Update peers
    for (auto&& peer : m_connectedPeers) {
        try {
            peer->Update();
        }
        catch (const std::runtime_error& e) {
            LOG_WARNING(VH_LOGGER, "Peer error");
            LOG_WARNING(VH_LOGGER, "{}", e.what());
            peer->m_socket->Close(false);
        }
    }



    // Pump steam callbacks
    m_acceptor->update();
    //if (VH_SETTINGS.serverDedicated)
    //    SteamGameServer_RunCallbacks();
    //else
    //    SteamAPI_RunCallbacks();

    // doesnt seem to work
    //AcceptorSteam::STEAM_NETWORKING_SOCKETS->RunCallbacks();



    // Cleanup
    {
        for (auto&& itr = m_onlinePeers.begin(); itr != m_onlinePeers.end(); ) {
            Peer& peer = *(*itr);

            if (peer.m_socket->get_status() == Status::Closed) {
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

            if (peer.m_socket->get_status() == Status::Closed) {
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

    LOG_INFO(VH_LOGGER, "Cleaning up peer");
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

    LOG_INFO(VH_LOGGER, "{} has disconnected", peer.m_socket->GetHostName());
}

void INetManager::Uninit() {
    SendDisconnect();

    for (auto&& peer : m_onlinePeers) {
        OnPeerQuit(*peer);
    }

    for (auto&& peer : m_connectedPeers) {
        OnPeerDisconnect(*peer);
    }

    //m_acceptor.reset();
    m_acceptor->stop();
}

void INetManager::OnConfigLoad(bool reloading) {
    bool hasPassword = !VH_SETTINGS.serverPassword.empty();

    if (hasPassword) {
        m_passwordSalt = VUtils::Random::GenerateAlphaNum(16);

        // Hash a salted password
        //VUtils::md5(merge.c_str(), merge.size(), reinterpret_cast<std::uint8_t*>(m_passwordHash.data()));

        auto s = avledet::crypto::md5(VH_SETTINGS.serverPassword + m_passwordSalt);
        m_passwordHash = avledet::lexicon::CSU::ascii(std::string_view(s));
    } else {
        m_passwordSalt.clear();
        m_passwordHash.clear();
    }
}
