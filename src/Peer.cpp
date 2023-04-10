#include "ValhallaServer.h"
#include "NetManager.h"
#include "ZDOManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"

static const char* STATUS_STRINGS[] = { "None", "Connecting", "Connected",
    "ErrorVersion", "ErrorDisconnected", "ErrorConnectFailed", "ErrorPassword",
    "ErrorAlreadyConnected", "ErrorBanned", "ErrorFull" };

// Static globals initialized once
std::string Peer::PASSWORD;
std::string Peer::SALT;

Peer::Peer(ISocket::Ptr socket)
    : m_socket(std::move(socket)), m_lastPing(steady_clock::now())
{
    this->Register(Hashes::Rpc::Disconnect, [](Peer* self) {
        LOG(INFO) << "RPC_Disconnect";
        self->Disconnect();
    });

    this->Register(Hashes::Rpc::C2S_Handshake, [](Peer* rpc) {
        rpc->Register(Hashes::Rpc::PeerInfo, [](Peer* rpc, BYTES_t bytes) {
            // Forward call to rpc
            DataReader reader(bytes);

            rpc->m_uuid = reader.Read<OWNER_t>();
            if (!rpc->m_uuid)
                throw std::runtime_error("peer provided 0 owner");

            auto version = reader.Read<std::string>();
            LOG(INFO) << "Client " << rpc->m_socket->GetHostName() << " has version " << version;
            if (version != VConstants::GAME)
                return rpc->Close(ConnectionStatus::ErrorVersion);

            rpc->m_pos = reader.Read<Vector3f>();
            rpc->m_name = reader.Read<std::string>();
            //if (!(name.length() >= 3 && name.length() <= 15))
                //throw std::runtime_error("peer provided invalid length name");

            auto password = reader.Read<std::string>();
            auto ticket = reader.Read<BYTES_t>(); // read in the dummy ticket

            if (SERVER_SETTINGS.playerAuth) {
                auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
                if (steamSocket && SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID()) != k_EBeginAuthSessionResultOK)
                    return rpc->Close(ConnectionStatus::ErrorBanned);
            }

            if (Valhalla()->m_blacklist.contains(rpc->m_socket->GetHostName()))
                return rpc->Close(ConnectionStatus::ErrorBanned);

            // sanitize name (spaces, any ascii letters)
            //for (auto ch : name) {
            //    if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))) {
            //        LOG(INFO) << "Player has unsupported name character: " << (int)ch;
            //        return rpc->Close(ConnectionStatus::ErrorDisconnected);
            //    }
            //}


            if (SERVER_SETTINGS.worldMode != WorldMode::PLAYBACK && password != PASSWORD)
                return rpc->Close(ConnectionStatus::ErrorPassword);



            // if peer already connected
            //  peers with a new character can connect while replaying,
            //  but same characters with presumably same uuid will not work (same host/steam acc works because ReplaySocket prepends host with a 'REPLAY_'
            if (NetManager()->GetPeerByHost(rpc->m_socket->GetHostName()) || NetManager()->GetPeer(rpc->m_uuid) || NetManager()->GetPeerByName(rpc->m_name))
                return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);



            // if whitelist enabled
            if (SERVER_SETTINGS.playerWhitelist
                && !Valhalla()->m_whitelist.contains(rpc->m_socket->GetHostName())) {
                return rpc->Close(ConnectionStatus::ErrorFull);
            }

            // if too many players online
            if (NetManager()->GetPeers().size() >= SERVER_SETTINGS.playerMax)
                return rpc->Close(ConnectionStatus::ErrorFull);

            //NetManager()->OnNewClient(rpc->m_socket, uuid, name, pos);

            NetManager()->OnNewPeer(*rpc);

            return false;
        });

        bool hasPassword = !SERVER_SETTINGS.serverPassword.empty();

        if (hasPassword) {
            // Init password statically once
            if (PASSWORD.empty()) {
                SALT = VUtils::Random::GenerateAlphaNum(16);

                const auto merge = SERVER_SETTINGS.serverPassword + SALT;

                // Hash a salted password
                PASSWORD.resize(16);
                MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
                    merge.size(), reinterpret_cast<uint8_t*>(PASSWORD.data()));
                
                VUtils::String::FormatAscii(PASSWORD);
            }
        }

        rpc->Invoke(Hashes::Rpc::S2C_Handshake, hasPassword, SALT);

        return false;
    });

    if (SERVER_SETTINGS.worldMode == WorldMode::CAPTURE) {
        this->m_recordThread = std::jthread([this](std::stop_token token, std::string host) {
            size_t chunkIndex = 0;

            const fs::path root = fs::path(VALHALLA_WORLD_RECORDING_PATH) / host;
            fs::create_directories(root);

#define CHUNK_COUNT 1000

            while (!token.stop_requested()) {
                bool flag;
                {
                    std::scoped_lock<std::mutex> scoped(m_recordmux);
                    flag = m_recordBuffer.size() >= CHUNK_COUNT;
                }

                if (flag) {
                    BYTES_t chunk;
                    DataWriter writer(chunk);

                    writer.Write((int32_t)CHUNK_COUNT);
                    for (int i = 0; i < CHUNK_COUNT; i++) {
                        nanoseconds ns;
                        BYTES_t packet;
                        {
                            std::scoped_lock<std::mutex> scoped(m_recordmux);

                            auto&& front = m_recordBuffer.front();
                            ns = front.first;
                            packet = std::move(front.second);

                            m_recordBuffer.pop_front();
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
                }

                std::this_thread::sleep_for(1ms);
            }
        }, m_socket->GetHostName());

        LOG(WARNING) << "Starting capture for " << m_socket->GetHostName();
    }

    VLOG(1) << "Peer()";
}

void Peer::Update() {
    OPTICK_EVENT();

    auto now(steady_clock::now());

    // Send packet data
    m_socket->Update();

    // Read packets
    while (auto opt = m_socket->Recv()) {

        auto&& bytes = opt.value();
        DataReader reader(bytes);
        auto hash = reader.Read<HASH_t>();

        /*
        assert(!m_recordPacket);

        if (SERVER_SETTINGS.worldMode == WorldMode::CAPTURE) {
            // Allow replay packets establishing a replay handshake through
            //  since no explicit peer-capture is taking place, all imperative rpc packets are required
            //  to perfectly replay everything in the game
            if (hash == Hashes::Rpc::C2S_Handshake
                || hash == Hashes::Rpc::C2S_UpdateID
                || hash == Hashes::Rpc::C2S_UpdatePos
                || hash == Hashes::Rpc::PeerInfo
                || hash == Hashes::Rpc::Disconnect)
            {
                this->m_recordPacket = true;
            }
        }*/


        
        if (hash == 0) {
            if (reader.Read<bool>()) {
                // Reply to the server with a pong
                bytes.clear();
                DataWriter writer(bytes);
                writer.Write<HASH_t>(0);
                writer.Write<bool>(false);
                m_socket->Send(std::move(bytes));
            }
            else {
                m_lastPing = now;
            }
        }
        else {
            /*
            assert(!m_recordPacket);

            if (SERVER_SETTINGS.worldMode == WorldMode::CAPTURE) {
                // Allow replay packets establishing a replay handshake through
                //  since no explicit peer-capture is taking place, all imperative rpc packets are required
                //  to perfectly replay everything in the game
                if (hash == Hashes::Rpc::C2S_Handshake
                    || hash == Hashes::Rpc::C2S_UpdateID
                    || hash == Hashes::Rpc::C2S_UpdatePos
                    || hash == Hashes::Rpc::PeerInfo
                    || hash == Hashes::Rpc::Disconnect) 
                {
                    this->m_recordPacket = true;
                }
            }*/

            InternalInvoke(hash, reader);
            /*
            if (this->m_recordPacket) {
                assert(SERVER_SETTINGS.worldMode == WorldMode::CAPTURE);
                
                auto ns(Valhalla()->Nanos());

                this->m_recordPacket = false;
                std::scoped_lock<std::mutex> scoped(m_recordmux);
                this->m_recordBuffer.push_back({ ns, std::move(bytes) });
            }*/
        }



        if (this->m_recordPacket) {
            assert(SERVER_SETTINGS.worldMode == WorldMode::CAPTURE);

            auto ns(Valhalla()->Nanos());

            this->m_recordPacket = false;
            std::scoped_lock<std::mutex> scoped(m_recordmux);
            this->m_recordBuffer.push_back({ ns, std::move(bytes) });
        }
    }

    if (now - m_lastPing > SERVER_SETTINGS.socketTimeout) {
        LOG(INFO) << "Client RPC timeout";
        Disconnect();
    }
}

bool Peer::Close(ConnectionStatus status) {
    LOG(INFO) << "Peer error: " << STATUS_STRINGS[(int)status];
    Invoke(Hashes::Rpc::S2C_Error, status);
    Disconnect();
    return false;
}



void Peer::ChatMessage(const std::string& text, ChatMsgType type, const Vector3f& pos, const UserProfile& profile, const std::string& senderID) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::ChatMessage,
        pos, //Vector3f(10000, 10000, 10000),
        type,
        profile, //"<color=yellow><b>SERVER</b></color>",
        text,
        senderID //""
    );
}

