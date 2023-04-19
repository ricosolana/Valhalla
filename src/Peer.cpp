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
        rpc->Register(Hashes::Rpc::PeerInfo, [](Peer* rpc, DataReader reader)
        //rpc->Register(Hashes::Rpc::PeerInfo, [](Peer* rpc, BYTES_t bytes)
            //int32_t,
            //OWNER_t uuid, std::string version, uint32_t nversion, Vector3f pos, std::string name, std::string password, BYTES_t ticket) 
            {
            // Forward call to rpc
            //DataReader reader(bytes);

                //std::string()
                //std::string_view()

            rpc->m_uuid = reader.Read<OWNER_t>();
            if (!rpc->m_uuid)
                throw std::runtime_error("peer provided 0 owner");

            auto version = reader.Read<std::string_view>();
            LOG(INFO) << "Client " << rpc->m_socket->GetHostName() << " has version " << version;
            if (version != VConstants::GAME)
                return rpc->Close(ConnectionStatus::ErrorVersion);

            auto nversion = reader.Read<uint32_t>();

            rpc->m_pos = reader.Read<Vector3f>();
            rpc->m_name = reader.Read<std::string>();
            //if (!(name.length() >= 3 && name.length() <= 15))
                //throw std::runtime_error("peer provided invalid length name");
            
            auto password = reader.Read<std::string_view>();

            if (VH_SETTINGS.playerAuth) {
                auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
                auto ticket = reader.Read<BYTE_VIEW_t>();
                if (steamSocket 
                    && (VH_SETTINGS.serverDedicated
                        ? SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID())
                        : SteamUser()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID())) != k_EBeginAuthSessionResultOK)
                    return rpc->Close(ConnectionStatus::ErrorDisconnected);
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


            if (VH_SETTINGS.worldCaptureMode != WorldMode::PLAYBACK && password != PASSWORD)
                return rpc->Close(ConnectionStatus::ErrorPassword);



            // if peer already connected
            //  peers with a new character can connect while replaying,
            //  but same characters with presumably same uuid will not work (same host/steam acc works because ReplaySocket prepends host with a 'REPLAY_'
            if (NetManager()->GetPeerByHost(rpc->m_socket->GetHostName()) || NetManager()->GetPeer(rpc->m_uuid) || NetManager()->GetPeerByName(rpc->m_name))
                return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);



            // if whitelist enabled
            if (VH_SETTINGS.playerWhitelist
                && !Valhalla()->m_whitelist.contains(rpc->m_socket->GetHostName())) {
                return rpc->Close(ConnectionStatus::ErrorFull);
            }

            // if too many players online
            if (NetManager()->GetPeers().size() >= VH_SETTINGS.playerMax)
                return rpc->Close(ConnectionStatus::ErrorFull);

            //NetManager()->OnNewClient(rpc->m_socket, uuid, name, pos);

            NetManager()->OnPeerConnect(*rpc);

            return false;
        });

        bool hasPassword = !VH_SETTINGS.serverPassword.empty();

        if (hasPassword) {
            // Init password statically once
            if (PASSWORD.empty()) {
                SALT = VUtils::Random::GenerateAlphaNum(16);

                const auto merge = VH_SETTINGS.serverPassword + SALT;

                // Hash a salted password
                PASSWORD.resize(16);
                MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
                    merge.size(), reinterpret_cast<uint8_t*>(PASSWORD.data()));
                
                VUtils::String::FormatAscii(PASSWORD);
            }
        }

        rpc->Invoke(Hashes::Rpc::S2C_Handshake, hasPassword, std::string_view(SALT));

        return false;
    });

    //VLOG(1) << "Peer()";

    LOG(WARNING) << m_socket->GetHostName() << " has connected";
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
        if (hash == 0) {
            if (reader.Read<bool>()) {
                // Reply to the server with a pong
                BYTES_t pong;
                DataWriter writer(pong);
                writer.Write<HASH_t>(0);
                writer.Write<bool>(false);
                m_socket->Send(std::move(pong));
            }
            else {
                m_lastPing = now;
            }
        }
        else {
            InternalInvoke(hash, reader);
        }

        if (VH_SETTINGS.worldCaptureMode == WorldMode::CAPTURE
            && !std::dynamic_pointer_cast<ReplaySocket>(m_socket))
        {
            auto ns(Valhalla()->Nanos());

            std::scoped_lock<std::mutex> scoped(m_recordmux);
            this->m_captureQueueSize += bytes.size();
            this->m_recordBuffer.push_back({ ns, std::move(bytes) });
        }
    }

    if (now - m_lastPing > VH_SETTINGS.playerTimeout) {
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
    this->Route(Hashes::Routed::ChatMessage,
        pos, //Vector3f(10000, 10000, 10000),
        type,
        profile, //"<color=yellow><b>SERVER</b></color>",
        std::string_view(text),
        std::string_view(senderID) //""
    );
}

void Peer::UIMessage(const std::string& text, UIMsgType type) {
    this->Route(Hashes::Routed::S2C_UIMessage, type, std::string_view(text));
}

ZDO* Peer::GetZDO() {
    return ZDOManager()->GetZDO(m_characterID);
}

void Peer::Teleport(const Vector3f& pos, const Quaternion& rot, bool animation) {
    this->Route(Hashes::Routed::S2C_RequestTeleport,
        pos,
        rot,
        animation
    );
}



void Peer::RouteParams(const ZDOID& targetZDO, HASH_t hash, BYTES_t params) {
    Invoke(Hashes::Rpc::RoutedRPC, RouteManager()->Serialize(VH_ID, this->m_uuid, targetZDO, hash, std::move(params)));
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
        || zdo.GetOwnerRevision() > find->second.first.m_ownerRev
        || zdo.m_dataRev > find->second.first.m_dataRev;
}