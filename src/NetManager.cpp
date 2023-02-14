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



bool INetManager::Kick(std::string user, const std::string &reason) {
    auto&& peer = GetPeer(user);
    try {
        if (!peer) peer = GetPeer(std::stoll(user));
    }
    catch (std::exception&) {}

    if (peer) {
        user = peer->m_socket->GetHostName();
        peer->Kick(reason);
        return true;
    }
    else {
        auto peers = GetPeers(user);

        for (auto&& peer : peers) {
            user = peer->m_socket->GetHostName();
            peer->Kick(reason);
        }

        if (!peers.empty())
            return true;
    }

    return false;
}

bool INetManager::Ban(std::string user, const std::string& reason) {
    {
        auto&& peer = GetPeer(user);
        try {
            if (!peer) peer = GetPeer(std::stoll(user));
        }
        catch (std::exception&) {}

        if (peer) {
            user = peer->m_socket->GetHostName();
            peer->Kick(reason);
            Valhalla()->m_blacklist.insert(user);
            return true;
        }
    }

    auto peers = GetPeers(user);

    for (auto&& peer : peers) {
        user = peer->m_socket->GetHostName();
        peer->Kick(reason);
    }

    if (!peers.empty())
        return true;
    
    Valhalla()->m_blacklist.insert(user);
    return false;
}

bool INetManager::Unban(const std::string& user) {
    return Valhalla()->m_blacklist.erase(user);
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
        static BYTES_t bytes; bytes.clear();
        DataWriter writer(bytes);
        writer.Write((int)m_peers.size());

        for (auto&& pair : m_peers) {
            auto&& peer = pair.second;
            writer.Write(peer->m_name);
            writer.Write(peer->m_socket->GetHostName());
            writer.Write(peer->m_characterID);
            writer.Write(peer->m_visibleOnMap);
            if (peer->m_visibleOnMap || SERVER_SETTINGS.playerForceVisible) {
                writer.Write(peer->m_pos);
            }
        }

        for (auto&& pair : m_peers) {
            auto&& peer = pair.second;
            // this is the problem
            peer->Invoke(Hashes::Rpc::PlayerList, bytes);
        }
    }
}

void INetManager::SendNetTime() {
    for (auto&& pair : m_peers) {
        auto&& peer = pair.second;
        peer->Invoke(Hashes::Rpc::NetTime, (double)Valhalla()->NetTime());
    }
}



void INetManager::SendPeerInfo(Peer* peer) {
    //auto now(steady_clock::now());
    //double netTime =
    //    (double)duration_cast<milliseconds>(now - m_startTime).count() / (double)((1000ms).count());
    static BYTES_t bytes; bytes.clear();
    DataWriter writer(bytes);

    writer.Write(Valhalla()->ID());
    writer.Write(VConstants::GAME);
    writer.Write(Vector3()); // dummy
    writer.Write(""); // dummy

    // why does server need to send a position and name?

    auto world = WorldManager()->GetWorld();

    writer.Write(world->m_name);
    writer.Write(world->m_seed);
    writer.Write(world->m_seedName);
    writer.Write(world->m_uid);
    writer.Write(world->m_worldGenVersion);
    writer.Write(Valhalla()->NetTime());

    peer->Invoke(Hashes::Rpc::PeerInfo, bytes);
}



