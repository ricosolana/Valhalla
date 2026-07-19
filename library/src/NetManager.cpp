//wtf am i using openssl for
//#include <openssl/md5.h>
//#include <openssl/rand.h>
#include <isteamgameserver.h>
#include <quill/LogMacros.h>
#include <string>
#include <string_view>
#include <vector>

#include "Avledet.h"
#include "Crypto.h"
#include "DataStream.h"
#include "DiscordManager.h"
#include "Hashes.h"
#include "ModManager.h"
#include "NetAcceptor.h"
#include "NetManager.h"
#include "NetSocket.h"
#include "Peer.h"
#include "ReplayManager.h"
#include "RouteManager.h"
#include "ServerSettings.h"
#include "Types.h"
#include "VUtils.h"
#include "VUtilsRandom.h"
#include "VUtilsResource.h"
#include "WorldManager.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

// TODO use netmanager instance instead

auto NET_MANAGER = std::make_unique<INetManager>();

INetManager *NetManager()
{
    return NET_MANAGER.get();
}

Peer::Ptr INetManager::Kick(std::string_view user)
{
    auto &&peer = FindPeer(user);
    if (peer) {
        peer->Kick();
    }

    return peer;
}

Peer::Ptr INetManager::Ban(std::string_view user)
{
    auto &&peer = FindPeer(user);

    if (peer) {
        Avledet()->m_blacklist.insert(peer->m_socket->get_host_name());
        peer->close(ConnectionStatus::ErrorBanned);
    } else
        Avledet()->m_blacklist.insert(user);

    return peer;
}

bool INetManager::Unban(std::string_view user)
{
    return Avledet()->m_blacklist.erase(user);
}

void INetManager::SendDisconnect()
{
    LOG_INFO(AVL_LOGGER, "Sending disconnect msg");

    for (auto &&peer : m_connectedPeers) {
        peer->SendDisconnect();
    }
}

void INetManager::SendPlayerList()
{
    if (!m_onlinePeers.empty()) {
        if (!AVL_SCRIPT_EVENT(IScriptManager::Events::PlayerList))
            return;

        DataWriter writer;

        writer.write(avledet::util::hashes::Rpc::S2C_UpdatePlayerList);// rpc hash

        {
            avledet::util::WriterScopedEncap scoped(writer);
            writer.write((std::uint32_t) m_onlinePeers.size());

            for (auto &&peer : m_onlinePeers) {
                writer.write(peer->m_name);
                writer.write(peer->m_characterID);
                writer.write("steam_" + peer->m_socket->get_host_name());
                auto &&platformItr = peer->m_syncData.find("platformDisplayName");
                auto &&platform    = platformItr != peer->m_syncData.end() ? platformItr->second : "";
                writer.write(platform);           // ...?
                auto forcedDisplayName = platform;//TODO the algo / usage is kinda weird / convoluted
                writer.write(forcedDisplayName);  //TODO
                writer.write(peer->IsMapVisible() || AVL_SETTINGS.playerListForceVisible);
                if (peer->IsMapVisible() || AVL_SETTINGS.playerListForceVisible) {
                    if (AVL_SETTINGS.playerListSmoothUpdating >= 2s)
                        writer.write(peer->m_pos);
                    else {// quickly dynamic map
                        auto &&zdo = peer->find_zdo();
                        if (zdo)
                            writer.write(zdo->get_position());
                        else
                            writer.write(peer->m_pos);
                    }
                }
            }
        }

        for (auto &&peer : m_onlinePeers) {
            peer->Send(writer.get_buf());
        }
    }
}

void INetManager::SendNetTime()
{
    for (auto &&peer : m_onlinePeers) {
        peer->Invoke(avledet::util::hashes::Rpc::S2C_UpdateTime, Avledet()->GetWorldTime());
    }
}

void INetManager::SendPeerInfo(Peer::Ptr peer)
{
    avledet::util::Writer writer;
    {
        //avledet::util::WriterScopedEncap scoped(writer);

        writer.write(Avledet()->ID());
        writer.write(std::string_view(VConstants::GAME));
        writer.write(VConstants::NETWORK);
        writer.write(Vector3f::ZERO);      // dummy
        writer.write(std::string_view(""));// dummy

        auto world = WorldManager()->GetWorld();

        writer.write(world->m_name);
        writer.write(world->m_seed);
        writer.write(world->m_seedName);// Peer does not seem to use
        writer.write(world->m_uid);
        writer.write(world->m_worldGenVersion);
        writer.write(Avledet()->GetWorldTime());
    }

    peer->Invoke(avledet::util::hashes::Rpc::PeerInfo, writer.release());
}

