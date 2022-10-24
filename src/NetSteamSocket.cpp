#include "NetSocket.h"
#include <isteamnetworkingsockets.h>
#include <isteamnetworkingutils.h>

SteamSocket::SteamSocket(HSteamNetConnection con) {
	m_handle = con;
	SteamNetConnectionInfo_t info;
	SteamGameServerNetworkingSockets()->GetConnectionInfo(m_handle, &info);
	m_steamID = info.m_identityRemote;
	LOG(INFO) << "Peer connected " << m_steamID.GetSteamID64();
	//ZSteamSocket.m_sockets.Add(this);
}

SteamSocket::~SteamSocket() {
	Close();
}



void SteamSocket::Start() {}

void SteamSocket::Close() {
	//Flush();
	//Thread.Sleep(100);
	auto steamID = m_steamID.GetSteamID();
	// Valheim does not linger the socket connection, it closes it immediately, 
	// NOT BEFORE CALLING THREAD.SLEEP ON THE MAIN THREAD!?!
	// 
	//	I will instead attempt to flush the data, instead of fucking sleeping on the main thread
	// Im certain theres a reason behind this, but wtf could it be?
	SteamGameServerNetworkingSockets()->FlushMessagesOnConnection(m_handle);

	// sleeping on the main thread aint great
	//std::this_thread::sleep_for(100ms);

	SteamGameServerNetworkingSockets()->CloseConnection(m_handle, 0, "", false);
	SteamGameServer()->EndAuthSession(steamID);
	m_handle = k_HSteamNetConnection_Invalid;
	m_steamID.Clear();
}



void SteamSocket::Update() {
	SendQueued();
	//if (this->)
}

void SteamSocket::Send(NetPackage::Ptr pkg) {
	if (pkg->m_stream.Length() == 0 || !Connected())
		return;

	m_sendQueue.push_back(pkg->m_stream.Bytes());	
}

NetPackage::Ptr SteamSocket::Recv() {
	if (Connected()) {
		SteamNetworkingMessage_t* msg; // will point to allocated messages
		if (SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(m_handle, &msg, 1) == 1) {
			auto pkg(PKG((byte_t*)msg->m_pData, msg->m_cbSize));
			msg->Release();
			return pkg;
		}
	}
	return nullptr;
}




int SteamSocket::GetSendQueueSize() const {
	if (Connected()) {
		int num = 0;
		for (auto&& bytes : m_sendQueue) {
			num += bytes.size();
		}
		SteamNetConnectionRealTimeStatus_t rt;
		if (SteamNetworkingSockets()->GetConnectionRealTimeStatus(m_handle, &rt, 0, NULL) == k_EResultOK) {
			num += rt.m_cbPendingReliable + rt.m_cbPendingUnreliable + rt.m_cbSentUnackedReliable;
		}

		return num;
	}
	return -1;
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
			m_handle, front.data(), front.size(), k_nSteamNetworkingSend_Reliable, &num);
		if (eresult != k_EResultOK) {
			LOG(INFO) << "Failed to send data, ec: " << eresult;
			return;
		}

		m_sendQueue.pop_front();
	}
}