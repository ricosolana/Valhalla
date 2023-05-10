#include <isteamutils.h>

#include "NetAcceptor.h"
#include "ValhallaServer.h"

AcceptorSteamP2P::AcceptorSteamP2P() {

    // This forces the Valheim Client to startup for some reason
    //if (SteamAPI_RestartAppIfNecessary(VALHEIM_APP_ID)) {
    //    //LOG(INFO) << "Restarting app through Steam";
    //    exit(0);
    //}

    if (!SteamAPI_Init()) {
        LOG_CRITICAL(LOGGER, "Failed to init steam api (steam_appid.txt missing or not logged in?)");
        std::exit(0);
    }

    LOG_INFO(LOGGER, "Logged into steam as {}", SteamFriends()->GetPersonaName());
    
    LOG_INFO(LOGGER, "Authentication status: {}", SteamNetworkingSockets()->InitAuthentication());

    //auto handle = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, 64);
    auto handle = SteamMatchmaking()->CreateLobby(VH_SETTINGS.serverPublic ? k_ELobbyTypePublic : k_ELobbyTypeFriendsOnly, 64);
    m_lobbyCreatedCallResult.Set(handle, this, &AcceptorSteamP2P::OnLobbyCreated);
    
    auto timeout = (float) duration_cast<milliseconds>(Valhalla()->Settings().playerTimeout).count();
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

AcceptorSteamP2P::~AcceptorSteamP2P() {
    if (m_listenSocket != k_HSteamListenSocket_Invalid) {
        //VLOG(1) << "~AcceptorSteamP2P()";
        for (auto&& socket : m_sockets)
            socket.second->Close(true);

        std::this_thread::sleep_for(1s);

        SteamNetworkingSockets()->CloseListenSocket(m_listenSocket);

        m_listenSocket = k_HSteamListenSocket_Invalid;
    }

    SteamAPI_Shutdown();
}

void AcceptorSteamP2P::Listen() {
    this->m_listenSocket = SteamNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);
}

ISocket::Ptr AcceptorSteamP2P::Accept() {
    auto pair = m_connected.begin();
    if (pair == m_connected.end())
        return nullptr;
    auto&& socket = std::move(pair->second);
    m_connected.erase(pair);
    return socket;
}



static const char* stateToString(ESteamNetworkingConnectionState state) {
    static const char* messages[] = { "None", "Connecting", "FindingRoute", "Connected", "ClosedByPeer", "ProblemDetectedLocally",
        "FinWait", "Linger", "Dead"
    };
    if (state >= 0 && state < sizeof(messages) / sizeof(messages[0])) {
        return messages[state];
    }
    return messages[5 + (-state)];
}

void AcceptorSteamP2P::OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t* data) {
    LOG_INFO(LOGGER, "NetConnectionStatusChanged: {}, old: ", stateToString(data->m_info.m_eState), stateToString(data->m_eOldState));

    if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected
        && (data->m_eOldState == k_ESteamNetworkingConnectionState_FindingRoute 
            || data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting))
    {
        m_connected[data->m_hConn] = m_sockets[data->m_hConn];
    }
    else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting
        && data->m_eOldState == k_ESteamNetworkingConnectionState_None)
    {
        if (SteamNetworkingSockets()->AcceptConnection(data->m_hConn) == k_EResultOK)
            m_sockets[data->m_hConn] = std::make_shared<SteamSocket>(data->m_hConn);
    }
    else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally
        || data->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
    {
        if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
            LOG_INFO(LOGGER, "{}", data->m_info.m_szEndDebug);

        auto&& pair = m_sockets.find(data->m_hConn);

        if (pair != m_sockets.end()) {
            pair->second->Close(false);

            m_connected.erase(data->m_hConn);
            m_sockets.erase(pair);
        }
    }
}

void AcceptorSteamP2P::OnSteamServersConnected(SteamServersConnected_t* data) {
    LOG_INFO(LOGGER, "Steam server connected");
}

void AcceptorSteamP2P::OnSteamServersDisconnected(SteamServersDisconnected_t* data) {
    LOG_INFO(LOGGER, "Steam server disconnected");
}

void AcceptorSteamP2P::OnSteamServerConnectFailure(SteamServerConnectFailure_t* data) {
    LOG_WARNING(LOGGER, "Steam server connect failure");
}



// call results
void AcceptorSteamP2P::OnLobbyCreated(LobbyCreated_t* data, bool failure) {
    if (failure) {
        LOG_ERROR(LOGGER, "Failed to create lobby");
    }
    else if (data->m_eResult == k_EResultNoConnection) {
        LOG_ERROR(LOGGER, "Failed to connect to Steam to register lobby");
    }
    else {
        this->m_lobbyID = CSteamID(data->m_ulSteamIDLobby);

        LOG_INFO(LOGGER, "Created lobby");

        this->OnConfigLoad(false);

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "version", VConstants::GAME)) {
            LOG_WARNING(LOGGER, "Unable to set lobby version");
        }
        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "networkversion", std::to_string(VConstants::NETWORK).c_str())) {
            LOG_WARNING(LOGGER, "Failed to set lobby networkversion");
        }
        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "serverType", "Steam user")) {
            LOG_WARNING(LOGGER, "Failed to set lobby serverType");
        }
        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "hostID", "")) {
            LOG_WARNING(LOGGER, "Failed to set lobby host");
        }
        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "isCrossplay", "0")) {
            LOG_WARNING(LOGGER, "Failed to set lobby isCrossplay");
        }

        SteamMatchmaking()->SetLobbyGameServer(m_lobbyID, 0, 0, SteamUser()->GetSteamID());
    }
}



void AcceptorSteamP2P::OnConfigLoad(bool reloading) {
    if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "name", VH_SETTINGS.serverName.c_str())) {
        LOG_ERROR(LOGGER, "Failed to set lobby name");
    }
    
    if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "password", VH_SETTINGS.serverPassword.empty() ? "0" : "1")) {
        LOG_ERROR(LOGGER, "Unable to set lobby password flag");
    }
}