//void INetManager::OnNewClient(ISocket::Ptr socket, avledet::util::UserID uuid, const std::string &name, const Vector3f &pos) {
void INetManager::OnPeerConnect(Peer::Ptr peer)
{
    peer->SetAdmin(Avledet()->m_admin.contains(peer->m_socket->get_host_name()));

    if (!AVL_SCRIPT_EVENT(IScriptManager::Events::Join, peer)) {
        return peer->Disconnect();
    }

    AVL_DISPATCH_WEBHOOK(peer->m_name + " has joined");

    // Important
    peer->Register(avledet::util::hashes::Rpc::C2S_PlayerData, [](Peer::Ptr peer, avledet::util::Bytes pkg) {
        //DataReader reader(pkg);
        //auto reader = DataReader(std::vector<char>(pkg.begin(), pkg.end()));
        // TODO cannabilize package?
        DataReader reader(pkg);

        peer->m_pos = reader.read<Vector3f>();
        peer->SetMapVisible(reader.read<bool>());

        // TODO create template guide
        //reader.read<avledet::util::Map<std::string_view, std::string_view>>();

        auto count = reader.read<std::int32_t>();
        for (int i = 0; i < count; i++) {
            // Read player event data (only 2):
            //  'possibleEvents'
            //  'baseValue' // used to be a zdo member
            auto key              = reader.read<std::string_view>();// key
            peer->m_syncData[key] = reader.read<std::string>();     // value
        }
    });

    // isnt 'ban' a command?
    //  it should be part of RemoteCommand
    peer->Register(avledet::util::hashes::Rpc::C2S_RemoteCommand,
                   [](Peer::Ptr peer, std::string_view command) {
                       if (!peer->IsAdmin())
                           return peer->ConsoleMessage("You are not admin");

                       // TODO run commands or something?
                       //  this is still in beta and subject to change
                       //  although unlikely because this commands gets funneled to
                       //  valheim commands, which have existed for a while.
                       //  The only difference is that some commands are now classified as remote vs local.
                       (void) command;
                   });

    // Important
    peer->Register(avledet::util::hashes::Rpc::C2S_UpdateID, [](Peer::Ptr peer, ZDOID characterID) {
        // Peer sends 0,0 on after death

        //TODO the player only sends this:
        //  on server join
        //  and on every after-death thereafter
        //if (peer->m_characterID)
        //AVL_DISPATCH_WEBHOOK(peer->m_name + " has died");

        //peer->m_characterID.set_id(characterID.get_id());
        peer->m_characterID = characterID;

        LOG_NOTICE(AVL_LOGGER, "Got CharacterID from {} ({})", peer->m_name, characterID);
    });

    peer->Register(avledet::util::hashes::Rpc::C2S_RequestKick,
                   [this](Peer::Ptr peer, std::string_view user) {
                       // TODO maybe permissions tree in future?
                       //  lua? ...
                       if (!peer->IsAdmin())
                           return peer->ConsoleMessage("You are not admin");

                       if (Kick(user)) {
                           peer->ConsoleMessage("Kicked '" + std::string(user) + "'");
                           AVL_DISPATCH_WEBHOOK(std::string(user) + " was kicked");
                       } else {
                           peer->ConsoleMessage("Player not found");
                       }
                   });

    peer->Register(avledet::util::hashes::Rpc::C2S_RequestBan, [this](Peer::Ptr peer, std::string_view user) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        if (Ban(user)) {
            peer->ConsoleMessage("Banned '" + std::string(user) + "'");
            AVL_DISPATCH_WEBHOOK(std::string(user) + " was banned");
        } else {
            peer->ConsoleMessage("Player not found");
        }
    });

    peer->Register(avledet::util::hashes::Rpc::C2S_RequestUnban,
                   [this](Peer::Ptr peer, std::string_view user) {
                       if (!peer->IsAdmin())
                           return peer->ConsoleMessage("You are not admin");

                       // devcommands requires an exact format...
                       Unban(user);

                       peer->ConsoleMessage("Unbanning user " + std::string(user));
                   });

    peer->Register(avledet::util::hashes::Rpc::C2S_RequestSave, [](Peer::Ptr peer) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        //WorldManager()->WriteFileWorldDB(true);

        WorldManager()->GetWorld()->WriteFiles();

        peer->ConsoleMessage("Saved the world");
    });

    peer->Register(avledet::util::hashes::Rpc::C2S_RequestBanList, [](Peer::Ptr peer) {
        if (!peer->IsAdmin())
            return peer->ConsoleMessage("You are not admin");

        if (Avledet()->m_blacklist.empty())
            peer->ConsoleMessage("Banned users: (none)");
        else {
            peer->ConsoleMessage("Banned users:");
            for (auto &&banned : Avledet()->m_blacklist) {
                peer->ConsoleMessage(banned);
            }
        }

        if (!AVL_SETTINGS.playerWhitelist)
            peer->ConsoleMessage("Whitelist is disabled");
        else {
            if (Avledet()->m_whitelist.empty())
                peer->ConsoleMessage("Whitelisted users: (none)");
            else {
                peer->ConsoleMessage("Whitelisted users:");
                for (auto &&banned : Avledet()->m_whitelist) {
                    peer->ConsoleMessage(banned);
                }
            }
        }
    });

    SendPeerInfo(peer);

    ZDOManager()->OnNewPeer(peer);
    RouteManager()->OnNewPeer(peer);
    ZoneManager()->OnNewPeer(peer);

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    if (AVL_SETTINGS.TEST_discordAccountLinking) {
        auto &&host_name = peer->m_socket->get_host_name();
        peer->SetGated(!DiscordManager()->m_linked_accounts.contains(host_name));
        if (peer->IsGated()) {
            DiscordManager()->m_temp_linking_keys[host_name]
                    = {VUtils::Random::GenerateAlphaNum(4), Avledet()->Nanos()};
        }
    }
