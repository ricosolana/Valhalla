#include <optick.h>
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

using namespace std::chrono;

// TODO use netmanager instance instead

auto NET_MANAGER(std::make_unique<INetManager>());
INetManager* NetManager() {
    return NET_MANAGER.get();
}

// used during server start
void INetManager::InitPassword() {
    m_hasPassword = !Valhalla()->Settings().serverPassword.empty();

    if (m_hasPassword) {
        // better
        m_salt = VUtils::Random::GenerateAlphaNum(16);

        // Create random 16 byte salt
        //m_salt.resize(16);
        //RAND_bytes(reinterpret_cast<uint8_t*>(m_salt.data()), m_salt.size());
        //VUtils::String::FormatAscii(m_salt);

        const auto merge = Valhalla()->Settings().serverPassword + m_salt;

        // Hash a salted password
        m_saltedPassword.resize(16);
        MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
            merge.size(), reinterpret_cast<uint8_t*>(m_saltedPassword.data()));

        VUtils::String::FormatAscii(m_saltedPassword);
    }
}



void INetManager::Kick(const std::string& user) {
    auto&& peer = GetPeer(user);
    if (peer)
        peer->Kick();
}

void INetManager::Ban(const std::string& user) {
    auto&& peer = GetPeer(user);
    if (!peer)
        return;

    Valhalla()->m_blacklist.insert(user);
}

void INetManager::Unban(const std::string& user) {
    LOG(INFO) << "Unbanning " << user;

    Valhalla()->m_blacklist.erase(user);
}

void INetManager::SendDisconnect(Peer *peer) {
    LOG(INFO) << "Disconnect sent to " << peer->m_socket->GetHostName();
    peer->Invoke("Disconnect");
}

void INetManager::SendDisconnect() {
    LOG(INFO) << "Sending disconnect msg";

    for (auto&& pair : m_peers) {
        SendDisconnect(pair.second.get());
    }
}



void INetManager::SendPlayerList() {
    if (!m_peers.empty()) {
        NetPackage pkg;
        pkg.Write((int)m_peers.size());

        for (auto&& pair : m_peers) {
            auto&& peer = pair.second;
            pkg.Write(peer->m_name);
            pkg.Write(peer->m_socket->GetHostName());
            pkg.Write(peer->m_characterID);
            pkg.Write(peer->m_visibleOnMap);
            if (peer->m_visibleOnMap || SERVER_SETTINGS.playerForceVisible) {
                pkg.Write(peer->m_pos);
            }
        }

        for (auto&& pair : m_peers) {
            auto&& peer = pair.second;
            // this is the problem
            peer->Invoke(Hashes::Rpc::PlayerList, pkg);
        }
    }
}

void INetManager::SendNetTime() {
    for (auto&& pair : m_peers) {
        auto&& peer = pair.second;
        peer->Invoke(Hashes::Rpc::NetTime, (double)Valhalla()->Time());
    }
}



void INetManager::SendPeerInfo(Peer* peer) {
    //auto now(steady_clock::now());
    //double netTime =
    //    (double)duration_cast<milliseconds>(now - m_startTime).count() / (double)((1000ms).count());
    NetPackage pkg;
    pkg.Write(Valhalla()->ID());
    pkg.Write(VConstants::GAME);
    pkg.Write(Vector3()); // dummy
    pkg.Write(""); // dummy

    // why does server need to send a position and name?

    auto world = WorldManager()->GetWorld();

    pkg.Write(world->m_name);
    pkg.Write(world->m_seed);
    pkg.Write(world->m_seedName);
    pkg.Write(world->m_uid);
    pkg.Write(world->m_worldGenVersion);
    pkg.Write(Valhalla()->NetTime());

    peer->Invoke(Hashes::Rpc::PeerInfo, pkg);
}



