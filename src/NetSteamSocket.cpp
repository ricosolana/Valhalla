#include "NetSocket.h"

#ifdef ENABLE_STEAM

SteamSocket::SteamSocket(HSteamNetConnection con) {
	m_con = con;
	SteamNetConnectionInfo_t info;
	SteamGameServerNetworkingSockets()->GetConnectionInfo(m_con, &info);
	m_peerID = info.m_identityRemote;
	LOG(INFO) << "Peer connected " << m_peerID.GetGenericString();
	//ZSteamSocket.m_sockets.Add(this);
}

SteamSocket::~SteamSocket() {
	Close();
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
	m_con = k_HSteamNetConnection_Invalid;
	m_peerID.Clear();
}



void SteamSocket::Update() {
	SendQueued();
}

void SteamSocket::Send(NetPackage::Ptr pkg) {
	if (pkg->m_stream.Length() == 0 || !Connected())
		return;

	m_sendQueue.push_back(pkg->m_stream.Bytes());	
}

NetPackage::Ptr SteamSocket::Recv() {
	if (!Connected())
		return nullptr;

	SteamNetworkingMessage_t* msg;
	if (SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(m_con, &msg, 1)) {
		auto pkg(PKG((byte_t*)msg->m_pData, msg->m_cbSize));
		msg->Release();
		return pkg;
	}
	return nullptr;
}



void SteamSocket::SendQueued() {
	if (!Connected())
		return;

	while (!m_sendQueue.empty()) {
		auto&& front = m_sendQueue.front();

		int64_t num = 0;
		auto eresult = SteamGameServerNetworkingSockets()->SendMessageToConnection(m_con, front.data(), front.size(), 8, &num);
		if (eresult != k_EResultOK)
		{
			LOG(INFO) << "Failed to send data, ec: " << eresult;
			return;
		}

		m_sendQueue.pop_front();
	}
}

#endif