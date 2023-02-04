#include <isteamgameserver.h>
#include <isteamnetworkingsockets.h>

#include "NetSocket.h"
#include "ValhallaServer.h"

SteamSocket::SteamSocket(HSteamNetConnection hConn)
    : m_hConn(hConn) {

    // This constructor doesnt do anything special
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



void SteamSocket::Close(bool flush) {
    if (!Connected())
        return;

    auto steamID = m_steamNetId.GetSteamID();

    if (flush) {
        SendQueued();
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

    m_connected = false;
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
        if (SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(m_hConn, &msg, MSG_COUNT) == MSG_COUNT) {
            NetPackage pkg((BYTE_t*)msg->m_pData, msg->m_cbSize);
            msg->Release();
            return pkg;
        }
        else {
            LOG(DEBUG) << "Failed to receive message";
        }
    }
    return std::nullopt;
}



std::string SteamSocket::GetHostName() const {
    return std::to_string(m_steamNetId.GetSteamID64());
}

std::string SteamSocket::GetAddress() const {
    return m_address;
}

bool SteamSocket::Connected() const {
    return m_connected;
}

unsigned int SteamSocket::GetSendQueueSize() const {
    //if (Connected()) {
        unsigned int num = 0;
        for (auto&& bytes : m_sendQueue) { // this is inefficient
            num += (int)bytes.size();
        }
        SteamNetConnectionRealTimeStatus_t rt{};
        if (SteamGameServerNetworkingSockets()->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr) == k_EResultOK) {
            num += rt.m_cbPendingReliable + rt.m_cbPendingUnreliable + rt.m_cbSentUnackedReliable;
        }

        return num;
    //}
    //return -1;
}



void SteamSocket::SendQueued() {
    if (!Connected())
        return;

    //OPTICK_CATEGORY("SocketUpdate", Optick::Category::Network);

    while (!m_sendQueue.empty()) {
        auto&& front = m_sendQueue.front();

        // TODO use SendMessage(); does not copy message structure buffer
        //  But memory container must not be vector
        //  A vector can ONLY be used, given that ownership is handed over to steamlib, or i multithread this
        //  
        // https://partner.steamgames.com/doc/api/ISteamNetworkingSockets#SendMessages
        //SteamNetworkingMessage_t* msg = SteamNetworkingUtils()->AllocateMessage(0);
        //msg->m_conn = m_hConn;          // set the intended recipient
        //msg->m_pData = front.data();    // set the buffer
        //msg->m_cbSize = front.size();   // set the buffer size
        //msg->m_nFlags = k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_ReliableNoNagle;   // set the message flags
        //msg->m_nUserData = reinterpret_cast<std::intptr_t>(&front); // send the destruction message data
        //msg->m_pfnFreeData = [](SteamNetworkingMessage_t* msg) {
        //    msg-
        //};
        //int64_t state = 0;
        //SteamGameServerNetworkingSockets()->SendMessages(1, msg, );

        if (SteamGameServerNetworkingSockets()->SendMessageToConnection(
                m_hConn, front.data(), front.size(), k_nSteamNetworkingSend_Reliable, nullptr) != k_EResultOK) {
            LOG(DEBUG) << "Failed to send message";
            return;
        }

        m_sendQueue.pop_front();
    }
}
