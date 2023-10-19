#include <isteamgameserver.h>
#include <isteamnetworkingsockets.h>
#include <isteamuser.h>
#include <steam_gameserver.h>

#include "NetSocket.h"
#include "ValhallaServer.h"
#include "ModManager.h"
#include "NetAcceptor.h"

//ISteamNetworkingSockets* SteamSocket::STEAM_NETWORKING_SOCKETS = nullptr;

SteamSocket::SteamSocket(HSteamNetConnection hConn)
    : m_hConn(hConn) {

    // Set basic socket info
    SteamNetConnectionInfo_t info;
    AcceptorSteam::STEAM_NETWORKING_SOCKETS->GetConnectionInfo(m_hConn, &info);

    m_steamNetId = info.m_identityRemote;

    char buf[20];
    info.m_addrRemote.ToString(buf, sizeof(buf), false);
    this->m_address = std::string(buf);

    m_connected = true;

    //VLOG(2) << "SteamSocket(), host: " << this->GetHostName() << ", address: " << this->GetAddress();

//#ifndef ELPP_DISABLE_VERBOSE_LOGS
//    SteamFriends()->RequestUserInformation(this->m_steamNetId.GetSteamID(), true);
//#endif
}

SteamSocket::~SteamSocket() {
    //VLOG(2) << "~SteamSocket(), host: " << this->GetHostName() << ", address: " << this->GetAddress();
    Close(true);
}



void SteamSocket::Close(bool flush) {
    if (!Connected())
        return;

    auto steamID = m_steamNetId.GetSteamID();



    auto self(shared_from_this());
    auto&& close = [this, self, steamID]() {
        AcceptorSteam::STEAM_NETWORKING_SOCKETS->CloseConnection(m_hConn, 0, "", false);
        if (VH_SETTINGS.serverDedicated) {
            SteamGameServer()->EndAuthSession(steamID);
        }
        else {
            SteamUser()->EndAuthSession(steamID);
        }

        //m_hConn = 0;
    };

    if (flush) {
        SendQueued();

        AcceptorSteam::STEAM_NETWORKING_SOCKETS->FlushMessagesOnConnection(m_hConn);
                
        Valhalla()->RunTaskLater([this, self, close](Task&) { close(); }, 3s);
    } else {
        close();
    }

    m_connected = false;
}



void SteamSocket::on_update() {
    SendQueued();
}

void SteamSocket::Send(BYTES_t bytes) {
    assert(!bytes.empty());

    m_sendQueue.push_back(std::move(bytes));
}

std::optional<BYTES_t> SteamSocket::Recv() {
    if (Connected()) {
#define MSG_COUNT 1
        SteamNetworkingMessage_t* msg; // will point to allocated messages
        if (AcceptorSteam::STEAM_NETWORKING_SOCKETS->ReceiveMessagesOnConnection(m_hConn, &msg, MSG_COUNT) == MSG_COUNT) {
            BYTES_t bytes; // ((BYTE_t*)msg->m_pData, msg->m_cbSize);
            bytes.insert(bytes.begin(),
                reinterpret_cast<BYTE_t*>(msg->m_pData), 
                reinterpret_cast<BYTE_t*>(msg->m_pData) + msg->m_cbSize);
            msg->Release();
            return bytes;
        }
        else {
            //LOG(DEBUG) << "Failed to receive message";
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
    //return m_hConn;
    return this->m_connected;
}



unsigned int SteamSocket::GetSendQueueSize() const {
    //if (Connected()) {
        unsigned int num = 0;
        for (auto&& bytes : m_sendQueue) { // this is inefficient
            num += (int)bytes.size();
        }
        SteamNetConnectionRealTimeStatus_t rt{};
        if (AcceptorSteam::STEAM_NETWORKING_SOCKETS->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr) == k_EResultOK) {
            num += rt.m_cbPendingReliable + rt.m_cbPendingUnreliable + rt.m_cbSentUnackedReliable;
        }
        
        return num;
    //}
    //return -1;
}

unsigned int SteamSocket::GetPing() const {
    SteamNetConnectionRealTimeStatus_t rt{};
    if (AcceptorSteam::STEAM_NETWORKING_SOCKETS->GetConnectionRealTimeStatus(m_hConn, &rt, 0, nullptr) == k_EResultOK) {
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
        msg->m_pfnFreeData = [](SteamNetworkingMessage_t* msg) {
            delete reinterpret_cast<BYTES_t*>(msg->m_nUserData);
        };

        messages[i] = msg;

        m_sendQueue.pop_front();
    }

    int64_t state[sizeof(messages) / sizeof(messages[0])]{};
    AcceptorSteam::STEAM_NETWORKING_SOCKETS->SendMessages(count, messages, state);
}

//#ifndef ELPP_DISABLE_VERBOSE_LOGS
//void SteamSocket::OnPersonaStateChange(PersonaStateChange_t* params) {
//    auto persona = SteamFriends()->GetFriendPersonaName(params->m_ulSteamID);
//
//    //VLOG(1) << "host: " << this->GetHostName()
//        << ", address: " << this->GetAddress()
//        << ", persona: " << persona;
//}
//#endif