void INetManager::RPC_PeerInfo(NetRpc* rpc, BYTES_t bytes) {
    DataReader reader(bytes);

    auto uuid = reader.Read<OWNER_t>();
    auto version = reader.Read<std::string>();
    LOG(INFO) << "Client " << rpc->m_socket->GetHostName() << " has version " << version;
    if (version != VConstants::GAME)
        return rpc->Close(ConnectionStatus::ErrorVersion);

    auto pos = reader.Read<Vector3>();
    auto name = reader.Read<std::string>();
    auto password = reader.Read<std::string>();
    auto ticket = reader.Read<BYTES_t>(); // read in the dummy ticket

    if (SERVER_SETTINGS.playerAuth) {
        auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
        if (steamSocket && SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID()) != k_EBeginAuthSessionResultOK)
            return rpc->Close(ConnectionStatus::ErrorBanned);
    }

    if (Valhalla()->m_blacklist.contains(rpc->m_socket->GetHostName()))
        return rpc->Close(ConnectionStatus::ErrorBanned);

    // allow ascii only (no spaces)
    for (auto ch : name) {
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))) {
            LOG(INFO) << "Player has illegal name character: " << (int)ch;
            return rpc->Close(ConnectionStatus::ErrorDisconnected);
        }
    }



    if (SERVER_SETTINGS.playerAutoPassword) {
        if (!rpc->m_skipPassword) {        
            if (password != m_saltedPassword)
                return rpc->Close(ConnectionStatus::ErrorPassword);

            Valhalla()->m_bypass.insert(rpc->m_socket->GetHostName());
        }
    } else if (password != m_saltedPassword)
        return rpc->Close(ConnectionStatus::ErrorPassword);



    // if peer already connected
    if (m_peers.contains(uuid))
        return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

    // if whitelist enabled
    if (SERVER_SETTINGS.playerWhitelist
        && !Valhalla()->m_whitelist.contains(rpc->m_socket->GetHostName())) {
        return rpc->Close(ConnectionStatus::ErrorFull);
    }

    // if too many players online
    if (m_peers.size() >= SERVER_SETTINGS.playerMax)
        return rpc->Close(ConnectionStatus::ErrorFull);

    Peer* peer;

    {
        auto ptr(std::make_unique<Peer>(rpc->m_socket, uuid, name, pos));

        peer = ptr.get();

        if (ModManager()->CallEvent(EVENT_HASH_Join, peer) == EventStatus::CANCEL) {
            return rpc->Close(ConnectionStatus::ErrorBanned);
        }

        m_peers.insert({ uuid, std::move(ptr) });
    }

    peer->m_admin = Valhalla()->m_admin.contains(peer->m_socket->GetHostName());
    
    //Peer* peer = m_peers.insert({ uuid, std::make_unique<Peer>(std::move(rpc->m_socket), uuid, name, pos) }).first->second.get();

    
    //Valhalla()->RunTaskLater()
    if (SERVER_SETTINGS.playerAutoPassword) {
        peer->m_magicLogin = rpc->m_skipPassword;
    }

    rpc = nullptr;

    // Important
    peer->Register(Hashes::Rpc::RefPos, [this](Peer* peer, Vector3 pos, bool publicRefPos) {
        peer->m_pos = pos;
        peer->m_visibleOnMap = publicRefPos; // stupid name
    });

    // Important
    peer->Register(Hashes::Rpc::CharacterID, [this](Peer* peer, NetID characterID) {
        if (!peer->m_characterID) {
            if (peer->m_magicLogin) {
                peer->ShowMessage("You were automagically logged in");
            }
            else {
                peer->ShowMessage("You will automagically log be logged in from now on");
            }
        }

        peer->m_characterID = characterID;

        LOG(INFO) << "Got CharacterID from " << peer->m_name << " ( " << characterID.m_uuid << ":" << characterID.m_id << ")";
    });

    // Extras
    //  Vanilla client has no means to doing this
    //peer->Register(Hashes::Rpc::RemotePrint, [](Peer* peer, std::string text) {
    //    // TODO limitation check
    //    LOG(INFO) << text << " (" << peer->m_name << " " << peer->m_socket->GetHostName() << ")";
    //});

    peer->Register(Hashes::Rpc::Kick, [this](Peer* peer, std::string user) {
        if (!peer->m_admin)
            return peer->Message("You are not admin");

        auto split = VUtils::String::Split(user, " ");

        if (Kick(std::string(split[0]), split.size() == 1 ? "" : std::string(split[1]))) {
            peer->Message("Kicked '" + user + "'");
        }
        else {
            peer->Message("Player not found");
        }
    });

    peer->Register(Hashes::Rpc::Ban, [this](Peer* peer, std::string user) {
        if (!peer->m_admin)
            return peer->Message("You are not admin");

        auto split = VUtils::String::Split(user, " ");

        if (Ban(std::string(split[0]), split.size() == 1 ? "" : std::string(split[1]))) {
            peer->Message("Banned '" + user + "'");
        }
        else {
            peer->Message("Player not found");
        }
    });

    peer->Register(Hashes::Rpc::Unban, [this](Peer* peer, std::string user) {
        if (!peer->m_admin)
            return peer->Message("You are not admin");

        if (Unban(user)) {
            peer->Message("Unbanned '" + user + "'");
        }
        else {
            peer->Message("Player is not banned");
        }
    });
    
    peer->Register(Hashes::Rpc::Save, [](Peer* peer) {
        if (!peer->m_admin)
            return peer->Message("You are not admin");

        WorldManager()->WriteFileWorldDB(true);

        peer->Message("Saved the world");
    });

    peer->Register(Hashes::Rpc::PrintBanned, [this](Peer* peer) {
        if (!peer->m_admin)
            return peer->Message("You are not admin");

        if (Valhalla()->m_blacklist.empty())
            peer->Message("Banned users: (none)");
        else {
            peer->Message("Banned users:");
            for (auto&& banned : Valhalla()->m_blacklist) {
                peer->Message(banned);
            }
        }

        if (!SERVER_SETTINGS.playerWhitelist)
            peer->Message("Whitelist is disabled");
        else {
            if (Valhalla()->m_whitelist.empty())
                peer->Message("Whitelisted users: (none)");
            else {
                peer->Message("Whitelisted users:");
                for (auto&& banned : Valhalla()->m_whitelist) {
                    peer->Message(banned);
                }
            }
        }
    });

    SendPeerInfo(peer);

    ZDOManager()->OnNewPeer(peer);
    RouteManager()->OnNewPeer(peer);
    ZoneManager()->OnNewPeer(peer);
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

