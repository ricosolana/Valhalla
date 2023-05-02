#include <isteamgameserver.h>
#include <isteamnetworkingsockets.h>
#include <isteamuser.h>
#include <steam_gameserver.h>

#include "NetSocket.h"
#include "ValhallaServer.h"
#include "ModManager.h"

SteamSocket::SteamSocket(HSteamNetConnection hConn)
    : m_hConn(hConn) {

    // This constructor doesnt do anything special
    SteamNetConnectionInfo_t info;
    if (VH_SETTINGS.serverDedicated)
        SteamGameServerNetworkingSockets()->GetConnectionInfo(m_hConn, &info);
    else
        SteamNetworkingSockets()->GetConnectionInfo(m_hConn, &info);

    m_steamNetId = info.m_identityRemote;

    char buf[20];
    info.m_addrRemote.ToString(buf, sizeof(buf), false);
    this->m_address = std::string(buf);

    m_connected = true;

    VLOG(2) << "SteamSocket(), host: " << this->GetHostName() << ", address: " << this->GetAddress();

//#ifndef ELPP_DISABLE_VERBOSE_LOGS
//    SteamFriends()->RequestUserInformation(this->m_steamNetId.GetSteamID(), true);
//#endif
}

SteamSocket::~SteamSocket() {
    VLOG(2) << "~SteamSocket(), host: " << this->GetHostName() << ", address: " << this->GetAddress();
    Close(true);
}



void SteamSocket::Close(bool flush) {
    if (!Connected())
        return;

    auto steamID = m_steamNetId.GetSteamID();

    auto self(shared_from_this());
    auto&& close = [this, self, steamID]() { 
        if (VH_SETTINGS.serverDedicated) {
            SteamGameServerNetworkingSockets()->CloseConnection(m_hConn, 0, "", false);
            SteamGameServer()->EndAuthSession(steamID);
        }
        else {
            SteamNetworkingSockets()->CloseConnection(m_hConn, 0, "", false);
            SteamUser()->EndAuthSession(steamID);
        }
    };

    if (flush) {
        SendQueued();
        if (VH_SETTINGS.serverDedicated)
            SteamGameServerNetworkingSockets()->FlushMessagesOnConnection(m_hConn);
        else SteamNetworkingSockets()->FlushMessagesOnConnection(m_hConn);
                
        Valhalla()->RunTaskLater([this, self, steamID, close](Task&) { close(); }, 3s);
    } else {
        close();
    }

    m_connected = false;
}



void SteamSocket::Update() {
    SendQueued();
}

void SteamSocket::Send(BYTES_t bytes) {
    if (!bytes.empty()) {
        // SteamSocket is not implemented in modman, so pass parent class
        //if (VH_DISPATCH_MOD_EVENT(IModManager::Events::Send, static_cast<ISocket*>(this), std::ref(bytes)))
        m_sendQueue.push_back(std::move(bytes));
        SendQueued();
    }
}

std::optional<BYTES_t> SteamSocket::Recv() {
    if (Connected()) {
#define MSG_COUNT 1
        SteamNetworkingMessage_t* msg; // will point to allocated messages
        if ((VH_SETTINGS.serverDedicated 
            ? SteamGameServerNetworkingSockets()->ReceiveMessagesOnConnection(m_hConn, &msg, MSG_COUNT) 
            : SteamNetworkingSockets()->ReceiveMessagesOnConnection(m_hConn, &msg, MSG_COUNT)) == MSG_COUNT) {
            BYTES_t bytes; // ((BYTE_t*)msg->m_pData, msg->m_cbSize);
            bytes.insert(bytes.begin(),
                reinterpret_cast<BYTE_t*>(msg->m_pData), 
                reinterpret_cast<BYTE_t*>(msg->m_pData) + msg->m_cbSize);
            msg->Release();
            return bytes;
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
        if ((VH_SETTINGS.serverDedicated 
            ? SteamGameServerNetworkingSockets()->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr) 
            : SteamNetworkingSockets()->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr)) == k_EResultOK) {
            num += rt.m_cbPendingReliable + rt.m_cbPendingUnreliable + rt.m_cbSentUnackedReliable;
        }
        
        return num;
    //}
    //return -1;
}

