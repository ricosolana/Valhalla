#pragma once

#include <chrono>

using namespace std::chrono_literals;

// Enable RPC method name debugging
//#define RPC_DEBUG

// Interval for RPC pinging
#define RPC_PING_INTERVAL 1s

// Enable AsyncQueue wait
//#define USE_DEQUE_WAIT

// Change on updates
#define VALHEIM_VERSION "0.211.11"

// Experimental Stream setting (untested)
//#define REALLOC_STREAM

// DO NOT CHANGE THIS VALUE!
#define VALHEIM_APP_ID 892970