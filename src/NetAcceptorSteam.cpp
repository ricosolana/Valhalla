#include "NetAcceptor.h"
#include <isteamutils.h>

AcceptorSteam::AcceptorSteam(uint16_t port) 
	: m_port(port) {
	float timeout = 30000.0f;
	int32 offline = 1;
	int32 sendrate = 153600;

	//SteamGameServer()->

	//SteamGameServerUtils()->Cre

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
	Close();
}

void AcceptorSteam::Start() {
	SteamNetworkingIPAddr steamNetworkingIPAddr; // nullify or whatever (default)
	steamNetworkingIPAddr.Clear(); // this is important, otherwise server wouldnt open listen socket
	steamNetworkingIPAddr.m_port = m_port; // it is later reassigned by fejd manager
	this->m_listenSocket = SteamGameServerNetworkingSockets()->CreateListenSocketIP(steamNetworkingIPAddr, 0, nullptr);
}

void AcceptorSteam::Close() {
	// used for client connected socket only
	

	// server use only
	if (m_listenSocket != k_HSteamListenSocket_Invalid) {
		LOG(INFO) << "Stopping Steam listening socket";
		SteamGameServerNetworkingSockets()->CloseListenSocket(m_listenSocket);
		m_listenSocket = k_HSteamListenSocket_Invalid;
	}

	//this.m_peerID.Clear();
}



const char* messages[] = { "None", "Connecting", "FindingRoute", "Connnected", "ClosedByPeer", "ProblemDetectedLocally",
	"FinWait", "Linger", "Dead"
};

const char* strState(ESteamNetworkingConnectionState state) {
	if (state >= 0 && state <= 5) {
		return messages[state];
	}
	return messages[5 + (-state)];
}



ISocket::Ptr AcceptorSteam::Accept() {
	if (m_awaiting.empty())
		return nullptr;
	auto&& front = std::move(m_awaiting.front());
	m_awaiting.pop_front();
	return front;
}



void AcceptorSteam::OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t *data) {

	LOG(INFO) << "Steam status changed msg " << strState(data->m_info.m_eState);
	if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connected
		&& data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
	{
		auto pair = m_connecting.find(data->m_hConn);
		if (pair != m_connecting.end()) {
			auto zsteamSocket = pair->second;
			SteamNetConnectionInfo_t steamNetConnectionInfo_t;
			if (SteamGameServerNetworkingSockets()->GetConnectionInfo(data->m_hConn, &steamNetConnectionInfo_t))
			{
				zsteamSocket->m_peerID = steamNetConnectionInfo_t.m_identityRemote;
			}

			LOG(INFO) << "Got connection, SteamID: " << zsteamSocket->m_peerID.GetSteamID64();
			m_connecting.erase(pair);
			m_awaiting.push_back(zsteamSocket);
		}
	}
	else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_Connecting 
		&& data->m_eOldState == k_ESteamNetworkingConnectionState_None)
	{
		m_connecting.insert({ data->m_hConn, std::make_shared<SteamSocket>(data->m_hConn) });
	}
	else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
	{
		LOG(INFO) << "Got problem " << data->m_info.m_eEndReason << ":" << data->m_info.m_szEndDebug;
		auto pair = m_connecting.find(data->m_hConn);
		if (pair != m_connecting.end()) {
			LOG(INFO) << "Closing socket";
			pair->second->Close();
			m_connecting.erase(pair);
		}
	}
	else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
	{
		auto pair = m_connecting.find(data->m_hConn);
		if (pair != m_connecting.end()) {
			LOG(INFO) << "Closing socket";
			pair->second->Close();
			m_connecting.erase(pair);
		}
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