void INetManager::RPC_PeerInfo(NetRpc* rpc, NetPackage pkg) {
    auto&& hostName = rpc->m_socket->GetHostName();

    auto uuid = pkg.Read<OWNER_t>();
    auto version = pkg.Read<std::string>();
    LOG(INFO) << "Client " << hostName << " has version " << version;
    if (version != VConstants::GAME)
        return rpc->Close(ConnectionStatus::ErrorVersion);

    auto pos = pkg.Read<Vector3>();
    auto name = pkg.Read<std::string>();
    auto password = pkg.Read<std::string>();
    auto ticket = pkg.Read<BYTES_t>(); // read in the dummy ticket

    if (SERVER_SETTINGS.playerAuth) {
        auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
        if (steamSocket && SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID()) != k_EBeginAuthSessionResultOK)
            return rpc->Close(ConnectionStatus::ErrorBanned);
    }

    if (Valhalla()->m_blacklist.contains(rpc->m_socket->GetHostName()))
        return rpc->Close(ConnectionStatus::ErrorBanned);

    if (password != m_saltedPassword)
        return rpc->Close(ConnectionStatus::ErrorPassword);

    // if peer already connected
    if (GetPeer(uuid))
        return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

    // if whitelist enabled
    if (SERVER_SETTINGS.playerWhitelist
        && !Valhalla()->m_whitelist.contains(rpc->m_socket->GetHostName())) {
        return rpc->Close(ConnectionStatus::ErrorFull);
    }

    // if too many players online
    if (m_peers.size() >= SERVER_SETTINGS.playerMax)
        return rpc->Close(ConnectionStatus::ErrorFull);

    assert(!m_peers.contains(uuid));
    Peer* peer = m_peers.insert({ uuid, std::make_unique<Peer>(std::move(rpc->m_socket), uuid, name, pos) }).first->second.get();
    
    assert(rpc->m_socket == nullptr);
    rpc = nullptr;

    // Important
    peer->Register(Hashes::Rpc::RefPos, [this](Peer* peer, Vector3 pos, bool publicRefPos) {
        peer->m_pos = pos;
        peer->m_visibleOnMap = publicRefPos; // stupid name
    });

    // Important
    peer->Register(Hashes::Rpc::CharacterID, [this](Peer* peer, NetID characterID) {
        peer->m_characterID = characterID;

        LOG(INFO) << "Got CharacterID from " << peer->m_name << " : " << characterID.ToString();
    });

    // Extras
    peer->Register(Hashes::Rpc::RemotePrint, [](Peer* peer, std::string text) {
        // TODO limitation check
        LOG(INFO) << text << " (" << peer->m_name << " " << peer->)";
    });

    peer->Register(Hashes::Rpc::Kick, [this](Peer* peer, std::string user) {
        // TODO Permission check
        std::string msg = "Kicking user " + user;
        peer->RemotePrint(msg);
        Kick(user);
    });

    peer->Register(Hashes::Rpc::Ban, [this](Peer* peer, std::string user) {
        // TODO Permission check
        peer->RemotePrint("Banning user " + user);
        Ban(user);
    });

    peer->Register(Hashes::Rpc::Unban, [this](Peer* peer, std::string user) {
        peer->RemotePrint("Unbanning user " + user);
        Unban(user);
    });
    
    peer->Register(Hashes::Rpc::Save, [](Peer* peer) {

    });

    peer->Register(Hashes::Rpc::PrintBanned, [this](Peer* peer) {
        std::string s = "Banned users";

        peer->RemotePrint(s);
    });

    SendPeerInfo(peer);

    ZDOManager()->OnNewPeer(peer);
    RouteManager()->OnNewPeer(peer);
    ZoneManager()->OnNewPeer(peer);

    //m_peers.insert({ peer->m_uuid, std::move(peer) });
}



// Return the peer or nullptr
Peer * INetManager::GetPeer(const std::string& name) {
    for (auto&& pair : m_peers) {
        auto&& peer = pair.second;
        if (peer->m_name == name)
            return peer.get();
    }
    return nullptr;
}