void Peer::UIMessage(const std::string& text, UIMsgType type) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::S2C_UIMessage, type, text);
}

ZDO* Peer::GetZDO() {
    return ZDOManager()->GetZDO(m_characterID);
}

void Peer::Teleport(const Vector3f& pos, const Quaternion& rot, bool animation) {
    RouteManager()->Invoke(m_uuid, Hashes::Routed::S2C_RequestTeleport,
        pos,
        rot,
        animation
    );
}



void Peer::RouteParams(const ZDOID& targetZDO, HASH_t hash, BYTES_t params) {
    Invoke(Hashes::Rpc::RoutedRPC, RouteManager()->Serialize(SERVER_ID, this->m_uuid, targetZDO, hash, std::move(params)));
}



void Peer::ZDOSectorInvalidated(ZDO& zdo) {
    if (zdo.IsOwner(this->m_uuid))
        return;

    if (!ZoneManager()->ZonesOverlap(zdo.GetZone(), m_pos)) {
        if (m_zdos.erase(zdo.ID())) {
            m_invalidSector.insert(zdo.ID());
        }
    }
}

bool Peer::IsOutdatedZDO(ZDO& zdo, decltype(m_zdos)::iterator& outItr) {
    auto&& find = m_zdos.find(zdo.ID());

    outItr = find;

    return find == m_zdos.end()
        || zdo.m_rev.m_ownerRev > find->second.m_ownerRev
        || zdo.m_rev.m_dataRev > find->second.m_dataRev;
}