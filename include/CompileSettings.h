#pragma once

#include <chrono>

using namespace std::chrono_literals;

#define SERVER_VERSION "1.0.0-alpha"

// Interval for RPC pinging
#define RPC_PING_INTERVAL 1s

// Enable AsyncQueue wait
//#define USE_DEQUE_WAIT

// DO NOT CHANGE THIS VALUE!
#define VALHEIM_APP_ID 892970

// ELPP log file name
#define LOGFILE_NAME "log.txt"



//
// Valheim version settings
//

// Change on updates
#define VALHEIM_VERSION "0.212.6"

static constexpr int32_t VALHEIM_WORLD_VERSION = 29;

static constexpr int32_t VALHEIM_WORLDGEN_VERSION = 2;

static constexpr int32_t VALHEIM_PGW_VERSION = 53;