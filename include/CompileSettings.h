#pragma once

#include <chrono>

using namespace std::chrono_literals;

#define SERVER_VERSION "1.0.0-alpha"

// Enable RPC method name debugging
//#define RPC_DEBUG

// Interval for RPC pinging
#define RPC_PING_INTERVAL 1s

// Enable AsyncQueue wait
//#define USE_DEQUE_WAIT

// Change on updates
#define VALHEIM_VERSION "0.211.11"

// DO NOT CHANGE THIS VALUE!
#define VALHEIM_APP_ID 892970