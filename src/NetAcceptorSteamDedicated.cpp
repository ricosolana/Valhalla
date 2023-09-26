#include <isteamutils.h>

#include "NetAcceptor.h"
#include "ValhallaServer.h"

ISteamNetworkingSockets* AcceptorSteam::STEAM_NETWORKING_SOCKETS {};

AcceptorSteam::AcceptorSteam() {

    // This forces the Valheim Client to startup for some reason
    //  TODO fix this properly
    //if (SteamAPI_RestartAppIfNecessary(VALHEIM_APP_ID)) {
    //    //LOG(INFO) << "Restarting app through Steam";
    //    exit(0);
    //}

    if (!(VH_SETTINGS.serverDedicated 
        ? SteamGameServer_Init(0, VH_SETTINGS.serverPort, VH_SETTINGS.serverPort + 1, EServerMode::eServerModeNoAuthentication, "1.0.0.0")
        : SteamAPI_Init())) {
        LOG_CRITICAL(LOGGER, "Failed to init steam");
        std::exit(0);
    }

    if (VH_SETTINGS.serverDedicated) {
        //this->m_steamNetworkingSockets = SteamGameServerNetworkingSockets();
        STEAM_NETWORKING_SOCKETS = SteamGameServerNetworkingSockets();

        SteamGameServer()->SetProduct("valheim");   // for version checking
        SteamGameServer()->SetModDir("valheim");    // game save location
        SteamGameServer()->SetDedicatedServer(true);
        SteamGameServer()->SetMaxPlayerCount(64);
        SteamGameServer()->LogOnAnonymous();        // no steam login necessary
        
        //SteamGameServer()->SetGameTags(VConstants::GAME);
        SteamGameServer()->SetGameTags(("\"gameversion\"=\"" 
            + std::string(VConstants::GAME) + "\",\"networkversion\"=\""
            + std::to_string(VConstants::NETWORK) + "\"").c_str()
        );
        
        this->OnConfigLoad(false);

        LOG_INFO(LOGGER, "Starting server on port {}", VH_SETTINGS.serverPort);
        LOG_INFO(LOGGER, "Server ID: {}", SteamGameServer()->GetSteamID().ConvertToUint64());
    }
    else {
        //this->m_steamNetworkingSockets = SteamNetworkingSockets();
        STEAM_NETWORKING_SOCKETS = SteamNetworkingSockets();

        auto handle = SteamMatchmaking()->CreateLobby(k_ELobbyTypePrivate, 64);
        m_lobbyCreatedCallResult.Set(handle, this, &AcceptorSteam::OnLobbyCreated);

        LOG_INFO(LOGGER, "Logged into steam as {}", SteamFriends()->GetPersonaName());
    }

    LOG_INFO(LOGGER, "Authentication status: {}", STEAM_NETWORKING_SOCKETS->InitAuthentication());
    
    auto timeout = (float)duration_cast<milliseconds>(Valhalla()->Settings().playerTimeout).count();
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
    if (m_listenSocket != k_HSteamListenSocket_Invalid) {
        //LOG(DEBUG) << "Destroying";
        for (auto &&socket : m_sockets)
            socket.second->Close(true);

        // TODO is sleep really the best here?
        std::this_thread::sleep_for(1s);

        STEAM_NETWORKING_SOCKETS->CloseListenSocket(m_listenSocket);

        m_listenSocket = k_HSteamListenSocket_Invalid;
    }

    if (VH_SETTINGS.serverDedicated)
        SteamGameServer_Shutdown();
    else
        SteamAPI_Shutdown();
}

void AcceptorSteam::Listen() {
    if (VH_SETTINGS.serverDedicated) {
        SteamNetworkingIPAddr steamNetworkingIPAddr{ .m_port = VH_SETTINGS.serverPort };
        this->m_listenSocket = STEAM_NETWORKING_SOCKETS->CreateListenSocketIP(steamNetworkingIPAddr, 0, nullptr);
    }
    else {
        this->m_listenSocket = STEAM_NETWORKING_SOCKETS->CreateListenSocketP2P(0, 0, nullptr);
    }
}

