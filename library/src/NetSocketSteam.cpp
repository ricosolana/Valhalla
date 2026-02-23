#include <isteamgameserver.h>
#include <isteamnetworkingsockets.h>
#include <isteamuser.h>
//#include <magic_enum.hpp> //TODO magic
#include <magic_enum/magic_enum.hpp>
#include <quill/LogMacros.h>
#include <quill/Utility.h>
#include <steam_gameserver.h>

#include "Avledet.h"
#include "ModManager.h"
#include "NetAcceptor.h"
#include "NetSocket.h"
#include "steamclientpublic.h"

namespace avledet::network {

    SteamSocket::SteamSocket(HSteamNetConnection hConn, bool is_outbound) :
        m_conn(hConn),
        m_status(Status::Connecting),
        m_is_outbound(is_outbound)
    {
        this->init_identifiers();
    }

    SteamSocket::~SteamSocket()
    {
        this->close(true);
    }

    void SteamSocket::init_identifiers()
    {
        SteamNetConnectionInfo_t info {};
        get_steam_sockets()->GetConnectionInfo(m_conn, &info);
        this->m_steam_id = info.m_identityRemote;

        char buf[SteamNetworkingIPAddr::k_cchMaxString];
        info.m_addrRemote.ToString(buf, sizeof(buf), false);
        this->m_address = buf;

        LOG_TRACE_L1(AVL_LOGGER, "init_identifiers for {}, address {}", get_host_name(), m_address);
    }

