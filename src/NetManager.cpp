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



bool INetManager::Kick(std::string user) {
    auto&& peer = GetPeer(user);
    try {
        if (!peer) peer = GetPeer(std::stoll(user));
    }
    catch (std::exception&) {}

    if (peer) {
        user = peer->m_socket->GetHostName();
        peer->Kick();
        return true;
    }
    else {
        auto peers = GetPeers(user);

        for (auto&& peer : peers) {
            user = peer->m_socket->GetHostName();
            peer->Kick();
        }

        if (!peers.empty())
            return true;
    }

    return false;
}

bool INetManager::Ban(std::string user) {
    {
        auto&& peer = GetPeer(user);
        try {
            if (!peer) peer = GetPeer(std::stoll(user));
        }
        catch (const std::exception&) {}

        if (peer) {
            user = peer->m_socket->GetHostName();
            peer->Kick();
            Valhalla()->m_blacklist.insert(user);
            return true;
        }
    }

    auto peers = GetPeers(user);

    for (auto&& peer : peers) {
        user = peer->m_socket->GetHostName();
        peer->Kick();
    }

    if (!peers.empty())
        return true;
    
    Valhalla()->m_blacklist.insert(user);
    return false;
}

bool INetManager::Unban(const std::string& user) {
    return Valhalla()->m_blacklist.erase(user);
}



void INetManager::SendDisconnect() {
    LOG(INFO) << "Sending disconnect msg";

    for (auto&& peer : m_clients) {
        peer->SendDisconnect();
    }
}



void INetManager::SendPlayerList() {
    if (!m_peers.empty()) {
        if (!ModManager()->CallEvent(IModManager::Events::PlayerList))
            return;

        static BYTES_t bytes; bytes.clear();
        DataWriter writer(bytes);
        writer.Write((int)m_peers.size());

        for (auto&& peer : m_peers) {
            writer.Write(peer->m_name);
            writer.Write(peer->m_socket->GetHostName());
            writer.Write(peer->m_characterID);
            //writer.Write(peer->m_visibleOnMap || SERVER_SETTINGS.playerForceVisible);
            //if (peer->m_visibleOnMap || SERVER_SETTINGS.playerForceVisible) {
            //    writer.Write(peer->m_pos);
            //}
            writer.Write(peer->m_visibleOnMap);
            if (peer->m_visibleOnMap)
                writer.Write(peer->m_pos);
        }

        for (auto&& peer : m_peers) {
            // this is the problem
            peer->Invoke(Hashes::Rpc::S2C_UpdatePlayerList, bytes);
        }
    }
}

void INetManager::SendNetTime() {
    for (auto&& peer : m_peers) {
        peer->Invoke(Hashes::Rpc::S2C_UpdateTime, Valhalla()->GetWorldTime());
    }
}



void INetManager::SendPeerInfo(Peer& peer) {
    static BYTES_t bytes; bytes.clear();
    DataWriter writer(bytes);

    writer.Write(Valhalla()->ID());
    writer.Write(VConstants::GAME);
    writer.Write(Vector3f::Zero()); // dummy
    writer.Write(""); // dummy

    auto world = WorldManager()->GetWorld();

    writer.Write(world->m_name);
    writer.Write(world->m_seed);
    writer.Write(world->m_seedName); // Peer does not seem to use
    writer.Write(world->m_uid);
    writer.Write(world->m_worldGenVersion);
    writer.Write(Valhalla()->GetWorldTime());

    peer.Invoke(Hashes::Rpc::PeerInfo, bytes);
}



