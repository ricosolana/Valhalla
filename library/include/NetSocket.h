#pragma once

#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <string>

#include <isteamfriends.h>
#include <steamnetworkingtypes.h>

#include "isteamnetworkingsockets.h"
#include "Types.h"
#include "VUtils.h"

namespace avledet::network {

    enum class Status
    {
        //Fresh,
        Connecting,
        Connected,
        Lingering,
        Closed,
        Connect_Failed,
    };

    class ISocket : public std::enable_shared_from_this<ISocket>
    {
      public:
        using Ptr = std::shared_ptr<ISocket>;

        virtual ~ISocket() = default;

        virtual void Close(bool linger) = 0;

        virtual std::vector<char> Recv()         = 0;
        virtual void send(std::vector<char> buf) = 0;

        virtual std::string get_host_name() = 0;
        virtual std::string get_address()   = 0;
        virtual bool is_outbound()          = 0;// TODO impl

        virtual Status get_status()       = 0;
        virtual int get_ping()            = 0;
        virtual int get_send_queue_size() = 0;
        //virtual std::tuple<float, float, int, float, float> get_connection_stats() = 0;

        // auto [local, remote] = get_connection_quality()
        virtual std::tuple<float, float> get_connection_quality() = 0;
    };

    class SteamSocket : public ISocket
    {
        friend class AcceptorSteam;

      private:
        void send_queued();

        void init_identifiers();

      protected:
        static bool is_game_server()
        {
            auto game_server = SteamGameServerNetworkingSockets();
            return game_server != nullptr;
        }

        static ISteamNetworkingSockets *get_steam_sockets()
        {
            auto game_server = SteamGameServerNetworkingSockets();
            return game_server ? game_server : SteamNetworkingSockets();
        }

      public:
        using Ptr = std::shared_ptr<SteamSocket>;

        explicit SteamSocket(HSteamNetConnection hConn, bool is_outbound);
        ~SteamSocket() override;

        void flush();
        bool authenticate(avledet::util::ByteView ticket);

        void Close(bool linger) override;

        void send(std::vector<char> bytes) override;
        std::vector<char> Recv() override;

        std::string get_host_name() override;
        std::string get_address() override;
        bool is_outbound() override;

        Status get_status() override;
        int get_ping() override;
        int get_send_queue_size() override;
        //std::tuple<float, float, int, float, float> get_connection_stats() override;
        std::tuple<float, float> get_connection_quality() override;

      private:
        SteamNetworkingIdentity m_steam_id {};
        std::list<std::vector<char>> m_send_queue;
        std::string m_address;
        HSteamNetConnection m_conn {};
        Status m_status {};
        bool const m_is_outbound;
    };

}// namespace avledet::network

using Status      = avledet::network::Status;
using ISocket     = avledet::network::ISocket;
using SteamSocket = avledet::network::SteamSocket;