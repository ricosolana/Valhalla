#include "NetSocket.h"

SteamSocket::SteamSocket(HSteamNetConnection con) {
	m_con = con;
	SteamNetConnectionInfo_t info;
	SteamGameServerNetworkingSockets()->GetConnectionInfo(m_con, &info);
	m_peerID = info.m_identityRemote;
	LOG(INFO) << "Peer connected " << m_peerID.GetGenericString();
	//ZSteamSocket.m_sockets.Add(this);
}

void SteamSocket::Start() {}

void SteamSocket::Close() {
	//Flush();
	//Thread.Sleep(100);
	auto steamID = m_peerID.GetSteamID();
	// Valheim does not linger the socket connection, it closes it immediately, 
	// NOT BEFORE CALLING THREAD.SLEEP ON THE MAIN THREAD!?!
	// 
	//	I will instead attempt to flush the data, instead of fucking sleeping on the main thread
	// Im certain theres a reason behind this, but wtf could it be?
	SteamGameServerNetworkingSockets()->CloseConnection(m_con, 0, "", true);
	SteamGameServer()->EndAuthSession(steamID);
	//this.m_con = HSteamNetConnection.Invalid;
}

void SteamSocket::Send(NetPackage::Ptr pkg) {
	if (pkg->m_stream.Length() == 0)
		return;

	if (GetConnectivity() != Connectivity::CLOSED) {
		m_sendQueue.push_back(pkg->m_stream.Bytes());
		SendQueuedPackages();
	}
}