    void SteamSocket::close(bool linger) noexcept
    {
        // logic:
        //  if we are already lingering, and we are close-now (do not linger), then override and close
        if (m_status == Status::Lingering && !linger) {
            LOG_TRACE_L1(AVL_LOGGER, "forced close while lingering for {}", get_host_name());
            m_status = Status::Closed;
            return;
        }

        switch (m_status) {
        case Status::Closed:
        case Status::Connect_Failed:
        case Status::Lingering: return;
        default: break;
        }

        if (m_status == Status::Connecting) {
            m_status = Status::Connect_Failed;
        } else {
            if (linger) {
                this->flush();

                m_status = Status::Lingering;
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

        LOG_TRACE_L1(AVL_LOGGER, "close for {}, status {}, linger {}", get_host_name(),
                     magic_enum::enum_name(m_status), linger);

        get_steam_sockets()->CloseConnection(m_conn, 0, "", linger);
    }

    void SteamSocket::flush()
    {
        this->send_queued();
        get_steam_sockets()->FlushMessagesOnConnection(m_conn);
    }

    bool SteamSocket::authenticate(avledet::util::ByteView ticket)
    {
        LOG_TRACE_L1(AVL_LOGGER, "authenticating for {}", get_host_name());

        EBeginAuthSessionResult result {};
        if (AVL_SETTINGS.serverDedicated) {
            result = SteamGameServer()->BeginAuthSession(ticket.data(), (int)ticket.size(),
                                                         m_steam_id.GetSteamID());
        } else {
            result = SteamUser()->BeginAuthSession(ticket.data(), (int)ticket.size(), m_steam_id.GetSteamID());
        }

        return result == k_EBeginAuthSessionResultOK;
    }

    void SteamSocket::send(std::vector<char> bytes) noexcept
    {
        assert(!bytes.empty());

        LOG_TRACE_L2(AVL_LOGGER, "send for {}", get_host_name());

        if (m_status != Status::Lingering)
            m_send_queue.push_back(std::move(bytes));

        this->send_queued();
    }

    std::vector<char> SteamSocket::Recv() noexcept
    {
        std::vector<char> bytes;

        if (m_status == Status::Connected || m_status == Status::Lingering) {
            static constexpr auto MSG_COUNT = 1;

            SteamNetworkingMessage_t *msg {};
            auto res = get_steam_sockets()->ReceiveMessagesOnConnection(m_conn, &msg, MSG_COUNT);
            if (res == MSG_COUNT) {
                bytes.insert(bytes.begin(), reinterpret_cast<char *>(msg->m_pData),
                             reinterpret_cast<char *>(msg->m_pData) + msg->m_cbSize);

                LOG_TRACE_L3(AVL_LOGGER, "recv for {}, | {} |", get_host_name(),
                             quill::utility::to_hex(bytes.data(), bytes.size()));

                msg->Release();
            } else if (res == -1) {
                // TODO suspicious, callback is already used,
                // why require a manual close
                LOG_TRACE_L1(AVL_LOGGER, "recv failed for {}", get_host_name());
                this->close(false);
            }
        } else {
            LOG_TRACE_L2(AVL_LOGGER, "illegal recv attempted for {}, status ", get_host_name(),
                         magic_enum::enum_name(m_status));
        }
        return bytes;
    }

    std::string SteamSocket::get_host_name() noexcept
    {
        return std::to_string(m_steam_id.GetSteamID64());
    }

    std::string SteamSocket::get_address() noexcept
    {
        return m_address;
    }

    bool SteamSocket::is_outbound() noexcept
    {
        return m_is_outbound;
    }

    int SteamSocket::get_send_queue_size() noexcept
    {
        int num = 0;
        for (auto &&bytes : m_send_queue) {// this is inefficient
            num += (int) bytes.size();
        }

        SteamNetConnectionRealTimeStatus_t rt {};
        if (get_steam_sockets()->GetConnectionRealTimeStatus(m_conn, &rt, 0, nullptr) == k_EResultOK) {
            LOG_TRACE_L3(AVL_LOGGER,
                         "get_send_queue_size for {}, queued {}, reliable {}, unreliable {}, unacked {}",
                         get_host_name(), num, rt.m_cbPendingReliable, rt.m_cbPendingUnreliable,
                         rt.m_cbSentUnackedReliable);
            num += rt.m_cbPendingReliable + rt.m_cbPendingUnreliable + rt.m_cbSentUnackedReliable;
        } else {
            LOG_TRACE_L3(AVL_LOGGER, "get_send_queue_size failed, hostname {}", get_host_name());
        }

        return num;
    }

    std::tuple<float, float> SteamSocket::get_connection_quality() noexcept
    {
        SteamNetConnectionRealTimeStatus_t rt {};
        if (get_steam_sockets()->GetConnectionRealTimeStatus(m_conn, &rt, 0, nullptr) == k_EResultOK) {
            auto &&local  = rt.m_flConnectionQualityLocal;
            auto &&remote = rt.m_flConnectionQualityRemote;
            LOG_TRACE_L2(AVL_LOGGER, "get_connection_quality for {}, local {}, remote {}", get_host_name(),
                         local, remote);
            return {local, remote};
        } else {
            LOG_TRACE_L2(AVL_LOGGER, "get_connection_quality failed, hostname {}", get_host_name());
        }
        return {};
    }

    Status SteamSocket::get_status() noexcept
    {
        return m_status;
    }

    int SteamSocket::get_ping() noexcept
    {
        SteamNetConnectionRealTimeStatus_t rt {};
        if (get_steam_sockets()->GetConnectionRealTimeStatus(m_conn, &rt, 0, nullptr) == k_EResultOK) {
            LOG_TRACE_L2(AVL_LOGGER, "get_ping, hostname {}, {}ms", get_host_name(), rt.m_nPing);
            return rt.m_nPing;
        } else {
            LOG_TRACE_L2(AVL_LOGGER, "get_ping fail, hostname {}", get_host_name());
        }
        return 0;
    }

    void SteamSocket::send_queued()
    {
        LOG_TRACE_L2(AVL_LOGGER, "sending queued, hostname ", get_host_name());

        if (m_status == Status::Connected /* || m_status == Status::Lingering*/) {
            for (auto &&itr = m_send_queue.begin(); itr != m_send_queue.end();) {
                auto &&array = *itr;
                LOG_TRACE_L3(AVL_LOGGER, "sending: | {} |",
                             quill::utility::to_hex(array.data(), array.size()));

                auto res = get_steam_sockets()->SendMessageToConnection(
                        m_conn, array.data(), (uint32_t) array.size(),
                        k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_ReliableNoNagle, nullptr);

                if (res != k_EResultOK) {
                    LOG_TRACE_L1(AVL_LOGGER, "send_queued() failed: {}", magic_enum::enum_name(res));
                    break;
                }
                // TODO should I recycle these vecs by hash type?
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

}// namespace avledet::network