std::vector<Peer*> INetManager::GetPeers(const std::string& addr) {
    auto addrSplit = VUtils::String::Split(addr, ".");

    std::vector<Peer*> peers;

    for (auto&& pair : m_peers) {
        auto&& peer = pair.second;

        // use wildcards too
        std::string p = peer->m_socket->GetAddress();
        auto peerSplit = VUtils::String::Split(p, ".");

        bool match = true;

        for (auto&& sub1 = peerSplit.begin(), sub2 = addrSplit.begin(); 
            sub1 != peerSplit.end() && sub2 != addrSplit.end();
            sub1++, sub2++) 
        {
            if (*sub2 == "*") continue;
            if (*sub2 != *sub1) {
                match = false;
                break;
            }
        }

        if (match)
            peers.push_back(peer.get());

        //if (peer->m_socket->GetAddress() == buf)
            //return peer.get();
    }
    return peers;
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

            rpc->Register(Hashes::Rpc::PeerInfo, [this](NetRpc* rpc, BYTES_t pkg) {
                RPC_PeerInfo(rpc, std::move(pkg));
            });

            if (SERVER_SETTINGS.playerAutoPassword && Valhalla()->m_bypass.contains(rpc->m_socket->GetHostName())) {
                rpc->m_skipPassword = true;
                rpc->Invoke(Hashes::Rpc::ClientHandshake, false, "");
            }
            else
                rpc->Invoke(Hashes::Rpc::ClientHandshake, m_hasPassword, m_salt);
        });

        m_rpcs.push_back(std::move(rpc));
    }







    // Send periodic data (2s)
    PERIODIC_NOW(2s, {
        SendNetTime();
        if (SERVER_SETTINGS.playerList)
            SendPlayerList();
    });

    // Send periodic pings (1s)
    PERIODIC_NOW(1s, {
        BYTES_t bytes;
        DataWriter writer(bytes);
        writer.Write<HASH_t>(0);
        writer.Write(true);

        for (auto&& rpc : m_rpcs) {
            LOG(DEBUG) << "Rpc join pinging ";
            rpc->m_socket->Send(bytes);
        }

        for (auto&& pair : m_peers) {
            auto&& peer = pair.second;
            LOG(DEBUG) << "Rpc pinging " << peer->m_uuid;
            peer->m_socket->Send(bytes);
        }
    });

    // Update peers
    for (auto&& pair : m_peers) {
        auto&& peer = pair.second;
        try {
            peer->Update();
        }
        catch (const VUtils::data_error& e) {
            LOG(ERROR) << "Peer provided bad data: " << e.what();
            peer->m_socket->Close(false);
        }
    }

    // Update joining (after peer update to avoid double updating any moved peer)
    for (auto&& rpc : m_rpcs) {
        try {
            rpc->PollOne();
        }
        catch (const VUtils::data_error& e) {
            LOG(ERROR) << "Peer provided bad data: " << e.what();
            rpc->m_socket->Close(false);
        }
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

                ModManager()->CallEvent(EVENT_HASH_Quit, peer.get());
                ZDOManager()->OnPeerQuit(peer.get());
                itr = m_peers.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }
}

void INetManager::Uninit() {
    m_acceptor.reset();
}

const robin_hood::unordered_map<OWNER_t, std::unique_ptr<Peer>>& INetManager::GetPeers() {
    return m_peers;
}
