#pragma once

#include <memory>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <steam_gameserver.h>
#include <thread>

#include "Avledet.h"
#include "NetSocket.h"

namespace avledet::network {
    class IAcceptor
    {
      public:
        // Create a steam user context
        static std::unique_ptr<IAcceptor> steam_user(bool is_lobby_server);
        static std::unique_ptr<IAcceptor> steam_dedicated(std::string bind_addr);

        //static std::unique_ptr<Context> tcp_user();
        //static std::unique_ptr<Context> tcp_dedicated(std::string bind_addr);

      public:
        virtual ~IAcceptor() = default;

        //virtual std::vector<char> get_auth_session_ticket() = 0;
        //virtual bool verify_auth_session_ticket(std::span<const char> ticket, Socket::Ptr socket) = 0;

        //virtual std::string get_public_ip() = 0;

        virtual void start()  = 0;
        virtual void update() = 0;
        virtual void stop()   = 0;

        virtual void on_connect(std::function<void(ISocket::Ptr)> callback) = 0;

        virtual ISocket::Ptr connect(std::string address) = 0;
    };

    class AcceptorSteam : public IAcceptor
    {
      public:
        //explicit SteamService(bool use_game_server);
        //SteamService(std::uint16_t port, bool p2p, );
        AcceptorSteam(bool is_lobby_server);
        AcceptorSteam(std::string bind_addr);
        ~AcceptorSteam() override;

        // generate a new self ticket
        std::vector<char> get_auth_session_ticket();

        //bool verify_auth_session_ticket(std::span<const char> ticket, ISocket::Ptr socket);

        //std::string get_public_ip() override;

        void start() override;
        void update() override;
        void stop() override;

        void on_connect(std::function<void(ISocket::Ptr)> callback) override;

        ISocket::Ptr connect(std::string address) override;

      private:
        // Expanded from the STEAM_CALLBACK macro
        // https://partner.steamgames.com/doc/sdk/api#callbacks
        //STEAM_CALLBACK(SteamContext, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);
        //STEAM_GAMESERVER_CALLBACK(AnonymousSteamService, OnSteamStatusChanged, SteamNetConnectionStatusChangedCallback_t);
        struct CCallbackInternal_OnSteamStatusChanged
            : private CCallbackImpl<sizeof(SteamNetConnectionStatusChangedCallback_t)>
        {
            CCallbackInternal_OnSteamStatusChanged(bool game_server)
            {
                if (game_server)
                    this->SetGameserverFlag();
                SteamAPI_RegisterCallback(this, SteamNetConnectionStatusChangedCallback_t::k_iCallback);
            }

          private:
            virtual void Run(void *pvParam)
            {
                // why this crazy deref
                // because this (outer SteamService) is unable to be referenced directly

                AcceptorSteam *pOuter = reinterpret_cast<AcceptorSteam *>(
                        reinterpret_cast<char *>(this)
                        - ((::size_t)
                           & reinterpret_cast<char volatile const &>(
                                   (((AcceptorSteam *) 0)->m_steamcallback_OnSteamStatusChanged))));
                pOuter->OnSteamStatusChanged(
                        reinterpret_cast<SteamNetConnectionStatusChangedCallback_t *>(pvParam));
            }
        };// CCallbackInternal_OnSteamStatusChanged m_steamcallback_OnSteamStatusChanged;

        //struct CCallbackInternal_OnAuthSessionTicketResponse : private CCallbackImpl< sizeof(GetAuthSessionTicketResponse_t) > {
        //    CCallbackInternal_OnAuthSessionTicketResponse(bool game_server) {
        //        if (game_server)
        //            this->SetGameserverFlag();
        //        SteamAPI_RegisterCallback(this, GetAuthSessionTicketResponse_t::k_iCallback);
        //    }
        //
        //    private: virtual void Run(void* pvParam) {
        //        SteamService* pOuter =
        //            reinterpret_cast<SteamService*>(
        //                reinterpret_cast<char*>(this) - ((::size_t) & reinterpret_cast<char const volatile&>((((SteamService*)0)->m_steamcallback_OnAuthSessionTicketResponse))));
        //
        //        pOuter->OnAuthSessionTicketResponse(reinterpret_cast<GetAuthSessionTicketResponse_t*>(pvParam));
        //    }
        //} m_steamcallback_OnAuthSessionTicketResponse;


        void OnSteamStatusChanged(SteamNetConnectionStatusChangedCallback_t *pParam);
        void OnAuthSessionTicketResponse(GetAuthSessionTicketResponse_t *pParam);

        void OnLobbyCreated(LobbyCreated_t *pCallback, bool failure);
        CCallResult<AcceptorSteam, LobbyCreated_t> m_lobbyCreatedCallResult;

        // cancel self issued ticket
        void cancel_auth_session_ticket();

        //bool is_game_server();
        //ISteamNetworkingSockets* get_steam_sockets();

        static auto get_socket(std::list<SteamSocket::Ptr> &list, HSteamNetConnection hConn)
        {
            for (auto &&itr = list.begin(); itr != list.end(); itr++) {
                if ((*itr)->m_conn == hConn) {
                    return itr;
                }
            }
            return list.end();
        }

      private:
        std::list<SteamSocket::Ptr> m_sockets;
        std::list<SteamSocket::Ptr> m_ready;

        //std::jthread m_thread;
        //std::mutex m_mux;
        CCallbackInternal_OnSteamStatusChanged m_steamcallback_OnSteamStatusChanged;
        CSteamID m_lobbyID {};
        std::function<void(ISocket::Ptr)> m_connect_callback;
        HSteamListenSocket m_listen_socket {};
        //std::uint16_t m_port{};
        //std::string m_bind_addr;
        //const bool m_is_p2p{};
        SteamNetworkingIPAddr m_addr;
        HAuthTicket m_ticket = k_HAuthTicketInvalid;


        // status logs
        //STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersConnected, SteamServersConnected_t);
        //STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServersDisconnected, SteamServersDisconnected_t);
        //STEAM_GAMESERVER_CALLBACK(AcceptorSteam, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
    };
}// namespace avledet::network

using IAcceptor     = avledet::network::IAcceptor;
using AcceptorSteam = avledet::network::AcceptorSteam;