// Return the peer or nullptr
Peer * INetManager::GetPeer(OWNER_t uuid) {
    auto&& find = m_peers.find(uuid);
    if (find != m_peers.end())
        return find->second.get();
    return nullptr;
}

void INetManager::Init() {
    LOG(INFO) << "Initializing NetManager";

    m_acceptor = std::make_unique<AcceptorSteam>();
    m_acceptor->Listen();

    InitPassword();
}

void INetManager::Update() {
    OPTICK_CATEGORY("NetManagerUpdate", Optick::Category::Network);    

    // Accept new connections
    while (auto opt = m_acceptor->Accept()) {
        auto&& rpc = std::make_unique<NetRpc>(opt.value());
                        
        rpc->Register(Hashes::Rpc::Disconnect, [](NetRpc* rpc) {
            LOG(INFO) << "RPC_Disconnect";
            rpc->m_socket->Close(true);
        });

        rpc->Register(Hashes::Rpc::ServerHandshake, [this](NetRpc* rpc) {
            LOG(INFO) << "Client initiated handshake " << rpc->m_socket->GetHostName() << " " << rpc->m_socket->GetAddress();

            rpc->Register(Hashes::Rpc::PeerInfo, [this](NetRpc* rpc, NetPackage pkg) {
                RPC_PeerInfo(rpc, std::move(pkg));
            });

            rpc->Invoke(Hashes::Rpc::ClientHandshake, m_hasPassword, m_salt);
        });

        m_rpcs.push_back(std::move(rpc));
    }



    SteamGameServer_RunCallbacks();

    // Cleanup
    {
        auto&& itr = m_rpcs.begin();
        while (itr != m_rpcs.end()) {
            auto&& rpc = itr->get();
            assert(rpc);
            if (!(rpc->m_socket && rpc->m_socket->Connected())) {
                itr = m_rpcs.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }

    {
        auto&& itr = m_peers.begin();
        while (itr != m_peers.end()) {
            auto&& peer = itr->second;
            assert(peer);

            if (!peer->m_socket->Connected()) {
                LOG(INFO) << "Cleaning up peer";

                // no longer needed
                //RouteManager()->

                ZDOManager()->OnPeerQuit(peer.get());
                itr = m_peers.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }



    // Send periodic data (2s)
    PERIODIC_NOW(2s, {
        SendNetTime();
        SendPlayerList();
    });

    // Send periodic pings (1s)
    PERIODIC_NOW(1s, {
        for (auto&& rpc : m_rpcs) {
            LOG(DEBUG) << "Rpc join pinging ";
            //auto pkg(PKG());
            NetPackage pkg;
            pkg.Write<HASH_t>(0);
            pkg.Write(true);
            rpc->m_socket->Send(pkg);
        }

        for (auto&& pair : m_peers) {
            auto&& peer = pair.second;
            LOG(DEBUG) << "Rpc pinging " << peer->m_uuid;
            NetPackage pkg;
            pkg.Write<HASH_t>(0);
            pkg.Write(true);
            peer->m_socket->Send(pkg);
        }
    });

    // Update peers
    for (auto&& pair : m_peers) {
        auto&& peer = pair.second;
        try {
            peer->Update();
        }
        catch (const std::range_error& e) {
            LOG(ERROR) << "Peer provided malformed data: " << e.what();
            peer->m_socket->Close(false);
        }
    }

    // Update joining (after peer update to avoid double updating any moved peer)
    for (auto&& rpc : m_rpcs) {
        try {
            rpc->PollOne();
        }
        catch (const std::range_error&) {
            LOG(ERROR) << "Connecting peer provided malformed payload";
            rpc->m_socket->Close(false);
        }
    }
}

void INetManager::Close() {
    m_acceptor.reset();
}

const robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>>& INetManager::GetPeers() {
    return m_peers;
}