#endif

    // TODO remove this for debug only
    peer->SetGated(AVL_SETTINGS.TEST_playerRestrict);

    m_onlinePeers.push_back(peer);
}

Peer::Ptr INetManager::FindPeer(std::string_view any)
{
    Peer::Ptr peer = FindPeerByHost(any);
    if (!peer)
        peer = FindPeerByName(any);
    if (!peer)
        peer = FindPeerByUserID(std::atoll(any.data()));
    return peer;
}

// Return the peer or nullptr
Peer::Ptr INetManager::FindPeerByName(std::string_view name)
{
    for (auto &&peer : m_onlinePeers) {
        if (peer->m_name == name)
            return peer;
    }
    return nullptr;
}

// Return the peer or nullptr
Peer::Ptr INetManager::FindPeerByUserID(avledet::util::UserID uuid)
{
    for (auto &&peer : m_onlinePeers) {
        if (peer->GetUserID() == uuid)
            return peer;
    }
    return nullptr;
}

Peer::Ptr INetManager::FindPeerByHost(std::string_view host)
{
    for (auto &&peer : m_onlinePeers) {
        if (peer->m_socket->get_host_name() == host)
            return peer;
    }
    return nullptr;
}

void INetManager::PostInit()
{
    LOG_NOTICE(AVL_LOGGER, "Initializing NetManager");

    //m_acceptor = std::make_unique<AcceptorSteam>();
    //m_acceptor->Listen();
    if (AVL_SETTINGS.TEST_serverTcp) {
        m_acceptor
                = IAcceptor::tcp_dedicated(AVL_SETTINGS.serverBindAddress + ":" + std::to_string(AVL_SETTINGS.serverPort));// m_acceptor
    } else {
        m_acceptor = IAcceptor::steam_dedicated(AVL_SETTINGS.serverBindAddress + ":"
                                                + std::to_string(AVL_SETTINGS.serverPort));      // m_acceptor
    }

    m_acceptor->start();
    m_acceptor->on_connect([this](ISocket::Ptr socket) {
        try {
            auto peer = std::make_shared<Peer>(std::move(socket));
            if (AVL_SCRIPT_EVENT(IScriptManager::Events::Connect, peer)) {
                m_connectedPeers.insert(m_connectedPeers.end(), peer);

                if (AVL_SETTINGS.m_replay_mode == ReplayMode::CAPTURE) {
                    // TODO implement the replay mode
                    //assert(false);
                    avledet::replay::ReplayManager()->on_new_peer(peer);
                }
            }
        } catch (std::exception const &e) {
            // (un)expected (more like unlikely), but, ... ERRORS ARE POSSIBLE! just look at literally any asio method...
        }
    });
}