ISocket::Ptr AcceptorSteam::Accept() {
    auto pair = m_connected.begin();
    if (pair == m_connected.end())
        return nullptr;
    auto &&socket = std::move(pair->second);
    m_connected.erase(pair);
    return socket;
}



static const char* stateToString(ESteamNetworkingConnectionState state) {
    static const char* messages[] = { "None", "Connecting", "FindingRoute", "Connected", "ClosedByPeer", "ProblemDetectedLocally",
        "FinWait", "Linger", "Dead"
    };
    if (state >= 0 && state < sizeof(messages)/sizeof(messages[0])) {
        return messages[state];
    } 
    return messages[5 + (-state)];
}

void AcceptorSteam::OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t *data) {
    LOG_INFO(LOGGER, "NetConnectionStatusChanged: {}, old: {}", stateToString(data->m_info.m_eState), stateToString(data->m_eOldState));

    if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected
        && (data->m_eOldState == k_ESteamNetworkingConnectionState_FindingRoute ||
            data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting))
    {
        m_connected[data->m_hConn] = m_sockets[data->m_hConn];
    }
    else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting 
        && data->m_eOldState == k_ESteamNetworkingConnectionState_None)
    {
        if (STEAM_NETWORKING_SOCKETS->AcceptConnection(data->m_hConn) == k_EResultOK)
            m_sockets[data->m_hConn] = std::make_shared<SteamSocket>(data->m_hConn);
    }
    else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally
        || data->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
    {
        if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
            LOG_INFO(LOGGER, "{}", data->m_info.m_szEndDebug);

        auto &&pair = m_sockets.find(data->m_hConn);

        if (pair != m_sockets.end()) {
            pair->second->Close(false);

            m_connected.erase(data->m_hConn);
            m_sockets.erase(pair);
        }
    }
}

/*
void AcceptorSteam::OnSteamServersConnected(SteamServersConnected_t* data) {
    LOG_INFO(LOGGER, "Steam server connected");
}

void AcceptorSteam::OnSteamServersDisconnected(SteamServersDisconnected_t* data) {
    LOG_INFO(LOGGER, "Steam server disconnected");
}

void AcceptorSteam::OnSteamServerConnectFailure(SteamServerConnectFailure_t* data) {
    LOG_WARNING(LOGGER, "Steam server connect failure");
}*/



// call results
void AcceptorSteam::OnLobbyCreated(LobbyCreated_t* data, bool failure) {
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
        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "modifiers", "0")) {
            LOG_WARNING(LOGGER, "Failed to set lobby isCrossplay");
        }
        
        SteamMatchmaking()->SetLobbyGameServer(m_lobbyID, 0, 0, SteamUser()->GetSteamID());
    }
}



void AcceptorSteam::OnConfigLoad(bool reloading) {
    if (VH_SETTINGS.serverDedicated) {
        SteamGameServer()->SetServerName(VH_SETTINGS.serverName.c_str());
        SteamGameServer()->SetMapName(VH_SETTINGS.serverName.c_str());
        SteamGameServer()->SetPasswordProtected(!VH_SETTINGS.serverPassword.empty());

        SteamGameServer()->SetAdvertiseServerActive(VH_SETTINGS.serverPublic);
    } else {
        if (!SteamMatchmaking()->SetLobbyType(m_lobbyID, VH_SETTINGS.serverPublic ? k_ELobbyTypePublic : k_ELobbyTypeFriendsOnly)) {
            LOG_ERROR(LOGGER, "Failed to set lobby visibility");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "name", VH_SETTINGS.serverName.c_str())) {
            LOG_ERROR(LOGGER, "Failed to set lobby name");
        }

        if (!SteamMatchmaking()->SetLobbyData(m_lobbyID, "password", VH_SETTINGS.serverPassword.empty() ? "0" : "1")) {
            LOG_ERROR(LOGGER, "Unable to set lobby password flag");
        }
    }
}
