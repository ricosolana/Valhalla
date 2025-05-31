#pragma once

#include <NetSocket.h>
#include <memory>
#include <functional>
#include <span>
#include <memory>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <thread>
#include <steam_gameserver.h>

#include "ValhallaServer.h"



namespace avledet::network {

    class Context {
    public:
        // Create a steam user context
        static std::unique_ptr<Context> steam_user(bool is_lobby_server);
        static std::unique_ptr<Context> steam_dedicated(std::string bind_addr);

        static std::unique_ptr<Context> tcp_user();
        static std::unique_ptr<Context> tcp_dedicated(std::string bind_addr);

    public:
        virtual ~Context() = default;

        //virtual std::vector<char> get_auth_session_ticket() = 0;
        //virtual bool verify_auth_session_ticket(std::span<const char> ticket, Socket::Ptr socket) = 0;

        virtual std::string get_public_ip() = 0;

        virtual void start() = 0;
        virtual void update() = 0;
        virtual void stop() = 0;

        virtual void on_connect(std::function<void(Socket::Ptr)> callback) = 0;

        virtual Socket::Ptr connect(std::string address) = 0;
    };

}// namespace avledet::network
