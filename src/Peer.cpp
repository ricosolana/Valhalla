#include "ValhallaServer.h"
#include "NetManager.h"
#include "ZDOManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"

static const char* STATUS_STRINGS[] = { "None", "Connecting", "Connected",
    "ErrorVersion", "ErrorDisconnected", "ErrorConnectFailed", "ErrorPassword",
    "ErrorAlreadyConnected", "ErrorBanned", "ErrorFull" };

// Static globals initialized once
//std::string Peer::PASSWORD;
//std::string Peer::SALT;

Peer::Peer(ISocket::Ptr socket)
    : m_socket(std::move(socket)), m_lastPing(steady_clock::now())
{
    this->Register(Hashes::Rpc::Disconnect, [](Peer* self) {
        //LOG(INFO) << "RPC_Disconnect";
        self->Disconnect();
    });

    this->Register(Hashes::Rpc::C2S_Handshake, [](Peer* rpc) {
        rpc->Register(Hashes::Rpc::PeerInfo, [](Peer* rpc, DataReader reader)
            {
            rpc->m_uuid = reader.Read<OWNER_t>();
            if (!rpc->m_uuid)
                throw std::runtime_error("peer provided 0 owner");

            auto version = reader.Read<std::string_view>();
            LOG_INFO(LOGGER, "Client {} has version {}", rpc->m_socket->GetHostName(), version);
            if (version != VConstants::GAME)
                return rpc->Close(ConnectionStatus::ErrorVersion);

            reader.Read<uint32_t>(); // network version

            rpc->m_pos = reader.Read<Vector3f>();
            rpc->m_name = reader.Read<std::string>();
            //if (!(name.length() >= 3 && name.length() <= 15))
                //throw std::runtime_error("peer provided invalid length name");
            
            auto password = reader.Read<std::string_view>();

            if (VH_SETTINGS.playerOnline) {
                auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket);
                auto ticket = reader.Read<BYTE_VIEW_t>();
                if (steamSocket 
                    && (VH_SETTINGS.serverDedicated
                        ? SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID())
                        : SteamUser()->BeginAuthSession(ticket.data(), ticket.size(), steamSocket->m_steamNetId.GetSteamID())) != k_EBeginAuthSessionResultOK)
                    return rpc->Close(ConnectionStatus::ErrorDisconnected);
            }

            if (
#ifdef VH_OPTION_ENABLE_CAPTURE
                VH_SETTINGS.packetMode != PacketMode::PLAYBACK && 
#endif
                password != NetManager()->m_password)
                return rpc->Close(ConnectionStatus::ErrorPassword);

            // if peer already connected
            //  peers with a new character can connect while replaying,
            //  but same characters with presumably same uuid will not work (same host/steam acc works because ReplaySocket prepends host with a 'REPLAY_'
            if (NetManager()->GetPeerByUUID(rpc->m_uuid) || NetManager()->GetPeerByName(rpc->m_name))
                return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

            NetManager()->OnPeerConnect(*rpc);

            return false;
        });

        if (Valhalla()->m_blacklist.contains(rpc->m_socket->GetHostName()))
            return rpc->Close(ConnectionStatus::ErrorBanned);

        if (NetManager()->GetPeerByHost(rpc->m_socket->GetHostName()))
            return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

        // if whitelist enabled
        if (VH_SETTINGS.playerWhitelist
            && !Valhalla()->m_whitelist.contains(rpc->m_socket->GetHostName())) {
            return rpc->Close(ConnectionStatus::ErrorFull);
        }

        // if too many players online
        if (NetManager()->GetPeers().size() >= VH_SETTINGS.playerMax)
            return rpc->Close(ConnectionStatus::ErrorFull);

        bool hasPassword = !VH_SETTINGS.serverPassword.empty();

        rpc->Invoke(Hashes::Rpc::S2C_Handshake, hasPassword, std::string_view(NetManager()->m_salt));

        return false;
    });

    LOG_INFO(LOGGER, "{} has connected", m_socket->GetHostName());
}

void Peer::Update() {
    ZoneScoped;

    auto now(steady_clock::now());

    // Send packet data
    m_socket->Update();

    // Read packets
    while (auto opt = this->Recv()) {

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
                this->Send(std::move(pong));
            }
            else {
                m_lastPing = now;
            }
        }
        else {
            InternalInvoke(hash, reader);
        }

#ifdef VH_OPTION_ENABLE_CAPTURE
        if (VH_SETTINGS.packetMode == PacketMode::CAPTURE
            && !std::dynamic_pointer_cast<ReplaySocket>(m_socket))
        {
            auto ns(Valhalla()->Nanos());

            std::scoped_lock<std::mutex> scoped(m_recordmux);
            this->m_captureQueueSize += bytes.size();
            this->m_recordBuffer.push_back({ ns, std::move(bytes) });
        }
#endif
    }

    if (VH_SETTINGS.playerTimeout > 0s && now - m_lastPing > VH_SETTINGS.playerTimeout) {
        LOG_INFO(LOGGER, "{} has timed out", this->m_socket->GetHostName());
        Disconnect();
    }
}

bool Peer::Close(ConnectionStatus status) {
    LOG_INFO(LOGGER, "Peer error: {}", STATUS_STRINGS[(int)status]);
    Invoke(Hashes::Rpc::S2C_Error, status);
    Disconnect();
    return false;
}



ZDO* Peer::GetZDO() {
    return ZDOManager()->GetZDO(m_characterID);
}

void Peer::Teleport(Vector3f pos, Quaternion rot, bool animation) {
    this->Route(Hashes::Routed::S2C_RequestTeleport,
        pos,
        rot,
        animation
    );
}



void Peer::RouteParams(ZDOID targetZDO, HASH_t hash, BYTES_t params) {
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