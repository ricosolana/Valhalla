#include "NetRpc.h"
#include "Hashes.h"
#include "ValhallaServer.h"
#include "NetManager.h"

NetRpc::NetRpc(ISocket::Ptr socket)
    : m_socket(std::move(socket)), m_lastPing(steady_clock::now())
{
    Register(
        Hashes::Rpc::Disconnect, [](NetRpc* self) {
            LOG(INFO) << "RPC_Disconnect";
            self->m_socket->Close(true);
        }
    );

    Register(Hashes::Rpc::C2S_Handshake, [](NetRpc* rpc, BYTES_t pkg) {
        rpc->Register(Hashes::Rpc::PeerInfo, [](NetRpc* rpc, BYTES_t bytes) {
            // Forward call to rpc

            DataReader reader(bytes);

            auto uuid = reader.Read<OWNER_t>();
            if (!uuid)
                throw std::runtime_error("peer provided 0 owner");

            auto version = reader.Read<std::string>();
            LOG(INFO) << "Client " << rpc->m_socket->GetHostName() << " has version " << version;
            if (version != VConstants::GAME)
                return rpc->Close(ConnectionStatus::ErrorVersion);

            auto pos = reader.Read<Vector3>();
            auto name = reader.Read<std::string>();
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



            if (password != rpc->m_password)
                return rpc->Close(ConnectionStatus::ErrorPassword);



            // if peer already connected
            if (NetManager()->GetPeer(uuid))
                return rpc->Close(ConnectionStatus::ErrorAlreadyConnected);

            // if whitelist enabled
            if (SERVER_SETTINGS.playerWhitelist
                && !Valhalla()->m_whitelist.contains(rpc->m_socket->GetHostName())) {
                return rpc->Close(ConnectionStatus::ErrorFull);
            }

            // if too many players online
            if (NetManager()->GetPeers().size() >= SERVER_SETTINGS.playerMax)
                return rpc->Close(ConnectionStatus::ErrorFull);

            NetManager()->OnNewClient(rpc->m_socket, uuid, name, pos);
        });

        if (!SERVER_SETTINGS.serverPassword.empty()) {
            std::string salt = VUtils::Random::GenerateAlphaNum(16);

            const auto merge = SERVER_SETTINGS.serverPassword + salt;

            // Hash a salted password
            rpc->m_password.resize(16);
            MD5(reinterpret_cast<const uint8_t*>(merge.c_str()),
                merge.size(), reinterpret_cast<uint8_t*>(rpc->m_password.data()));

            rpc->Invoke(Hashes::Rpc::S2C_Handshake, true, salt);
        }
        else {
            rpc->Invoke(Hashes::Rpc::S2C_Handshake, false, "");
        }
    });
}

void NetRpc::PollOne() {
    m_socket->Update();

    auto opt = m_socket->Recv();
    if (!opt)
        return;

    auto now(steady_clock::now());

    auto&& bytes = opt.value();

    DataReader pkg(bytes);

    auto hash = pkg.Read<HASH_t>();
    if (hash == 0) {
        if (pkg.Read<bool>()) {
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
        auto&& find = m_methods.find(hash);
        if (find != m_methods.end()) {
            find->second->Invoke(this, pkg);
        }
    }
}

static const char* STATUS_STRINGS[] = { "None", "Connecting", "Connected",
    "ErrorVersion", "ErrorDisconnected", "ErrorConnectFailed", "ErrorPassword",
    "ErrorAlreadyConnected", "ErrorBanned", "ErrorFull" };

void NetRpc::Close(ConnectionStatus status) {
    LOG(INFO) << "Client error: " << STATUS_STRINGS[(int)status];
    Invoke(Hashes::Rpc::S2C_Error, status);
    m_socket->Close(true);
}
