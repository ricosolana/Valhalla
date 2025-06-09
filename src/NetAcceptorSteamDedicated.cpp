#include <isteamutils.h>
#include <magic_enum.hpp>
#include <quill/LogMacros.h>
#include <stdexcept>

#include "CompileSettings.h"
#include "NetAcceptor.h"
#include "ValhallaServer.h"
#include "steam_api_common.h"

std::unique_ptr<IAcceptor> IAcceptor::steam_user(bool is_lobby_server) {
    return std::make_unique<AcceptorSteam>(is_lobby_server);
}

// 
std::unique_ptr<IAcceptor> IAcceptor::steam_dedicated(std::string bind_addr) {
    return std::make_unique<AcceptorSteam>(std::move(bind_addr));
}



//SteamContext::SteamContext(bool use_game_server) {
//    if (use_game_server) {
//
//    }
//    else {
//
//    }
//}

std::pair<uint32_t, uint16_t> ip_to_machine_order(const std::string& ip) {
    uint32_t result = 0;
    size_t start = 0;
    size_t end = ip.find('.');

    //if (ip.starts_with("localhost")) {
    //    result = 0x7f000001;
    //}
    //else {
        //for (int i = 0; i < 4; ++i) {
        //    end = ip.find('.');
        //    result |= std::stoi(ip.substr(start, end - start)) << (24 - i * 8);
        //
        //    if (i == 3)
        //        break;
        //
        //    start = end + 1;
        //}

    for (int i = 0; i < 4; ++i) {
        if (end == std::string::npos && i < 3) {
            // throw if insufficient
            throw std::runtime_error("ip parsing failed");
        }

        // Convert the part to an integer and shift it to its position
        // test order
        result |= std::stoi(ip.substr(start, end - start)) << (24 - i * 8);

        // Move to the next part
        start = end + 1;
        end = ip.find('.', start);
    }
    //}

    int port = 0;

    auto idx = ip.find_last_of(":");
    if (idx != std::string::npos) {
        port = std::stoi(ip.substr(idx + 1));
    } else {
        throw std::runtime_error("port is required");
    }

    if (port < 0 || port > std::numeric_limits<uint16_t>::max()) {
        throw std::runtime_error("port is invalid");
    }

    return { result, (uint16_t)port };
}

AcceptorSteam::AcceptorSteam(bool is_lobby_server)
    : m_steamcallback_OnSteamStatusChanged(false),
        m_addr({})
     {
    //m_steamcallback_OnSteamStatusChanged.SetGameserverFlag();

    if (!SteamAPI_Init()) {
        throw std::runtime_error("unable to init steam game server api");
    }

    if (is_lobby_server) {
        auto handle = SteamMatchmaking()->CreateLobby(k_ELobbyTypePrivate, 64);
        m_lobbyCreatedCallResult.Set(handle, this, &AcceptorSteam::OnLobbyCreated);
    }

    LOG_INFO(VH_LOGGER, "Logged into steam as {}", SteamFriends()->GetPersonaName());

    // If we attempt connecting to a server prior to this returning 2 (success/..)
    //  socket receives CA BadCert
    auto auth = SteamNetworkingSockets()->InitAuthentication();

    // TODO use magic-enum
    LOG_INFO(VH_LOGGER, "Authentication status: {}", magic_enum::enum_name(auth)); // TODO magic enum

    // init auth session
    //GetAuthSessionTicketResponse_t

}

