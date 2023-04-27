#include <isteamutils.h>

#include "NetAcceptor.h"
#include "ValhallaServer.h"

AcceptorSteamDedicated::AcceptorSteamDedicated()
    : m_port(Valhalla()->Settings().serverPort) {

    // This forces the Valheim Client to startup for some reason
    //if (SteamAPI_RestartAppIfNecessary(VALHEIM_APP_ID)) {
    //    LOG(INFO) << "Restarting app through Steam";
    //    exit(0);
    //}

    if (!SteamGameServer_Init(0, m_port, m_port + 1, EServerMode::eServerModeNoAuthentication, "1.0.0.0")) {
        LOG(FATAL) << "Failed to init steam game server (steam_appid.txt missing?)";
    }

    SteamGameServer()->SetProduct("valheim");   // for version checking
    SteamGameServer()->SetModDir("valheim");    // game save location
    SteamGameServer()->SetDedicatedServer(true);
    SteamGameServer()->SetMaxPlayerCount(64);
    SteamGameServer()->LogOnAnonymous();        // no steam login necessary

    LOG(INFO) << "Starting server on port " << m_port;
    LOG(INFO) << "Server ID: " << SteamGameServer()->GetSteamID().ConvertToUint64();
    LOG(INFO) << "Authentication status: " << SteamGameServerNetworkingSockets()->InitAuthentication();

    SteamGameServer()->SetServerName(Valhalla()->Settings().serverName.c_str());
    SteamGameServer()->SetMapName(Valhalla()->Settings().serverName.c_str());
    SteamGameServer()->SetPasswordProtected(!Valhalla()->Settings().serverPassword.empty());
    SteamGameServer()->SetGameTags(VConstants::GAME);
    SteamGameServer()->SetAdvertiseServerActive(Valhalla()->Settings().serverPublic);

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

AcceptorSteamDedicated::~AcceptorSteamDedicated() {
    if (m_listenSocket != k_HSteamListenSocket_Invalid) {
        LOG(DEBUG) << "Destroying";
        for (auto &&socket : m_sockets)
            socket.second->Close(true);

        SteamGameServerNetworkingSockets()->CloseListenSocket(m_listenSocket);

        m_listenSocket = k_HSteamListenSocket_Invalid;
    }

    SteamGameServer_Shutdown();
}

void AcceptorSteamDedicated::Listen() {
    SteamNetworkingIPAddr steamNetworkingIPAddr{};    // nullify or whatever (default)
    steamNetworkingIPAddr.Clear();                  // this is important, otherwise bind is invalid
    steamNetworkingIPAddr.m_port = m_port;          // it is later reassigned by FejdManager
    this->m_listenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketIP(steamNetworkingIPAddr, 0, nullptr);
}

ISocket::Ptr AcceptorSteamDedicated::Accept() {
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

void AcceptorSteamDedicated::OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t *data) {
    LOG(INFO) << "NetConnectionStatusChanged: " << stateToString(data->m_info.m_eState) << ", old: " << stateToString(data->m_eOldState);

    if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected
        && data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
    {
        m_connected[data->m_hConn] = m_sockets[data->m_hConn];
    }
    else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting 
        && data->m_eOldState == k_ESteamNetworkingConnectionState_None)
    {
        if (SteamGameServerNetworkingSockets()->AcceptConnection(data->m_hConn) == k_EResultOK)
            m_sockets[data->m_hConn] = std::make_shared<SteamSocket>(data->m_hConn);
    }
    else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally
        || data->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
    {
        if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
            LOG(INFO) << data->m_info.m_szEndDebug;

        auto &&pair = m_sockets.find(data->m_hConn);

        if (pair != m_sockets.end()) {
            pair->second->Close(false);

            m_connected.erase(data->m_hConn);
            m_sockets.erase(pair);
        }
    }
}

void AcceptorSteamDedicated::OnSteamServersConnected(SteamServersConnected_t* data) {
    LOG(INFO) << "Steam server connected";
}

void AcceptorSteamDedicated::OnSteamServersDisconnected(SteamServersDisconnected_t* data) {
    LOG(INFO) << "Steam server disconnected";
}

void AcceptorSteamDedicated::OnSteamServerConnectFailure(SteamServerConnectFailure_t* data) {
    LOG(INFO) << "Steam server connect failure";
}
