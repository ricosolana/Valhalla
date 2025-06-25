#include "NetManager.h"
#include "RouteManager.h"
#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "ZDOManager.h"
#include <magic_enum.hpp>
#include <quill/LogMacros.h>

// Static globals initialized once
//std::string Peer::PASSWORD;
//std::string Peer::SALT;

Peer::Peer(ISocket::Ptr socket) :
    m_lastPing(std::chrono::steady_clock::now()),
    m_socket(std::move(socket))
{
    this->Register(avledet::util::hashes::Rpc::Disconnect, [](Peer *self) {
        LOG_INFO(AVL_LOGGER, "RPC_Disconnect");
        self->Disconnect();
    });

    this->Register(avledet::util::hashes::Rpc::C2S_Handshake, [](Peer *rpc) {
        rpc->Register(avledet::util::hashes::Rpc::PeerInfo, [](Peer *rpc, DataReader reader) {
            rpc->m_characterID.set_user_id(reader.read<std::int64_t>());
#if AVL_IS_ON(AVL_DISALLOW_MALICIOUS_PLAYERS)
            if (!rpc->m_characterID)
                throw std::runtime_error("peer provided 0 owner");
#endif
            auto version = reader.read<std::string_view>();
            LOG_INFO(AVL_LOGGER, "Client {} has version {}", rpc->m_socket->get_host_name(), version);
            if (version != VConstants::GAME)
                return rpc->Close(ConnectionStatus::ErrorVersion);

            // network version
            if (reader.read<std::uint32_t>() != VConstants::NETWORK) {
                return rpc->Close(ConnectionStatus::ErrorVersion);
            }

            rpc->m_pos = reader.read<Vector3f>();
#if AVL_IS_ON(AVL_DISALLOW_NON_CONFORMING_PLAYERS)
            if (rpc->m_pos.Hsq_magnitude()
                > IZoneManager::WORLD_RADIUS_IN_METERS * IZoneManager::WORLD_RADIUS_IN_METERS)
                throw std::runtime_error("peer position is outside of map");
#endif
            rpc->m_name = reader.read<std::string>();
#if AVL_IS_ON(AVL_DISALLOW_NON_CONFORMING_PLAYERS)
            if (!(rpc->m_name.length() >= 3 && rpc->m_name.length() <= 15))
                throw std::runtime_error("peer provided invalid length name");
#endif
            auto password = reader.read<std::string_view>();

            if (AVL_SETTINGS.playerOnline) {
                auto ticket = reader.read<avledet::util::ByteView>();

                if (auto steamSocket = std::dynamic_pointer_cast<SteamSocket>(rpc->m_socket)) {

                    if (!steamSocket->authenticate(ticket)) {
                        LOG_INFO(AVL_LOGGER, "Client {} has invalid ticket", rpc->m_socket->get_host_name());
                        return rpc->Close(ConnectionStatus::ErrorDisconnected);
                    }
                }
            }

            if (password != std::string_view(NetManager()->m_passwordHash))
                return rpc->Close(ConnectionStatus::ErrorPassword);

            // if peer already connected
            //  peers with a new character can connect while replaying,
            //  but same characters with presumably same uuid will not work (same host/steam acc works because ReplaySocket prepends host with a 'REPLAY_'
            if (NetManager()->FindPeerByUserID(rpc->GetUserID()) || NetManager()->FindPeerByName(rpc->m_name))
                return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

            NetManager()->OnPeerConnect(*rpc);

            return false;
        });

        if (Avledet()->m_blacklist.contains(rpc->m_socket->get_host_name()))
            return rpc->Close(ConnectionStatus::ErrorBanned);

        if (NetManager()->FindPeerByHost(rpc->m_socket->get_host_name()))
            return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

        // if whitelist enabled
        if (AVL_SETTINGS.playerWhitelist
            && !Avledet()->m_whitelist.contains(rpc->m_socket->get_host_name())) {
            return rpc->Close(ConnectionStatus::ErrorFull);
        }

        // if too many players online
        if (NetManager()->GetPeers().size() >= AVL_SETTINGS.playerMax)
            return rpc->Close(ConnectionStatus::ErrorFull);

        bool hasPassword = !AVL_SETTINGS.serverPassword.empty();

        rpc->Invoke(avledet::util::hashes::Rpc::S2C_Handshake, hasPassword,
                    std::string_view(NetManager()->m_passwordSalt));

        return false;
    });

    LOG_INFO(AVL_LOGGER, "{} has connected", m_socket->get_host_name());
}

void Peer::update()
{
    ZoneScoped;

    auto now(std::chrono::steady_clock::now());

    // Send packet data
    //m_socket->Update();

    // Read packets
    while (auto opt = this->Recv()) {
        auto &&bytes = opt.value();
        DataReader reader(bytes);

        auto hash = reader.read<avledet::util::Hash>();
        if (hash == 0) [[unlikely]] {
            if (reader.read<bool>()) {
                // Reply to the server with a pong
                DataWriter writer;
                writer.write((avledet::util::Hash) 0);
                writer.write(false);
                this->Send(std::move(writer.get_buf()));
            } else {
                m_lastPing = now;
            }
        } else [[likely]] {
            InternalInvoke(hash, reader);
        }
    }

    if (AVL_SETTINGS.playerTimeout > 0s && now - m_lastPing > AVL_SETTINGS.playerTimeout) [[unlikely]] {
        LOG_INFO(AVL_LOGGER, "{} has timed out", this->m_socket->get_host_name());
        Disconnect();
    }
}

bool Peer::Close(ConnectionStatus status)
{
    LOG_INFO(AVL_LOGGER, "Peer error: {}", magic_enum::enum_name(status));
    Invoke(avledet::util::hashes::Rpc::S2C_Error, status);
    Disconnect();
    return false;
}

void Peer::SetAdmin(bool enable)
{
    if (enable)
        Avledet()->m_admin.erase(m_socket->get_host_name());
    else
        Avledet()->m_admin.insert(m_socket->get_host_name());
}

ZDO::optional Peer::GetZDO()
{
    return ZDOManager()->GetZDO(m_characterID);
}

void Peer::Teleport(Vector3f pos, Quaternion rot, bool animation)
{
    this->Route(avledet::util::hashes::Routed::S2C_RequestTeleport, pos, rot, animation);
}

void Peer::RouteParams(ZDOID targetZDO, avledet::util::Hash hash, avledet::util::Bytes params)
{
    Invoke(avledet::util::hashes::Rpc::RoutedRPC,
           RouteManager()->Serialize(AVL_ID, this->GetUserID(), targetZDO, hash, std::move(params)));
}

void Peer::ZDOSectorInvalidated(ZDO::reference zdo)
{
    if (zdo->IsOwner(this->GetUserID()))
        return;

    if (!ZoneManager()->ZonesOverlap(zdo->GetZone(), m_pos)) {
        if (m_zdos.erase(zdo->GetID())) {
            m_invalidSector.insert(zdo->GetID());
        }
    }
}

bool Peer::IsOutdatedZDO(ZDO::reference zdo, decltype(m_zdos)::iterator &outItr)
{
    auto &&find = m_zdos.find(zdo->GetID());

    outItr = find;

    return find == m_zdos.end() || zdo->GetOwnerRevision() > find->second.first.GetOwnerRevision()
           || zdo->GetDataRevision() > find->second.first.GetDataRevision();
}