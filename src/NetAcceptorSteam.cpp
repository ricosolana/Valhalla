#include "NetAcceptor.h"
#include <isteamutils.h>

AcceptorSteam::AcceptorSteam(const std::string& name, 
	bool hasPassword, 
	uint16_t port,
	bool isPublic,
	float timeout)
	: m_port(port) {

	if (SteamAPI_RestartAppIfNecessary(VALHEIM_APP_ID)) {
		LOG(INFO) << "Restarting app through Steam";
		exit(0);
	}

	if (!SteamGameServer_Init(0, port, port + 1, EServerMode::eServerModeNoAuthentication, "1.0.0.0")) {
		LOG(ERROR) << "Failed to init steam game server";
		exit(0);
	}

	SteamGameServer()->SetProduct("valheim");   // for version checking
	SteamGameServer()->SetModDir("valheim");    // game save location
	SteamGameServer()->SetDedicatedServer(true);
	SteamGameServer()->SetMaxPlayerCount(64);
	SteamGameServer()->LogOnAnonymous();        // no steam login necessary
		
	LOG(INFO) << "Starting server on port " << port;
	LOG(INFO) << "Server ID: " << SteamGameServer()->GetSteamID().ConvertToUint64();
	LOG(INFO) << "Authentication status: " << SteamGameServerNetworkingSockets()->InitAuthentication();

	SteamGameServer()->SetServerName(name.c_str());
	SteamGameServer()->SetMapName(name.c_str());
	SteamGameServer()->SetPasswordProtected(hasPassword);
	SteamGameServer()->SetGameTags(VALHEIM_VERSION);
	SteamGameServer()->SetAdvertiseServerActive(isPublic);

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
	SteamNetworkingIPAddr steamNetworkingIPAddr; // nullify or whatever (default)
	steamNetworkingIPAddr.Clear(); // this is important, otherwise server wouldnt open listen socket
	steamNetworkingIPAddr.m_port = m_port; // it is later reassigned by fejd manager
	this->m_listenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketIP(steamNetworkingIPAddr, 0, nullptr);
}

ISocket::Ptr AcceptorSteam::Accept() {
	if (m_readyAccepted.empty())
		return nullptr;

	auto pair = m_readyAccepted.begin();
	auto socket = pair->second; // itr becomes invalid, so store ptr
	m_readyAccepted.erase(pair);
	return socket;
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
		auto pair = m_pendingConnect.find(data->m_hConn);
		if (pair != m_pendingConnect.end()) {
			auto socket = pair->second;

			SteamNetConnectionInfo_t steamNetConnectionInfo_t;
			if (SteamGameServerNetworkingSockets()->GetConnectionInfo(data->m_hConn, &steamNetConnectionInfo_t)) {
				socket->m_steamID = steamNetConnectionInfo_t.m_identityRemote;
			}

			LOG(INFO) << "Connection ready, SteamID: " << socket->m_steamID.GetSteamID64();
			m_readyAccepted.insert({ data->m_hConn, socket });
			m_pendingConnect.erase(pair);
		}
	}
	else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting 
		&& data->m_eOldState == k_ESteamNetworkingConnectionState_None)
	{
		auto eresult = SteamGameServerNetworkingSockets()->AcceptConnection(data->m_hConn);
		LOG(INFO) << "Accepting new connection " << eresult;
		if (eresult == k_EResultOK) {
			auto ptr = std::make_shared<SteamSocket>(data->m_hConn);
			m_pendingConnect.insert({ data->m_hConn, ptr });
			m_sockets.insert({ data->m_hConn, ptr });
		}
	}
	else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally
		|| data->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
	{		
		if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			LOG(INFO) << data->m_info.m_szEndDebug;

		auto pair = m_sockets.find(data->m_hConn);
		if (pair != m_sockets.end())
			pair->second->Close();

		m_pendingConnect.erase(data->m_hConn);
		m_readyAccepted.erase(data->m_hConn);
		m_sockets.erase(pair);
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
