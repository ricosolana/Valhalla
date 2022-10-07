#pragma once

#include <chrono>

using namespace std::chrono_literals;

// Uncomment this to enable RPC method name debugging
// Must also be enabled on client!
//#define RPC_DEBUG

// Timeout for RPC pong
static constexpr std::chrono::seconds RPC_PING_TIMEOUT = 30s;

// Interval for RPC pinging
static constexpr std::chrono::seconds RPC_PING_INTERVAL = 1s;

// Enable or disable AsyncQueue wait
//#define USE_DEQUE_WAIT

// Change on updates
#define SERVER_VERSION "0.211.8"

// Possibly faster but more unsafe Stream
//#define REALLOC_STREAM