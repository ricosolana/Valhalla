#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <string>

#include <asio.hpp>
#include <isteamfriends.h>
#include <isteamnetworkingsockets.h>
#include <steamnetworkingtypes.h>

#include "Types.h"
#include "VUtils.h"

namespace avledet::network {

    enum class Status : std::uint8_t
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

        virtual void close(bool linger) noexcept = 0;

        virtual std::vector<char> Recv() noexcept = 0;
        virtual void send(std::vector<char> buf) noexcept = 0;

        virtual std::string get_host_name() noexcept = 0;
        virtual std::string get_address() noexcept   = 0;
        virtual bool is_outbound() noexcept          = 0;// TODO impl

        virtual Status get_status() noexcept = 0;
        virtual int get_ping() noexcept      = 0;
        virtual int get_send_queue_size() noexcept = 0;
        //virtual std::tuple<float, float, int, float, float> get_connection_stats() = 0;

        // auto [local, remote] = get_connection_quality()
        virtual std::tuple<float, float> get_connection_quality() noexcept = 0;
    };

    class SteamSocket : public ISocket
    {
        friend class AcceptorSteam;

      private:
        void send_queued();

        void init_identifiers();

      protected:
        static bool is_game_server() noexcept 
        {
            auto game_server = SteamGameServerNetworkingSockets();
            return game_server != nullptr;
        }

        static ISteamNetworkingSockets *get_steam_sockets() noexcept 
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

        void close(bool linger) noexcept override;

        void send(std::vector<char> bytes) noexcept override;
        std::vector<char> Recv() noexcept override;

        std::string get_host_name() noexcept override;
        std::string get_address() noexcept override;
        bool is_outbound() noexcept override;

        Status get_status() noexcept override;
        int get_ping() noexcept override;
        int get_send_queue_size() noexcept override;
        std::tuple<float, float> get_connection_quality() noexcept override;

      private:
        SteamNetworkingIdentity m_steam_id {};
        std::list<std::vector<char>> m_send_queue;
        std::string m_address;
        HSteamNetConnection m_conn {};
        Status m_status {};
        bool const m_is_outbound;
    };

    class TcpSocket : public ISocket
    {
        friend class TCPAcceptor;

      private:
        void do_read_header();
        void do_read_body();
        void do_write_header(std::reference_wrapper<std::vector<char> const> packet);
        void do_write_body(std::reference_wrapper<std::vector<char> const> packet);

      public:
        using Ptr = std::shared_ptr<TcpSocket>;

        explicit TcpSocket(asio::ip::tcp::socket socket, asio::ip::tcp::endpoint endpoint, bool is_outbound);
        ~TcpSocket();

        void close(bool linger) noexcept override;

        std::vector<char> Recv() noexcept override;
        void send(std::vector<char> buf) noexcept override;

        std::string get_host_name() noexcept override;
        std::string get_address() noexcept override;
        bool is_outbound() noexcept override;

        Status get_status() noexcept override;
        int get_ping() noexcept override;
        int get_send_queue_size() noexcept override;

        //std::tuple<float, float, int, float, float> get_connection_stats() override {
        //	return {};
        //};

        std::tuple<float, float> get_connection_quality() noexcept override;

        void do_read();

      private:
        asio::ip::tcp::socket m_socket;           // 160 bytes
        std::vector<char> m_temp_read_bytes;      // 32 bytes
        std::list<std::vector<char>> m_recv;      // 24 bytes
        std::list<std::vector<char>> m_send;      // 24 bytes
        //std::string m_address;                    // 24? bytes; cached because asio doesnt cache the address
        asio::ip::tcp::endpoint m_endpoint;         // 16 bytes; cached
        std::shared_mutex m_mux;                  // 8 bytes
        std::uint32_t m_temp_read_size {};        // 4 bytes
        std::uint32_t m_temp_write_size {};       // 4 bytes
        std::atomic_uint32_t m_send_queue_size {};// 4 bytes
        std::atomic<Status> m_status {};          // 1 bytes
        bool const m_is_outbound;
    };

}// namespace avledet::network

using Status      = avledet::network::Status;
using ISocket     = avledet::network::ISocket;
using SteamSocket = avledet::network::SteamSocket;