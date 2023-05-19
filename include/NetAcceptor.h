#pragma once

#include <memory>
#include <thread>
#include <asio.hpp>

#include "NetSocket.h"
#include "ValhallaServer.h"

class NetAcceptor {
#ifdef ESP_PLATFORM
// esp sockets...
#endif

    asio::io_context m_ctx;
    std::thread m_thread;
    asio::ip::tcp::acceptor m_acceptor;
    
    std::list<NetSocket::Ptr> m_acceptedQueue;
    std::mutex m_mux;
    
public:
    NetAcceptor();

    ~NetAcceptor();

    // Start listening for connections
    //  Call this once
    void Listen();

    // Poll for a ready and newly accepted connection
    //  Call this per frame
    //  Returns nullptr if no connection is waiting
    NetSocket::Ptr Accept();

private:
    void DoAccept();
};