AcceptorSteam::AcceptorSteam(std::string bind_addr)
    : m_steamcallback_OnSteamStatusChanged(true)
{
    // host order, i.e 127.0.0.1 == 0x7f000001
    //SteamGameServer_InitEx(0x7f000001)
    
    auto [nIP, nPort] = ip_to_machine_order(bind_addr);

    {
        SteamErrMsg outErr{};
        auto result = SteamGameServer_InitEx(nIP, nPort, nPort + 1, EServerMode::eServerModeNoAuthentication, "1.0.0.0", &outErr);
        if (result != k_ESteamAPIInitResult_OK) {
            LOG_ERROR(VH_LOGGER, "{}", outErr);
            throw std::runtime_error("unable to init steam api");
        }
    }
    
    //if (!SteamGameServer_Init(nIP /*m_addr.GetIPv4()*/, nPort, nPort + 1, EServerMode::eServerModeNoAuthentication, "1.0.0.0"))
    //    throw std::runtime_error("unable to init steam game server api");

    m_addr.SetIPv4(nIP, nPort);

    SteamGameServer()->SetProduct("valheim");
    SteamGameServer()->SetModDir("valheim");
    SteamGameServer()->SetDedicatedServer(true);
    SteamGameServer()->SetMaxPlayerCount(64);
    SteamGameServer()->LogOnAnonymous();

    SteamGameServer()->SetGameTags(("\"gameversion\"=\""
        + std::string(VConstants::GAME) + "\",\"networkversion\"=\""
        + std::to_string(VConstants::NETWORK) + "\"").c_str()
    );

    SteamGameServer()->SetServerName("avledet");
    SteamGameServer()->SetMapName("avledet");
    SteamGameServer()->SetPasswordProtected(false);
    SteamGameServer()->SetAdvertiseServerActive(false);

    auto auth = SteamGameServerNetworkingSockets()->InitAuthentication();

    LOG_INFO(VH_LOGGER, "Starting server on port {}", m_addr.m_port);
    LOG_INFO(VH_LOGGER, "Server ID: {}", SteamGameServer()->GetSteamID().ConvertToUint64());
    LOG_INFO(VH_LOGGER, "Authentication status: {}", (int) auth); // TODO magic enum

    using namespace std::chrono_literals;

    auto timeout = (float)(30000ms).count();
    int32 offline = 1;
    int32 sendrate = 153600;
    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_TimeoutConnected,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Float, &timeout);
    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_IP_AllowWithoutAuth,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Int32, &offline);
    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_SendRateMin,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Int32, &sendrate);
    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_SendRateMax,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Int32, &sendrate);
}

AcceptorSteam::~AcceptorSteam() {
    this->stop();
}



std::vector<char> AcceptorSteam::get_auth_session_ticket() {
    this->cancel_auth_session_ticket();

    auto array = std::vector<char>(1024);
    unsigned int num = 0;
    SteamNetworkingIdentity id{};
    m_ticket = SteamUser()->GetAuthSessionTicket(array.data(), (int)array.size(), &num, &id);
    if (m_ticket != k_HAuthTicketInvalid) {
        array.resize(num);
        return array;
    }
    return {};
}

/*
bool AcceptorSteam::verify_auth_session_ticket(std::span<const char> ticket, ISocket::Ptr ptr) {
    auto socket = std::dynamic_pointer_cast<SteamSocket>(ptr);
    if (socket) {
        return SteamUser()->BeginAuthSession(
            ticket.data(), (int)ticket.size(), socket->m_steam_id.GetSteamID()) == k_EBeginAuthSessionResultOK;
    }
    std::unreachable();
}
*/

void AcceptorSteam::cancel_auth_session_ticket() {
    if (m_ticket != k_HAuthTicketInvalid) {
        SteamUser()->CancelAuthTicket(m_ticket);
        m_ticket = k_HAuthTicketInvalid;
        LOG_INFO(VH_LOGGER, "cancelled session ticket");
    }
}

void AcceptorSteam::start() {
    if (m_addr.m_port) {
        // TODO do not test based on port
        //  ie. p2p (logged in) server has no port, only lobby
        if (SteamSocket::is_game_server()) {
            m_listen_socket = SteamGameServerNetworkingSockets()->CreateListenSocketIP(m_addr, 0, nullptr);
        } else {
            m_listen_socket = SteamNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);
        }

        if (m_listen_socket == k_HSteamListenSocket_Invalid) {
            throw std::runtime_error("unable to create listen socket");
        }
    }

    using namespace std::chrono_literals;

    LOG_INFO(VH_LOGGER, "waiting for authentication");

    auto btime = std::chrono::steady_clock::now();

    while (SteamSocket::get_steam_sockets()->GetAuthenticationStatus(nullptr)
        != ESteamNetworkingAvailability::k_ESteamNetworkingAvailability_Current) {
        this->update();

        auto now = std::chrono::steady_clock::now();
        if (now - btime > 10s) {
            LOG_INFO(VH_LOGGER, "authentication took too long");
            std::exit(EXIT_FAILURE);
        }

        std::this_thread::sleep_for(1ms);
    }

    LOG_INFO(VH_LOGGER, "authenticated");
}

