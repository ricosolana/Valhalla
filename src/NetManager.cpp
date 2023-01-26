#include <optick.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <isteamgameserver.h>

#include "NetManager.h"
#include "ValhallaServer.h"
#include "WorldManager.h"
#include "VUtilsRandom.h"
#include "Hashes.h"

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




void RemotePrint(NetRpc* rpc, const std::string& s) {
    rpc->Invoke(Hashes::Rpc::RemotePrint, s);
}

//void Disconnect(NetPeer *peer) {
//    //peer->m_rpc->m_socket->Close();
//    peer->Disconnect();
//}




void INetManager::Kick(NetPeer *peer) {
    if (!peer)
        return;

    peer->Kick();
}

void INetManager::Kick(const std::string& user) {
    Kick(GetPeer(user));
}

void INetManager::Ban(const std::string& user) {
    LOG(INFO) << "Banning " << user;

    Valhalla()->m_banned.insert(user);
}

void INetManager::Unban(const std::string& user) {
    LOG(INFO) << "Unbanning " << user;

    Valhalla()->m_banned.erase(user);
}

void INetManager::SendDisconnect(NetPeer *peer) {
    LOG(INFO) << "Disconnect sent to " << peer->m_rpc->m_socket->GetHostName();
    peer->m_rpc->Invoke("Disconnect");
}

void INetManager::SendDisconnect() {
    LOG(INFO) << "Sending disconnect msg";

    for (auto&& peer : m_peers) {
        SendDisconnect(peer.get());
    }
}



void INetManager::SendPlayerList() {
    if (!m_peers.empty()) {
        NetPackage pkg;
        pkg.Write((int)m_peers.size());

        for (auto&& peer : m_peers) {
            pkg.Write(peer->m_name);
            pkg.Write(peer->m_rpc->m_socket->GetHostName());
            pkg.Write(peer->m_characterID);
            pkg.Write(peer->m_visibleOnMap);
            if (peer->m_visibleOnMap) {
                pkg.Write(peer->m_pos);
            }
        }

        for (auto&& peer : m_peers) {
            // this is the problem
            peer->m_rpc->Invoke(Hashes::Rpc::PlayerList, pkg);
        }
    }
}

void INetManager::SendNetTime() {
    for (auto&& peer : m_peers) {
        peer->m_rpc->Invoke(Hashes::Rpc::NetTime, (double)Valhalla()->Time());
    }
}



void INetManager::SendPeerInfo(NetRpc* rpc) {
    //auto now(steady_clock::now());
    //double netTime =
    //    (double)duration_cast<milliseconds>(now - m_startTime).count() / (double)((1000ms).count());
    NetPackage pkg;
    pkg.Write(Valhalla()->ID());
    pkg.Write(VConstants::GAME);
    pkg.Write(Vector3()); // dummy
    pkg.Write(VConstants::PLAYERNAME); // dummy

    // why does server need to send a position and name?



    pkg.Write(m_world->m_name);
    pkg.Write(m_world->m_seed);
    pkg.Write(m_world->m_seedName);
    pkg.Write(m_world->m_uid);
    pkg.Write(m_world->m_worldGenVersion);
    pkg.Write(Valhalla()->NetTime());

    rpc->Invoke(Hashes::Rpc::PeerInfo, pkg);
}



void INetManager::RPC_PeerInfo(NetRpc* rpc, NetPackage pkg) {
    rpc->Unregister(Hashes::Rpc::PeerInfo);

    auto&& hostName = rpc->m_socket->GetHostName();

    auto uuid = pkg.Read<OWNER_t>();
    auto version = pkg.Read<std::string>();
    LOG(INFO) << "Client " << hostName << " has version " << version;
    if (version != VConstants::GAME)
        return rpc->SendError(ConnectionStatus::ErrorVersion);

    auto pos = pkg.Read<Vector3>();
    auto name = pkg.Read<std::string>();
    auto password = pkg.Read<std::string>();
    auto ticket = pkg.Read<BYTES_t>(); // read in the dummy ticket

    auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
    if (steamSocket && SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID()) != k_EBeginAuthSessionResultOK)
        return rpc->SendError(ConnectionStatus::ErrorBanned);

    if (password != m_saltedPassword)
        return rpc->SendError(ConnectionStatus::ErrorPassword);

    // if peer already connected
    if (GetPeer(uuid))
        return rpc->SendError(ConnectionStatus::ErrorAlreadyConnected);

    if (Valhalla()->m_banned.contains(rpc->m_socket->GetHostName()))
        return rpc->SendError(ConnectionStatus::ErrorBanned);

    // this is ugly
    // but probably how it should be done
    // Transfer the Rpc
    for (auto &&j: m_joining) {
        if (j.get() == rpc) {
            j.release(); // NOLINT(bugprone-unused-return-value)
            break;
        }
    }

    NetPeer* peer = new NetPeer(std::unique_ptr<NetRpc>(rpc), uuid, name);
    m_peers.push_back(std::unique_ptr<NetPeer>(peer));

    peer->m_pos = pos;


    // Important
    rpc->Register(Hashes::Rpc::RefPos, [this](NetRpc* rpc, Vector3 pos, bool publicRefPos) {
        auto&& peer = GetPeer(rpc);

        peer->m_pos = pos;
        peer->m_visibleOnMap = publicRefPos; // stupid name
    });

    // Important
    rpc->Register(Hashes::Rpc::CharacterID, [this](NetRpc* rpc, NetID characterID) {
        auto&& peer = GetPeer(rpc);
        peer->m_characterID = characterID;

        LOG(INFO) << "Got CharacterID from " << peer->m_name << " : " << characterID.ToString();
    });

    // Extras
    rpc->Register(Hashes::Rpc::RemotePrint, [](NetRpc* rpc, std::string text) {
        LOG(INFO) << text;
    });

    rpc->Register(Hashes::Rpc::Kick, [this](NetRpc* rpc, std::string user) {
        // TODO Permission check
        std::string msg = "Kicking user " + user;
        RemotePrint(rpc, msg);
        Kick(user);
    });

    rpc->Register(Hashes::Rpc::Ban, [this](NetRpc* rpc, std::string user) {
        // TODO Permission check
        RemotePrint(rpc, "Banning user " + user);
        Ban(user);
    });

    rpc->Register(Hashes::Rpc::Unban, [this](NetRpc* rpc, std::string user) {
        RemotePrint(rpc, "Unbanning user " + user);
        Unban(user);
    });
    
    rpc->Register(Hashes::Rpc::Save, [](NetRpc* rpc) {

    });

    rpc->Register(Hashes::Rpc::PrintBanned, [this](NetRpc* rpc) {
        std::string s = "Banned users";
        //std::vector<std:

        RemotePrint(rpc, s);
    });

    SendPeerInfo(rpc);
}



