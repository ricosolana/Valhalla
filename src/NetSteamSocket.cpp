#include "NetSocket.h"

SteamSocket::SteamSocket(HSteamNetConnection con) {
	m_steamNetCon = con;
	SteamNetConnectionInfo_t info;
	SteamGameServerNetworkingSockets()->GetConnectionInfo(m_steamNetCon, &info);
	m_steamNetId = info.m_identityRemote;
	LOG(INFO) << "Peer connected " << m_steamNetId.GetSteamID64();
	//ZSteamSocket.m_sockets.Add(this);
}

SteamSocket::~SteamSocket() {
	Close();
}



void SteamSocket::Start() {}

void SteamSocket::Close() {
	//Flush();
	//Thread.Sleep(100);
	auto steamID = m_steamNetId.GetSteamID();
	// Valheim does not linger the socket connection, it closes it immediately, 
	// NOT BEFORE CALLING THREAD.SLEEP ON THE MAIN THREAD!?!
	// 
	//	I will instead attempt to flush the data, instead of fucking sleeping on the main thread
	// Im certain theres a reason behind this, but wtf could it be?
	SteamGameServerNetworkingSockets()->FlushMessagesOnConnection(m_steamNetCon);
	SteamGameServerNetworkingSockets()->CloseConnection(m_steamNetCon, 0, "", true);
	SteamGameServer()->EndAuthSession(steamID);
	m_steamNetCon = k_HSteamNetConnection_Invalid;
	m_steamNetId.Clear();
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
	if (Connected()) {
		SteamNetworkingMessage_t* msg; // will point to allocated messages
		if (SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(m_steamNetCon, &msg, 1) == 1) {
			auto pkg(PKG((byte_t*)msg->m_pData, msg->m_cbSize));
			msg->Release();
			return pkg;
		}
	}
	return nullptr;
}



void SteamSocket::SendQueued() {
	if (!Connected())
		return;

	while (!m_sendQueue.empty()) {
		auto&& front = m_sendQueue.front();

		int64_t num = 0;
		// could try SendMessage(); does not copy data buffer
		// https://partner.steamgames.com/doc/api/ISteamNetworkingSockets#SendMessages
		auto eresult = SteamGameServerNetworkingSockets()->SendMessageToConnection(
			m_steamNetCon, front.data(), front.size(), k_nSteamNetworkingSend_Reliable, &num);
		if (eresult != k_EResultOK) {
			LOG(INFO) << "Failed to send data, ec: " << eresult;
			return;
		}

		m_sendQueue.pop_front();
	}
}
