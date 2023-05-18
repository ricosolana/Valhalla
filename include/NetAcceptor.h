#pragma once

#include <memory>
#include <thread>

#include "NetSocket.h"
#include "ValhallaServer.h"

class NetAcceptor {
#ifdef ESP_PLATFORM
// esp sockets...
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
    NetSocket* Accept();
};