void AcceptorSteam::update() {
    if (SteamSocket::is_game_server())
        SteamGameServer_RunCallbacks();
    else
        SteamAPI_RunCallbacks();

    for (auto&& socket : m_ready) {
        m_connect_callback(std::move(socket));
    }
    m_ready.clear();
}

void AcceptorSteam::stop() {
    {
        this->cancel_auth_session_ticket();

        {
            for (auto&& socket : m_sockets)
                socket->flush();
        }

        // we end the scope prematurely to avoid
        // stealing the mutex during the sleep

        using namespace std::chrono_literals;

        // TODO is sleep really the best here?
        //  there is no great alternative
        if (m_listen_socket) {
            std::this_thread::sleep_for(1s);
        }

        {
            //std::scoped_lock scoped(m_mux);
            for (auto&& socket : m_sockets)
                socket->Close(false);
        }

        // TODO does this generate callbacks? 
        // if not, we can stop the thread earlier / immediately
        SteamSocket::get_steam_sockets()->CloseListenSocket(m_listen_socket);
        m_listen_socket = k_HSteamListenSocket_Invalid;

        if (SteamSocket::is_game_server())
            SteamGameServer_Shutdown();
        else
            SteamAPI_Shutdown();
    }
}

ISocket::Ptr AcceptorSteam::connect(std::string address) {
    SteamNetworkingIPAddr addr{};
    auto res = addr.ParseString(address.c_str());
    if (!res)
        throw std::runtime_error("invalid address");

    //SteamGameServer()->LogOnAnonymous
    return *m_sockets.insert(m_sockets.end(),
        std::make_shared<SteamSocket>(
            SteamSocket::get_steam_sockets()->ConnectByIPAddress(addr, 0, nullptr),
            true
        )
    );
}

void AcceptorSteam::on_connect(std::function<void(ISocket::Ptr)> callback) {
    m_connect_callback = callback;
}



void AcceptorSteam::OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t* data) {
    // Client has no listen socket; obviously we are not 'listening' for incoming connections
    auto im_client = data->m_info.m_hListenSocket == k_HSteamListenSocket_Invalid;

    LOG_INFO(VH_LOGGER, "status: {} -> {} (im client: {})",
        magic_enum::enum_name(data->m_eOldState), magic_enum::enum_name(data->m_info.m_eState), // TODO magic enum?
        (im_client ? "true" : "false")
    );

    //std::scoped_lock scoped(m_mux);

    // Following a disposal-on-disconnect form of sockets
    // Steam internals are respected, but closed sockets become invalidated
    // Reusability is not supported, where create() socket is required
    // for any continuous connections

    // Try to ignore outgoing connections

    auto&& socket_itr = this->get_socket(m_sockets, data->m_hConn);
    auto&& socket = socket_itr != m_sockets.end() ? *socket_itr : nullptr; // take ownership

    if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected
        && (data->m_eOldState == k_ESteamNetworkingConnectionState_FindingRoute ||
            data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)) {
        if (socket) {
            if (im_client) {
                LOG_INFO(VH_LOGGER, "outbound socket connected {}", data->m_hConn);
            } else {
                LOG_INFO(VH_LOGGER, "connected socket queued {}", data->m_hConn);
            }
            socket->m_status = Status::Connected;
            m_ready.push_back(socket);
        } else {
            assert(false);
            // unexpected
            //LOG_DEBUG(VH_LOGGER, "connected socket missing {}", data->m_hConn);
        }
    } else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting
        && data->m_eOldState == k_ESteamNetworkingConnectionState_None) {
        // ListenSocket will be invalid when the connection is not an incoming client
        //  (when we initiated the connection)
        if (im_client) {
            if (socket) {
                //socket->m_status = Status::Connecting;
                LOG_INFO(VH_LOGGER, "outbound socket connecting {}", data->m_hConn);
            } else {
                assert(false);
            }
        } else {
            assert(!socket);
            if (SteamSocket::get_steam_sockets()->AcceptConnection(data->m_hConn) == k_EResultOK) {
                socket = m_sockets.emplace_back(std::make_shared<SteamSocket>(data->m_hConn, false));
                //socket->m_status = Status::Connecting;
                LOG_INFO(VH_LOGGER, "inbound socket connecting {}", data->m_hConn);
            } else {
                //TODO
                //  this branch (and all other old->new state branches above)
                //  are possible to be reached due to steam shenanigans... so handle accordingly
                assert(false);
                //LOG_ERROR(VH_LOGGER, "failed to accept connecting socket ", data->m_hConn);
            }
        }
    } else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally
        || data->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer
        || (data->m_info.m_eState == k_ESteamNetworkingConnectionState_None
            && data->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
        ) {
        if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
            LOG_INFO(VH_LOGGER, "{}", data->m_info.m_szEndDebug);

        if (socket) {
            socket->Close(false);

            auto&& ready_itr = this->get_socket(m_ready, data->m_hConn);
            if (ready_itr != m_ready.end())
                m_ready.erase(ready_itr);

            socket->m_conn = k_HSteamNetConnection_Invalid;

            // Finally erase
            m_sockets.erase(socket_itr);
        } else {
            assert(false);
            //LOG_ERROR(VH_LOGGER, "closing socket not found {}", data->m_hConn);
        }
    }
}

