#include <isteamutils.h>

#include "NetAcceptor.h"
#include "ValhallaServer.h"

AcceptorSteam::AcceptorSteam()
	: m_port(Valhalla()->Settings().serverPort) {

	if (SteamAPI_RestartAppIfNecessary(VALHEIM_APP_ID)) {
		LOG(INFO) << "Restarting app through Steam";
		exit(0);
	}

	if (!SteamGameServer_Init(0, m_port, m_port + 1, EServerMode::eServerModeNoAuthentication, "1.0.0.0")) {
		LOG(ERROR) << "Failed to init steam game server";
		exit(0);
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
	SteamGameServer()->SetGameTags(VALHEIM_VERSION);
	SteamGameServer()->SetAdvertiseServerActive(Valhalla()->Settings().serverPublic);

    float timeout = (float)Valhalla()->Settings().socketTimeout.count();
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
		LOG(INFO) << "Stopping Steam listening socket";
		SteamGameServerNetworkingSockets()->CloseListenSocket(m_listenSocket);
		m_listenSocket = k_HSteamListenSocket_Invalid;
	}

	SteamGameServer_Shutdown();
}

void AcceptorSteam::Listen() {
	SteamNetworkingIPAddr steamNetworkingIPAddr;    // nullify or whatever (default)
	steamNetworkingIPAddr.Clear();                  // this is important, otherwise bind is invalid
	steamNetworkingIPAddr.m_port = m_port;          // it is later reassigned by FejdManager
	this->m_listenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketIP(steamNetworkingIPAddr, 0, nullptr);
}

ISocket *AcceptorSteam::Accept() {
    auto pair = m_connected.begin();
    if (pair == m_connected.end())
        return nullptr;
    auto socket = pair->second;
    auto itr = m_connected.erase(pair);
    return socket;
}

void AcceptorSteam::Cleanup(ISocket* socket) {
    auto* steamSocket = dynamic_cast<SteamSocket*>(socket);
    assert(steamSocket && "Received a non steam socket in SteamAcceptor!");

    auto pair = m_sockets.find(steamSocket->m_hConn);
    if (pair != m_sockets.end())
        socket->Close();

    m_sockets.erase(pair);
}


const char* stateToString(ESteamNetworkingConnectionState state) {
	static const char* messages[] = { "None", "Connecting", "FindingRoute", "Connected", "ClosedByPeer", "ProblemDetectedLocally",
		"FinWait", "Linger", "Dead"
	};
	if (state >= 0 && state <= 5) {
		return messages[state];
	}
	return messages[5 + (-state)];
}



void AcceptorSteam::OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t *data) {
	LOG(INFO) << "Connection status changed: " << stateToString(data->m_info.m_eState);
	if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected
		&& data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
	{
		auto pair = m_connecting.find(data->m_hConn);
		if (pair != m_connecting.end()) {
			auto socket = pair->second;

			SteamNetConnectionInfo_t steamNetConnectionInfo_t;
			if (SteamGameServerNetworkingSockets()->GetConnectionInfo(data->m_hConn, &steamNetConnectionInfo_t)) {
				socket->m_steamNetId = steamNetConnectionInfo_t.m_identityRemote;
			}

            // Under what circumstances should an assign be used vs an insert
            // insert has behaviour to determine beforehand whether an element existed
			LOG(INFO) << "Connection ready, SteamID: " << socket->m_steamNetId.GetSteamID64();
			m_connected[data->m_hConn] = socket;
			m_connecting.erase(pair);
		}
	}
	else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting 
		&& data->m_eOldState == k_ESteamNetworkingConnectionState_None)
	{
		auto eresult = SteamGameServerNetworkingSockets()->AcceptConnection(data->m_hConn);
		LOG(INFO) << "Accepting new connection " << eresult;
		if (eresult == k_EResultOK) {
			auto ptr = std::make_unique<SteamSocket>(data->m_hConn);    // First time connecting, so alloc
			m_connecting[data->m_hConn] = ptr.get();
			m_sockets[data->m_hConn] = std::move(ptr);
		}
	}
	else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally
		|| data->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
	{
		if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			LOG(INFO) << data->m_info.m_szEndDebug;

        // Determine that the socket was correctly opened since the connection began
		auto pair = m_sockets.find(data->m_hConn);
		if (pair == m_sockets.end()) {
            LOG(ERROR) << "Unable to find disconnecting socket";
            return;
        }

        // Remove if was ready for external accept/polling
        m_connected.erase(data->m_hConn);

        // Remote if was in initial connecting state
        auto erased = m_connecting.erase(data->m_hConn);
        if (erased) // Internally free (never became external)
            m_sockets.erase(pair);
        else // Close it so external can respond
            pair->second->Close();
	}
}

void AcceptorSteam::OnSteamServersConnected(SteamServersConnected_t* data) {
	LOG(INFO) << "Server connected";
}

void AcceptorSteam::OnSteamServersDisconnected(SteamServersDisconnected_t* data) {
	LOG(INFO) << "Server disconnected";
}

void AcceptorSteam::OnSteamServerConnectFailure(SteamServerConnectFailure_t* data) {
	LOG(INFO) << "Server connect failure";
}