//void INetManager::OnNewClient(ISocket::Ptr socket, OWNER_t uuid, const std::string &name, const Vector3f &pos) {
void INetManager::OnNewPeer(Peer& peer) {
    //auto peer(std::make_unique<Peer>(std::move(socket)));

    //peer->m_uuid = uuid;
    //peer->m_name = name;
    //peer->m_pos = pos;

    peer.m_admin = Valhalla()->m_admin.contains(peer.m_socket->GetHostName());

    if (!ModManager()->CallEvent(IModManager::Events::Join, peer)) {
        return peer.Kick();
    }

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdatePos, [this](Peer* peer, Vector3f pos, bool publicRefPos) {
        peer->m_pos = pos;
        peer->m_visibleOnMap = publicRefPos; // stupid name
        });

    // Important
    peer.Register(Hashes::Rpc::C2S_UpdateID, [this](Peer* peer, ZDOID characterID) {
        peer->m_characterID = characterID;
        // Peer does send 0,0 on death
        //if (!characterID)
            //throw std::runtime_error("charac")

        LOG(INFO) << "Got CharacterID from " << peer->m_name << " ( " << characterID.m_uuid << ":" << characterID.m_id << ")";
        });

    peer.Register(Hashes::Rpc::C2S_RequestKick, [this](Peer* peer, std::string user) {
        // TODO maybe permissions tree in future?
        //  lua? ...
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        auto split = VUtils::String::Split(user, " ");

        if (Kick(std::string(split[0]))) {
            peer->ConsoleMessage("Kicked '" + user + "'");
        }
        else {
            peer->ConsoleMessage("Player not found");
        }
        });

    peer.Register(Hashes::Rpc::C2S_RequestBan, [this](Peer* peer, std::string user) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        auto split = VUtils::String::Split(user, " ");

        if (Ban(std::string(split[0]))) {
            peer->ConsoleMessage("Banned '" + user + "'");
        }
        else {
            peer->ConsoleMessage("Player not found");
        }
        });

    peer.Register(Hashes::Rpc::C2S_RequestUnban, [this](Peer* peer, std::string user) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        // devcommands requires an exact format...
        Unban(user);
        peer->ConsoleMessage("Unbanning user " + user);

        //if (Unban(user)) {
        //    peer->ConsoleMessage("Unbanning user " + user);
        //}
        //else {
        //    peer->ConsoleMessage("Player is not banned");
        //}
        });

    peer.Register(Hashes::Rpc::C2S_RequestSave, [](Peer* peer) {
        if (!peer->m_admin)
            return peer->ConsoleMessage("You are not admin");

        WorldManager()->WriteFileWorldDB(true);

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

        if (!SERVER_SETTINGS.playerWhitelist)
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

    m_peers.push_back(&peer);
}


// Return the peer or nullptr
Peer* INetManager::GetPeer(const std::string& name) {
    for (auto&& peer : m_peers) {
        if (peer->m_name == name)
            return peer;
    }
    return nullptr;
}

// Return the peer or nullptr
Peer* INetManager::GetPeer(OWNER_t uuid) {
    for (auto&& peer : m_peers) {
        if (peer->m_uuid == uuid)
            return peer;
    }
    return nullptr;
}

std::vector<Peer*> INetManager::GetPeers(const std::string& addr) {
    auto addrSplit = VUtils::String::Split(addr, ".");

    std::vector<Peer*> peers;

    for (auto&& peer : m_peers) {

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
            peers.push_back(peer);

        //if (peer->m_socket->GetAddress() == buf)
            //return peer.get();
    }
    return peers;
}

void INetManager::Init() {
    LOG(INFO) << "Initializing NetManager";

    m_acceptor = std::make_unique<AcceptorSteam>();
    m_acceptor->Listen();
}

void INetManager::Update() {
    OPTICK_CATEGORY("NetManagerUpdate", Optick::Category::Network);    

    // Accept new connections
    while (auto opt = m_acceptor->Accept()) {
        auto&& peer = std::make_unique<Peer>(std::move(*opt));
        if (ModManager()->CallEvent(IModManager::Events::Connect, peer.get()))
            m_clients.push_back(std::move(peer));
    }       

    // Send periodic data (2s)
    PERIODIC_NOW(2s, {
        SendNetTime();
        if (SERVER_SETTINGS.playerList)
            SendPlayerList();
        }
    ); 

    // Send periodic pings (1s)
    PERIODIC_NOW(1s, {
        BYTES_t bytes;
        DataWriter writer(bytes);
        writer.Write<HASH_t>(0);
        writer.Write(true);

        for (auto&& peer : m_clients) {
            peer->m_socket->Send(bytes);
        }
    });

    // Update peers
    for (auto&& peer : m_clients) {
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
    SteamGameServer_RunCallbacks();

    // Cleanup
    {
        for (auto&& itr = m_peers.begin(); itr != m_peers.end(); ) {
            Peer& peer = *(*itr);

            if (!peer.m_socket->Connected()) {
                CleanupPeer(peer);

                itr = m_peers.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }

    {
        for (auto&& itr = m_clients.begin(); itr != m_clients.end(); ) {
            Peer* peer = itr->get();
            assert(peer);
            if (!(peer->m_socket && peer->m_socket->Connected())) {
                ModManager()->CallEvent(IModManager::Events::Disconnect, peer);

                itr = m_clients.erase(itr);
            }
            else {
                ++itr;
            }
        }
    }
}

void INetManager::CleanupPeer(Peer& peer) {
    LOG(INFO) << "Cleaning up peer";
    ModManager()->CallEvent(IModManager::Events::Quit, peer);
    ZDOManager()->OnPeerQuit(peer);

    if (peer.m_admin)
        Valhalla()->m_admin.insert(peer.m_socket->GetHostName());
    else
        Valhalla()->m_admin.erase(peer.m_socket->GetHostName());
}

void INetManager::Uninit() {
    SendDisconnect();

    for (auto&& peer : m_peers) {
        CleanupPeer(*peer);
    }

    m_acceptor.reset();
}