/*
void AcceptorSteam::OnSteamServersConnected(SteamServersConnected_t* data) {
    LOG_INFO(VH_LOGGER, "Steam server connected");
}

void AcceptorSteam::OnSteamServersDisconnected(SteamServersDisconnected_t* data) {
    LOG_INFO(VH_LOGGER, "Steam server disconnected");
}

void AcceptorSteam::OnSteamServerConnectFailure(SteamServerConnectFailure_t* data) {
    LOG_WARNING(VH_LOGGER, "Steam server connect failure");
}*/


// auth session ticket has nothing to do with steam networking sockets handshake
void AcceptorSteam::OnAuthSessionTicketResponse(GetAuthSessionTicketResponse_t* data) {
    (void)data;
    LOG_INFO(VH_LOGGER, "auth session response callback");
}



// call results
void AcceptorSteam::OnLobbyCreated(LobbyCreated_t* data, bool failure) {
    if (failure) {
        LOG_ERROR(VH_LOGGER, "Failed to create lobby");
    } else if (data->m_eResult == k_EResultNoConnection) {
        LOG_ERROR(VH_LOGGER, "Failed to connect to Steam to register lobby");
    } else {
        this->m_lobbyID = CSteamID(data->m_ulSteamIDLobby);

        LOG_INFO(VH_LOGGER, "Created lobby");

        if (!SteamMatchmaking()->SetLobbyType(m_lobbyID, k_ELobbyTypeFriendsOnly)) { //VH_SETTINGS.serverPublic ? k_ELobbyTypePublic : k_ELobbyTypeFriendsOnly)) {
            LOG_ERROR(VH_LOGGER, "Failed to set lobby visibility");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "name", "avledet")) {
            LOG_ERROR(VH_LOGGER, "Failed to set lobby name");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "password", "0")) {
            LOG_ERROR(VH_LOGGER, "Unable to set lobby password flag");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "version", VConstants::GAME)) {
            LOG_WARNING(VH_LOGGER, "Unable to set lobby version");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "networkversion", std::to_string(VConstants::NETWORK).c_str())) {
            LOG_WARNING(VH_LOGGER, "Failed to set lobby networkversion");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "serverType", "Steam user")) {
            LOG_WARNING(VH_LOGGER, "Failed to set lobby serverType");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "hostID", "")) {
            LOG_WARNING(VH_LOGGER, "Failed to set lobby host");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "isCrossplay", "0")) {
            LOG_WARNING(VH_LOGGER, "Failed to set lobby isCrossplay");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "modifiers", "0")) {
            LOG_WARNING(VH_LOGGER, "Failed to set lobby isCrossplay");
        }

        SteamMatchmaking()->SetLobbyGameServer(m_lobbyID, 0, 0, SteamUser()->GetSteamID());
    }
}

//bool SteamContext::is_game_server() {
//    auto game_server = SteamGameServerNetworkingSockets();
//    return game_server != nullptr;
//}
//
//ISteamNetworkingSockets* SteamContext::get_steam_sockets() {
//    auto game_server = SteamGameServerNetworkingSockets();
//    return game_server ? game_server : SteamNetworkingSockets();
//}