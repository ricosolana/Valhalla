#pragma once

#include <chrono>

using namespace std::chrono_literals;

#define SERVER_VERSION "1.0.0-alpha"

// Interval for RPC pinging
// Should be inlined
//#define RPC_PING_INTERVAL 1s

// Enable AsyncQueue wait
//#define USE_DEQUE_WAIT

// DO NOT CHANGE THIS VALUE!
//#define VALHEIM_APP_ID 892970

// ELPP log file name
#define LOGFILE_NAME "log.txt"

//#define DUMMY_PLAYERNAME "Stranger"

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr int32_t APP_ID = 892970;

    static const char* PLAYERNAME = "Stranger";

    // Valheim game version
    static const char* GAME = "0.212.9";

    // worldgenerator
    static constexpr int32_t WORLD = 29;

    // Used in WorldGenerator terrain
    static constexpr int32_t WORLDGEN = 2;

    // Used in ZDO
    static constexpr int32_t PGW = 53;

    // Used in ZoneSystem ZoneLocation-Prefabs
    static constexpr int32_t FEATURE = 1;

    // Player version
    // Used only in client
    //static constexpr int32_t PLAYER = 37;
}