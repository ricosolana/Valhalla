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
    LOG(INFO) << "Sending disconnect msg";

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
                writer.Write(std::string_view(peer->m_name));
                writer.Write(std::string_view(peer->m_socket->GetHostName()));
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

        writer.Write(std::string_view(world->m_name));
        writer.Write(world->m_seed);
        writer.Write(std::string_view(world->m_seedName)); // Peer does not seem to use
        writer.Write(world->m_uid);
        writer.Write(world->m_worldGenVersion);
        writer.Write(Valhalla()->GetWorldTime());
    });
}



//void INetManager::OnNewClient(ISocket::Ptr socket, OWNER_t uuid, const std::string &name, const Vector3f &pos) {
void INetManager::OnPeerConnect(Peer& peer) {
    peer.m_admin = Valhalla()->m_admin.contains(peer.m_socket->GetHostName());

    if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::Join, peer)) {
        return peer.Disconnect();
    }

    VH_DISPATCH_WEBHOOK(peer.m_name + " has joined");

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdatePos, [this](Peer* peer, Vector3f pos, bool publicRefPos) {
        peer->m_pos = pos;
        peer->m_visibleOnMap = publicRefPos; // stupid name
        });

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdateID, [this](Peer* peer, ZDOID characterID) {
        // Peer sends 0,0 on death
        
        if (peer->m_characterID)
            VH_DISPATCH_WEBHOOK(peer->m_name + " has died");

        peer->m_characterID = characterID;

        LOG(INFO) << "Got CharacterID from " << peer->m_name << " ( " << characterID.GetOwner() << ":" << characterID.GetUID() << ")";
        });

    peer.Register(Hashes::Rpc::C2S_RequestKick, [this](Peer* peer, std::string_view user) {
        // TODO maybe permissions tree in future?
        //  lua? ...
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        if (Kick(user)) {
            peer->ConsoleMessage(std::make_tuple("Kicked '", user, "'"));
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
            peer->ConsoleMessage(std::make_tuple("Banned '", user, "'"));
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

        peer->ConsoleMessage(std::make_tuple("Unbanning user ", user));
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

    if (VH_SETTINGS.discordAccountLinking) {
        peer.m_gatedPlaythrough = !DiscordManager()->m_linkedAccounts.contains(peer.m_socket->GetHostName());
        if (peer.m_gatedPlaythrough) {
            DiscordManager()->m_tempLinkingKeys[peer.m_socket->GetHostName()] = VUtils::Random::GenerateAlphaNum(4);
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
    LOG(INFO) << "Initializing NetManager";

#ifdef VH_OPTION_ENABLE_CAPTURE
    // load session file if replaying
    if (VH_SETTINGS.packetMode == PacketMode::PLAYBACK) {
        auto path = fs::path(VH_CAPTURE_PATH)
            / WorldManager()->GetWorld()->m_name
            / std::to_string(VH_SETTINGS.packetCaptureSessionIndex)
            / "sessions.pkg";

        if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(path)) {
            DataReader reader(*opt);

            auto count = reader.Read<int32_t>();
            for (int i = 0; i < count; i++) {
                auto host = reader.Read<std::string>();
                auto nsStart = nanoseconds(reader.Read<int64_t>());
                auto nsEnd = nanoseconds(reader.Read<int64_t>());

                m_sortedSessions.push_back( { std::move(host), { nsStart, nsEnd } } );
            }
        }        
    }
#endif

    if (VH_SETTINGS.serverDedicated) m_acceptor = std::make_unique<AcceptorSteamDedicated>();
    else m_acceptor = std::make_unique<AcceptorSteamP2P>();

    m_acceptor->Listen();
}

void INetManager::Update() {
    ZoneScoped;

    // Accept new connections
    while (auto sock = m_acceptor->Accept()) {
        auto&& ptr = std::make_unique<Peer>(std::move(sock));
        if (VH_DISPATCH_MOD_EVENT(IModManager::Events::Connect, ptr.get())) {
            Peer* peer = (*m_connectedPeers.insert(m_connectedPeers.end(), std::move(ptr))).get();
            
#ifdef VH_OPTION_ENABLE_CAPTURE
            if (VH_SETTINGS.packetMode == PacketMode::CAPTURE) {
                // record peer joindata
                m_sortedSessions.push_back({ peer->m_socket->GetHostName(),
                    { Valhalla()->Nanos(), 0ns } });
                peer->m_disconnectCapture = &m_sortedSessions.back().second.second;

                const fs::path root = fs::path(VH_CAPTURE_PATH) 
                    / WorldManager()->GetWorld()->m_name 
                    / std::to_string(VH_SETTINGS.packetCaptureSessionIndex)
                    / peer->m_socket->GetHostName() 
                    / std::to_string(m_sessionIndexes[peer->m_socket->GetHostName()]++);

                std::string host = peer->m_socket->GetHostName();

                peer->m_recordThread = std::jthread([root, peer, host](std::stop_token token) {
                    tracy::SetThreadName(("Recorder" + host).c_str());

                    size_t chunkIndex = 0;

                    fs::create_directories(root);

                    auto&& saveBuffered = [&](int count) {
                        //ZoneScoped;

                        BYTES_t chunk;
                        DataWriter writer(chunk);

                        writer.Write((int32_t)count);
                        for (int i = 0; i < count; i++) {
                            nanoseconds ns;
                            BYTES_t packet;
                            {
                                std::scoped_lock<std::mutex> scoped(peer->m_recordmux);

                                auto&& front = peer->m_recordBuffer.front();
                                ns = front.first;
                                packet = std::move(front.second);
                                peer->m_captureQueueSize -= packet.size();

                                peer->m_recordBuffer.pop_front();
                            }
                            writer.Write(ns.count());
                            writer.Write(packet);
                        }

                        fs::path path = root / (std::to_string(chunkIndex++) + ".cap");

                        if (auto compressed = ZStdCompressor().Compress(chunk)) {
                            if (VUtils::Resource::WriteFile(path, *compressed))
                                LOG(WARNING) << "Saving " << path.c_str();
                            else
                                LOG(ERROR) << "Failed to save " << path.c_str();
                        }
                        else
                            LOG(ERROR) << "Failed to compress packet capture chunk";
                    };

                    // Primary occasional saving of captured packets
                    while (!token.stop_requested()) {
                        size_t size = 0;
                        size_t captureQueueSize = 0;
                        {
                            std::scoped_lock<std::mutex> scoped(peer->m_recordmux);
                            size = peer->m_recordBuffer.size();
                            captureQueueSize = peer->m_captureQueueSize;
                        }

                        // save at ~256Kb increments
                        if (captureQueueSize > VH_SETTINGS.packetFileUpperSize) {
                            saveBuffered(size);
                        }

                        std::this_thread::sleep_for(1ms);

                        FrameMark;
                    }

                    LOG(WARNING) << "Terminating async capture writer " << host;

                    // Save any buffered captures before exit
                    int size = 0;
                    {
                        std::scoped_lock<std::mutex> scoped(peer->m_recordmux);
                        size = peer->m_recordBuffer.size();
                    }

                    if (size)
                        saveBuffered(size);

                });

                LOG(WARNING) << "Starting capture for " << peer->m_socket->GetHostName();
            }
#endif
        }
    }

#ifdef VH_OPTION_ENABLE_CAPTURE
    // accept replay peers
    if (VH_SETTINGS.packetMode == PacketMode::PLAYBACK) {
        if (!m_sortedSessions.empty()) {
            auto&& front = m_sortedSessions.front();
            if (Valhalla()->Nanos() >= front.second.first) {
                auto&& peer = std::make_unique<Peer>(
                    std::make_shared<ReplaySocket>(front.first, m_sessionIndexes[front.first]++, front.second.second));

                m_connectedPeers.push_back(std::move(peer));

                m_sortedSessions.pop_front();
            }
            else {
                PERIODIC_NOW(30s, {
                    LOG(INFO) << "Replay peer joining in " << duration_cast<seconds>(front.second.first - Valhalla()->Nanos());
                });
            }
        }
    }
#endif


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
            peer->m_socket->Send(bytes);
        }
    });

    // Update peers
    for (auto&& peer : m_connectedPeers) {
        try {
            peer->Update();
        }
        catch (const std::runtime_error& e) {
            LOG(WARNING) << "Peer error";
            LOG(WARNING) << e.what();
            peer->m_socket->Close(false);
        }
    }

    // TODO I think this is in the correct location?
    if (VH_SETTINGS.serverDedicated)
        SteamGameServer_RunCallbacks();
    else 
        SteamAPI_RunCallbacks();

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

    LOG(INFO) << "Cleaning up peer";
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Quit, peer);
    ZDOManager()->OnPeerQuit(peer);

    if (peer.m_admin)
        Valhalla()->m_admin.insert(peer.m_socket->GetHostName());
    else
        Valhalla()->m_admin.erase(peer.m_socket->GetHostName());
}

