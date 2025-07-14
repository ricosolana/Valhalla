#pragma once

#include <asio.hpp>

#include <thread>

#include "NetAcceptor.h"
#include "NetSocket.h"

namespace avledet::network {

    class TCPAcceptor : public IAcceptor
    {
      public:
        asio::io_context m_ctx;

        std::jthread m_thread;
        asio::ip::tcp::acceptor m_acceptor;

        std::list<TcpSocket::Ptr> m_queued;
        std::mutex m_mux;
        std::function<void(ISocket::Ptr)> m_connect_callback;
        bool const m_is_listener;

      public:
        TCPAcceptor();
        TCPAcceptor(asio::ip::tcp::endpoint ep);
        TCPAcceptor(asio::ip::address address, asio::ip::port_type port);
        TCPAcceptor(asio::ip::port_type port);

        ~TCPAcceptor() override;

        //std::vector<char> get_auth_session_ticket() override;
        //bool verify_auth_session_ticket(std::span<const char> ticket, Socket::Ptr socket) override;

        std::string resolve(std::string hostname, std::string service);

        //std::string get_public_ip() override;

        void start() override;
        void update() override;
        void stop() override;

        void on_connect(std::function<void(ISocket::Ptr)> callback) override;

        ISocket::Ptr connect(std::string address) override;

      private:
        void do_accept();
    };

}// namespace avledet::network