// Retrieve a peer by its member Rpc
// Will throw if peer by rpc is not found
NetPeer * INetManager::GetPeer(NetRpc* rpc) {
    for (auto&& peer : m_peers) {
        if (peer->m_rpc.get() == rpc)
            return peer.get();
    }
    throw std::runtime_error("Unable to find Rpc attached to peer");
}

// Return the peer or nullptr
NetPeer * INetManager::GetPeer(const std::string& name) {
    for (auto&& peer : m_peers) {
        if (peer->m_name == name)
            return peer.get();
    }
    return nullptr;
}

// Return the peer or nullptr
NetPeer * INetManager::GetPeer(OWNER_t uuid) {
    for (auto&& peer : m_peers) {
        if (peer->m_uuid == uuid)
            return peer.get();
    }
    return nullptr;
}

void INetManager::Init() {
    m_acceptor = std::make_unique<AcceptorSteam>();
    m_acceptor->Listen();

    InitPassword();

    m_world = new World{};
}

void INetManager::Update() {
    OPTICK_EVENT();
    // Accept new connections into joining
    while (auto opt = m_acceptor->Accept()) {
        auto&& rpc = std::make_unique<NetRpc>(opt.value());
                        
        rpc->Register(Hashes::Rpc::Disconnect, [](NetRpc* rpc) {
            LOG(INFO) << "RPC_Disconnect";
            rpc->m_socket->Close(true);
        });

        rpc->Register(Hashes::Rpc::ServerHandshake, [this](NetRpc* rpc) {
            rpc->Unregister(Hashes::Rpc::ServerHandshake);

            LOG(INFO) << "Client initiated handshake " << rpc->m_socket->GetHostName() << " " << rpc->m_socket->GetAddress();

            rpc->Register(Hashes::Rpc::PeerInfo, [this](NetRpc* rpc, NetPackage pkg) {
                RPC_PeerInfo(rpc, std::move(pkg));
            });

            rpc->Invoke(Hashes::Rpc::ClientHandshake, m_hasPassword, m_salt);
        });

        m_joining.push_back(std::move(rpc));
    }

    // Cleanup joining
    {
        auto&& itr = m_joining.begin();
        while (itr != m_joining.end()) {
            if (!(*itr && (*itr)->m_socket->Connected())) {
                itr = m_joining.erase(itr);
            } else {
                ++itr;
            }
        }
    }

    // Cleanup peers
    {
        auto&& itr = m_peers.begin();
        while (itr != m_peers.end()) {
            if (!(*itr)->m_rpc->m_socket->Connected()) {
                LOG(INFO) << "Cleaning up peer";
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
        for (auto&& rpc : m_joining) {
            LOG(DEBUG) << "Rpc join pinging ";
            //auto pkg(PKG());
            NetPackage pkg;
            pkg.Write<HASH_t>(0);
            pkg.Write(true);
            rpc->m_socket->Send(pkg);
        }

        for (auto&& peer : m_peers) {
            LOG(DEBUG) << "Rpc pinging " << peer->m_uuid;
            NetPackage pkg;
            pkg.Write<HASH_t>(0);
            pkg.Write(true);
            peer->m_rpc->m_socket->Send(pkg);
        }
    });

    // Update peers
    for (auto&& peer : m_peers) {
        try {
            peer->m_rpc->Update();
        }
        catch (const std::range_error& e) {
            LOG(ERROR) << "Peer provided malformed data: " << e.what();
            peer->m_rpc->m_socket->Close(false);
        }
    }

    // Update joining (after peer update to avoid double updating any moved peer)
    for (auto&& rpc : m_joining) {
        try {
            rpc->Update();
        }
        catch (const std::range_error&) {
            LOG(ERROR) << "Connecting peer provided malformed payload";
            rpc->m_socket->Close(false);
        }
    }

    //m_netTime += delta;
    SteamGameServer_RunCallbacks();
}

void INetManager::Close() {
    m_acceptor.reset();
}

const std::vector<std::unique_ptr<NetPeer>>& INetManager::GetPeers() {
    return m_peers;
}