void INetManager::OnPeerDisconnect(Peer& peer) {
    ModManager()->CallEvent(IModManager::Events::Disconnect, peer);

#ifdef VH_OPTION_ENABLE_CAPTURE
    if (VH_SETTINGS.packetMode == PacketMode::CAPTURE) {
        *(peer.m_disconnectCapture) = Valhalla()->Nanos();
    }
#endif

    peer.SendDisconnect();

    LOG(INFO) << peer.m_socket->GetHostName() << " disconnected";
}

void INetManager::Uninit() {
    SendDisconnect();

    for (auto&& peer : m_onlinePeers) {
        OnPeerQuit(*peer);
    }

    for (auto&& peer : m_connectedPeers) {
        OnPeerDisconnect(*peer);
    }

#ifdef VH_OPTION_ENABLE_CAPTURE
    if (VH_SETTINGS.packetMode == PacketMode::CAPTURE) {
        // save sessions
        BYTES_t bytes;
        DataWriter writer(bytes);
        writer.Write((int)m_sortedSessions.size());
        for (auto&& session : m_sortedSessions) {
            writer.Write(std::string_view(session.first));
            writer.Write(session.second.first.count());
            writer.Write(session.second.second.count());
        }

        auto path = fs::path(VH_CAPTURE_PATH)
            / WorldManager()->GetWorld()->m_name
            / std::to_string(VH_SETTINGS.packetCaptureSessionIndex)
            / "sessions.pkg";

        VUtils::Resource::WriteFile(path, bytes);
    }
#endif

    m_acceptor.reset();
}

void INetManager::OnConfigLoad(bool reloading) {
    bool hasPassword = !VH_SETTINGS.serverPassword.empty();

    if (hasPassword) {
        m_salt = VUtils::Random::GenerateAlphaNum(16);

        const auto merge = VH_SETTINGS.serverPassword + m_salt;

        // Hash a salted password
        m_password.resize(16);
        MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
            merge.size(), reinterpret_cast<uint8_t*>(m_password.data()));

        VUtils::String::FormatAscii(m_password);
    }

    if (m_acceptor)
        m_acceptor->OnConfigLoad(reloading);
}
