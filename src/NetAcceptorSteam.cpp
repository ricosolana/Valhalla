#include "NetAcceptor.h"

AcceptorSteam::AcceptorSteam(uint16_t port) 
	: m_port(port) {
}

AcceptorSteam::~AcceptorSteam() {

}

void AcceptorSteam::Start() {
	SteamNetworkingIPAddr steamNetworkingIPAddr; // nullify or whatever (default)
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



void OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t data) {

	LOG(INFO) << "Steam status changed msg " << strState(data.m_info.m_eState);
	if (data.m_info.m_eState == k_ESteamNetworkingConnectionState_Connected && data.m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
	{
		LOG(INFO) << "Client fully connected";

		auto socket = std::make_shared<SteamSocket>()

			ZSteamSocket zsteamSocket = ZSteamSocket.FindSocket(data.m_hConn);
		if (zsteamSocket != null)
		{
			SteamNetConnectionInfo_t steamNetConnectionInfo_t;
			if (SteamGameServerNetworkingSockets.GetConnectionInfo(data.m_hConn, out steamNetConnectionInfo_t))
			{
				zsteamSocket.m_peerID = steamNetConnectionInfo_t.m_identityRemote;
			}
			ZLog.Log("Got connection SteamID " + zsteamSocket.m_peerID.GetSteamID().ToString());
		}
	}
	if (data.m_info.m_eState == ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_Connecting && data.m_eOldState == ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_None)
	{
		ZLog.Log("New connection");
		ZSteamSocket listner = ZSteamSocket.GetListner();
		if (listner != null)
		{
			listner.OnNewConnection(data.m_hConn);
		}
	}
	if (data.m_info.m_eState == ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
	{
		ZLog.Log("Got problem " + data.m_info.m_eEndReason.ToString() + ":" + data.m_info.m_szEndDebug);
		ZSteamSocket zsteamSocket2 = ZSteamSocket.FindSocket(data.m_hConn);
		if (zsteamSocket2 != null)
		{
			ZLog.Log("  Closing socket " + zsteamSocket2.GetHostName());
			zsteamSocket2.Close();
		}
	}
	if (data.m_info.m_eState == ESteamNetworkingConnectionState.k_ESteamNetworkingConnectionState_ClosedByPeer)
	{
		ZLog.Log("Socket closed by peer " + data.ToString());
		ZSteamSocket zsteamSocket3 = ZSteamSocket.FindSocket(data.m_hConn);
		if (zsteamSocket3 != null)
		{
			ZLog.Log("  Closing socket " + zsteamSocket3.GetHostName());
			zsteamSocket3.Close();
		}
	}
}