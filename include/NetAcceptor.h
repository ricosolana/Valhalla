#pragma once

#include <memory>
#include <thread>

#include "NetSocket.h"
#include "ValhallaServer.h"

class NetAcceptor {
#ifdef USE_STEAM_NETWORKING_SOCKETS

#endif

public:
    NetAcceptor();

    ~NetAcceptor();

    // Init listening and queueing any accepted connections
    //  Should be non-blocking
    void Listen();

    // Poll for a ready and newly accepted connection
    //  Should be non-blocking
    //  Nullable
    ISocket::Ptr Accept();
};
