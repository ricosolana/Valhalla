#include "NetSocket.h"
#include <isteamnetworkingsockets.h>
#include <isteamnetworkingutils.h>
#include "ValhallaServer.h"

SteamSocket::SteamSocket(HSteamNetConnection hConn)
    : m_hConn(hConn) {

	SteamNetConnectionInfo_t info;
	SteamGameServerNetworkingSockets()->GetConnectionInfo(m_hConn, &info);
	m_steamNetId = info.m_identityRemote;

    char buf[20];
    info.m_addrRemote.ToString(buf, sizeof(buf), false);
    this->m_address = std::string(buf);

    m_connected = true;
}

SteamSocket::~SteamSocket() {
    Close(true);
}



void SteamSocket::Start() {}

void SteamSocket::Close(bool flush) {
    if (!Connected())
        return;

    m_connected = false;

    auto steamID = m_steamNetId.GetSteamID();
//m_steamNetId.GetIPAddr()
    if (flush) {
        SteamGameServerNetworkingSockets()->FlushMessagesOnConnection(m_hConn);
        auto self(shared_from_this());
        Valhalla()->RunTaskLater([this, self, steamID](Task&) {
            SteamGameServerNetworkingSockets()->CloseConnection(m_hConn, 0, "", false);
            SteamGameServer()->EndAuthSession(steamID);
        }, 3s);
    } else {
        SteamGameServerNetworkingSockets()->CloseConnection(m_hConn, 0, "", false);
        SteamGameServer()->EndAuthSession(steamID);
    }
}



void SteamSocket::Update() {
    OPTICK_EVENT();
	SendQueued();
}

void SteamSocket::Send(NetPackage pkg) {
	if (pkg.m_stream.Length() == 0)
		return;

    m_sendQueue.push_back(std::move(pkg.m_stream.m_buf));
}

std::optional<NetPackage> SteamSocket::Recv() {
    OPTICK_EVENT();
	if (Connected()) {
#define MSG_COUNT 1
		SteamNetworkingMessage_t* msg; // will point to allocated messages
		if (SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(m_hConn, &msg, MSG_COUNT) >= MSG_COUNT) {
			NetPackage pkg((BYTE_t*)msg->m_pData, msg->m_cbSize);
			msg->Release();
			return pkg;
		}
	}
	return std::nullopt;
}



int SteamSocket::GetSendQueueSize() const {
	if (Connected()) {
		int num = 0;
		for (auto&& bytes : m_sendQueue) {
			num += (int)bytes.size();
		}
		SteamNetConnectionRealTimeStatus_t rt{};
		if (SteamNetworkingSockets()->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr) == k_EResultOK) {
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

        // TODO use SendMessage(); does not copy message structure buffer
        //  But memory container must not be vector
        // https://partner.steamgames.com/doc/api/ISteamNetworkingSockets#SendMessages
        //auto msg = SteamNetworkingUtils()->AllocateMessage(0);
        //msg->m_cbSize = front.size();
        //msg->m_pData = front.data();
        //msg->m_pfnFreeData = [](SteamNetworkingMessage_t* msg) {
        //    msg-
        //};
        //int64_t num = 0;
        //SteamGameServerNetworkingSockets()->SendMessages(1, msg,);

        if (SteamGameServerNetworkingSockets()->SendMessageToConnection(
                m_hConn, front.data(), front.size(), k_nSteamNetworkingSend_Reliable, nullptr) != k_EResultOK) {
			LOG(INFO) << "Failed to send data";
			return;
		}

		m_sendQueue.pop_front();
	}
}
