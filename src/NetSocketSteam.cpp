#include <isteamgameserver.h>
#include <isteamnetworkingsockets.h>
#include <isteamuser.h>
#include <steam_gameserver.h>

#include "NetSocket.h"
#include "ValhallaServer.h"
#include "ModManager.h"
#include "NetAcceptor.h"

SteamSocket::SteamSocket(HSteamNetConnection hConn, bool is_outbound)
    : m_conn(hConn), m_status(Status::Connecting), m_is_outbound(is_outbound) {
    this->init_identifiers();
}

SteamSocket::~SteamSocket() {
    this->Close(true);
}

void SteamSocket::init_identifiers() {
    SteamNetConnectionInfo_t info{};
    get_steam_sockets()->GetConnectionInfo(m_conn, &info);
    m_steam_id = info.m_identityRemote;

    char buf[SteamNetworkingIPAddr::k_cchMaxString];
    info.m_addrRemote.ToString(buf, sizeof(buf), false);
    m_address = buf;
}

void SteamSocket::Close(bool linger) {
    switch (m_status) {
        case Status::Closed:
        case Status::Connect_Failed:
        case Status::Lingering:
            return;
        default:
            break;
    }

    if (m_status == Status::Connecting) {
        m_status = Status::Connect_Failed;
    } else {
        if (linger) {
            m_status = Status::Lingering;

            this->flush();
        } else {
            m_status = Status::Closed;
        }
    }

    // try invalidating ticket regardless of linger/close
    auto steam_id = m_steam_id.GetSteamID();
    if (this->is_game_server()) {
        SteamGameServer()->EndAuthSession(steam_id);
    } else {
        SteamUser()->EndAuthSession(steam_id);
    }

    get_steam_sockets()->CloseConnection(m_conn, 0, "", linger);
}

void SteamSocket::flush() {
    this->send_queued();
    get_steam_sockets()->FlushMessagesOnConnection(m_conn);
}

bool SteamSocket::authenticate(avledet::util::ByteView ticket) {
    return ((VH_SETTINGS.serverDedicated
            ? SteamGameServer()->BeginAuthSession(ticket.data(), ticket.size(), m_steam_id.GetSteamID())
            : SteamUser()->BeginAuthSession(ticket.data(), ticket.size(), m_steam_id.GetSteamID())) != k_EBeginAuthSessionResultOK);
}



void SteamSocket::Send(std::vector<char> bytes) {
    assert(!bytes.empty());

    m_send_queue.push_back(std::move(bytes));
    this->send_queued();
}

std::vector<char> SteamSocket::Recv() {
    std::vector<char> bytes;

    if (m_status == Status::Connected
        || m_status == Status::Lingering) {
        static constexpr auto MSG_COUNT = 1;

        SteamNetworkingMessage_t* msg{};
        auto res = get_steam_sockets()->ReceiveMessagesOnConnection(m_conn, &msg, MSG_COUNT);
        if (res == MSG_COUNT) {
            bytes.insert(bytes.begin(),
                reinterpret_cast<char*>(msg->m_pData),
                reinterpret_cast<char*>(msg->m_pData) + msg->m_cbSize);
            msg->Release();
        } else if (res == -1) {
            // TODO suspicious, callback is already used,
            // why require a manual close
            this->Close(false);
        }
    }
    return bytes;
}

std::string SteamSocket::GetHostName() {
    return std::to_string(m_steam_id.GetSteamID64());
}

std::string SteamSocket::GetAddress() {
    return m_address;
}

bool SteamSocket::is_outbound() {
    return m_is_outbound;
}

int SteamSocket::GetSendQueueSize() {
    int num = 0;
    for (auto&& bytes : m_send_queue) { // this is inefficient
        num += (int)bytes.size();
    }

    SteamNetConnectionRealTimeStatus_t rt{};
    if (get_steam_sockets()->GetConnectionRealTimeStatus(m_conn, &rt, 0, nullptr) == k_EResultOK) {
        num += rt.m_cbPendingReliable + rt.m_cbPendingUnreliable + rt.m_cbSentUnackedReliable;
    }

    return num;
}

std::tuple<float, float> SteamSocket::get_connection_quality() {
    SteamNetConnectionRealTimeStatus_t rt{};
    if (get_steam_sockets()->GetConnectionRealTimeStatus(m_conn, &rt, 0, nullptr) == k_EResultOK) {
        return { rt.m_flConnectionQualityLocal, rt.m_flConnectionQualityRemote };
    }
    return {};
}

Status SteamSocket::get_status() {
    return m_status;
}

int SteamSocket::GetPing() {
    SteamNetConnectionRealTimeStatus_t rt{};
    if (get_steam_sockets()->GetConnectionRealTimeStatus(m_conn, &rt, 0, nullptr) == k_EResultOK) {
        return rt.m_nPing;
    }
    return 0;
}

void SteamSocket::send_queued() {
    if (m_status == Status::Connected) {
        for (auto&& itr = m_send_queue.begin(); itr != m_send_queue.end();) {
            auto&& array = *itr;
            auto res = get_steam_sockets()->SendMessageToConnection(m_conn, array.data(), (uint32_t)array.size(),
                k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_ReliableNoNagle, nullptr);
            if (res != k_EResultOK) {
                std::cout << "data send failed\n";
                //if (res == k_EResultNoConnection) {
                //    // close
                //    this->close(true);
                //}
                break;
            }
            itr = m_send_queue.erase(itr);
        }
    }

    //assert(false);

    /*
    // TODO
    // the error handling here (determining whether packets are queued / sent)
    // is practically non-existent,
    // consider reverting back to the more inefficient but safer SendMessage
    assert(false);
    std::array<SteamNetworkingMessage_t*, 10> messages{};
    auto count = std::min(m_send_queue.size(), messages.size());

    for (size_t i = 0; i < count; i++) {
        auto uniq = std::make_unique<std::vector<char>>(std::move(m_send_queue.front())); // avoids very slim chance of memory leak

        SteamNetworkingMessage_t* msg = SteamNetworkingUtils()->AllocateMessage(0);
        msg->m_conn = m_conn;              // set the intended recipient
        msg->m_pData = uniq->data();        // set the buffer
        msg->m_cbSize = (int)uniq->size();  // set the buffer size
        msg->m_nUserData = reinterpret_cast<std::intptr_t>(uniq.release());
        msg->m_nFlags = k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_ReliableNoNagle;   // set the message flags
        msg->m_pfnFreeData = [](SteamNetworkingMessage_t* msg) {
            delete reinterpret_cast<std::vector<char>*>(msg->m_nUserData);
        };
        messages[i] = msg;

        // What if send fails?
        // did we just lose data?
        m_send_queue.pop_front();
    }

    std::array<int64_t, messages.size()> state{};
    // TODO what is the behaviour when this call fails?
    // the documentation states that message ownership is transferred,
    // but does this mean that the message free() function is called?

    // A way to test this would be to flood the internal message
    // buffer, and utilize clumsy to severely limit throughput.
    // if message free() is never called, then there are leaks
    // this is unlikely (assuming ownership means everything is
    // eventually cleaned up)
    this->get_steam_sockets()->SendMessages((int)count, messages.data(), state.data());

    // regardless of the result, failed messages will result in data loss,
    // at the application level (valheim), and thus stop comms
    // how to fix

    //

    for (size_t i=0; i < count; i++) {
        auto&& status = state[i];
        if (status != EResult::k_EResultOK) {
            this->close(false);
            break;
        }
    }*/
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