unsigned int SteamSocket::GetPing() const {
    SteamNetConnectionRealTimeStatus_t rt{};
    if ((VH_SETTINGS.serverDedicated
        ? SteamGameServerNetworkingSockets()->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr)
        : SteamNetworkingSockets()->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr)) == k_EResultOK) {
        return rt.m_nPing;
    }
    return 0;
}



void SteamSocket::SendQueued() {
    if (!Connected())
        return;

    SteamNetworkingMessage_t* messages[10] {};
    auto count = std::min(m_sendQueue.size(), sizeof(messages) / sizeof(messages[0]));

    for (auto i = 0; i < count; i++) {
        auto&& front = m_sendQueue.front();

        SteamNetworkingMessage_t* msg = SteamNetworkingUtils()->AllocateMessage(0);
        msg->m_conn = m_hConn;          // set the intended recipient
        msg->m_pData = front.data();    // set the buffer
        msg->m_cbSize = front.size();   // set the buffer size
        msg->m_nUserData = reinterpret_cast<std::intptr_t>(new BYTES_t(std::move(front)));
        msg->m_nFlags = k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_ReliableNoNagle;   // set the message flags
        //msg->m_nUserData = reinterpret_cast<std::intptr_t>(&front); // send the destruction message data
        msg->m_pfnFreeData = [](SteamNetworkingMessage_t* msg) {
            delete reinterpret_cast<BYTES_t*>(msg->m_nUserData);
        };

        messages[i] = msg;

        m_sendQueue.pop_front();
    }

    int64_t state[sizeof(messages) / sizeof(messages[0])]{};
    SteamGameServerNetworkingSockets()->SendMessages(count, messages, state);
    
    return;
    
    while (!m_sendQueue.empty()) {
        auto&& front = m_sendQueue.front();



        // TODO use SendMessage(); does not copy message structure buffer
        //  But memory container must not be vector
        //  A vector can ONLY be used, given that ownership is handed over to steamlib, or i multithread this
        //  
        // https://partner.steamgames.com/doc/api/ISteamNetworkingSockets#SendMessages
        SteamNetworkingMessage_t* msg = SteamNetworkingUtils()->AllocateMessage(0);
        msg->m_conn = m_hConn;          // set the intended recipient
        msg->m_pData = front.data();    // set the buffer
        msg->m_cbSize = front.size();   // set the buffer size
        msg->m_nUserData = reinterpret_cast<std::intptr_t>(new BYTES_t(std::move(front)));
        msg->m_nFlags = k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_ReliableNoNagle;   // set the message flags
        //msg->m_nUserData = reinterpret_cast<std::intptr_t>(&front); // send the destruction message data
        msg->m_pfnFreeData = [](SteamNetworkingMessage_t* msg) {
            delete reinterpret_cast<BYTES_t*>(msg->m_nUserData);
        };

        int64_t state[1]{};
        SteamGameServerNetworkingSockets()->SendMessages(1, &msg, state);

        /*
        if ((VH_SETTINGS.serverDedicated
            ? SteamGameServerNetworkingSockets()->SendMessageToConnection(
                m_hConn, front.data(), front.size(), k_nSteamNetworkingSend_Reliable, nullptr) 
            : SteamNetworkingSockets()->SendMessageToConnection(
                m_hConn, front.data(), front.size(), k_nSteamNetworkingSend_Reliable, nullptr)) != k_EResultOK) {
            LOG(DEBUG) << "Failed to send message";
            return;
        }*/

        m_sendQueue.pop_front();
    }
}

//#ifndef ELPP_DISABLE_VERBOSE_LOGS
//void SteamSocket::OnPersonaStateChange(PersonaStateChange_t* params) {
//    auto persona = SteamFriends()->GetFriendPersonaName(params->m_ulSteamID);
//
//    VLOG(1) << "host: " << this->GetHostName()
//        << ", address: " << this->GetAddress()
//        << ", persona: " << persona;
//}
//#endif