void INetManager::Update()
{
    ZoneScoped;

    // Send periodic data (2s)
    if (VUtils::run_periodic<struct periodic_peer_nettime>(2s)) {
        SendNetTime();
    }

    if (AVL_SETTINGS.playerListSmoothUpdating > 0s) {
        if (VUtils::run_periodic<struct periodic_peer_tablist>(AVL_SETTINGS.playerListSmoothUpdating)) {
            SendPlayerList();
        }
    }

    // Send periodic pings (1s)
    if (VUtils::run_periodic<struct periodic_peer_keepalive>(1s)) {
        DataWriter writer;
        writer.write((avledet::util::Hash) 0);
        writer.write(true);

        for (auto &&peer : m_connectedPeers) {
            peer->Send(writer.get_buf());
        }
    }

    // Update peers
    for (auto &&peer : m_connectedPeers) {
        try {
            if (VUtils::run_periodic<struct log_trace_player_status>(3s)) {
                LOG_TRACE_L1(AVL_LOGGER, "{}, {}, {}ms", peer->m_socket->get_host_name(),
                             peer->m_socket->get_address(), peer->m_socket->get_ping());
            }

            peer->update();
        } catch (std::runtime_error const &e) {
            LOG_WARNING(AVL_LOGGER, "Peer error");
            LOG_WARNING(AVL_LOGGER, "{}", e.what());
            peer->m_socket->close(false);
        }
    }

    // Pump steam callbacks
    m_acceptor->update();

    // Cleanup
    {
        for (auto &&itr = m_onlinePeers.begin(); itr != m_onlinePeers.end();) {
            Peer::Ptr &peer = *itr;

            if (peer->m_socket->get_status() == Status::Closed) {
                OnPeerQuit(peer);

                itr = m_onlinePeers.erase(itr);
            } else {
                ++itr;
            }
        }
    }

    {
        for (auto &&itr = m_connectedPeers.begin(); itr != m_connectedPeers.end();) {
            Peer::Ptr &peer = *itr;

            if (peer->m_socket->get_status() == Status::Closed) {
                OnPeerDisconnect(peer);

                itr = m_connectedPeers.erase(itr);
            } else {
                ++itr;
            }
        }
    }
}

void INetManager::OnPeerQuit(Peer::Ptr peer)
{
    LOG_INFO(AVL_LOGGER, "Cleaning up peer");
    AVL_DISPATCH_WEBHOOK(peer->m_name + " has quit");
    AVL_SCRIPT_EVENT(IScriptManager::Events::Quit, peer);

    ZDOManager()->OnPeerQuit(peer);

    if (AVL_SETTINGS.m_replay_mode == ReplayMode::CAPTURE) {
        // TODO
        //assert(false);
        avledet::replay::ReplayManager()->on_peer_quit(peer);
    }

    if (peer->IsAdmin()) {
        Avledet()->m_admin.insert(peer->m_socket->get_host_name());
    } else {
        Avledet()->m_admin.erase(peer->m_socket->get_host_name());
    }
}

void INetManager::OnPeerDisconnect(Peer::Ptr peer)
{
    AVL_SCRIPT_EVENT(IScriptManager::Events::Disconnect, peer);

    peer->SendDisconnect();

    LOG_INFO(AVL_LOGGER, "{} has disconnected", peer->m_socket->get_host_name());
}

void INetManager::Uninit()
{
    SendDisconnect();

    //TODO ... dont like both of these...
    for (auto &&peer : m_onlinePeers) {
        OnPeerQuit(peer);
    }

    for (auto &&peer : m_connectedPeers) {
        OnPeerDisconnect(peer);
    }

    //m_acceptor.reset();
    m_acceptor->stop();
}

void INetManager::OnConfigLoad(bool reloading)
{
    (void) reloading;

    bool hasPassword = !AVL_SETTINGS.m_server_password.empty();

    if (hasPassword) {
        m_passwordSalt = VUtils::Random::GenerateAlphaNum(16);

        // Hash a salted password
        //VUtils::md5(merge.c_str(), merge.size(), reinterpret_cast<std::uint8_t*>(m_passwordHash.data()));

        auto s         = avledet::crypto::md5(AVL_SETTINGS.m_server_password + m_passwordSalt);
        m_passwordHash = avledet::lexicon::CSU::ascii(std::string_view(s));
    } else {
        m_passwordSalt.clear();
        m_passwordHash.clear();
    }
}
