#pragma once

#include <chrono>

using namespace std::chrono_literals;

//#define RUN_TESTS

#define VALHALLA_SERVER_VERSION "v1.0.3"

// Interval for RPC pinging
// Should be inlined
//#define RPC_PING_INTERVAL 1s

// Enable AsyncQueue wait
//#define USE_DEQUE_WAIT

// DO NOT CHANGE THIS VALUE!
//#define VALHEIM_APP_ID 892970

// ELPP log file name
#define VALHALLA_LOGFILE_NAME "logs/log.txt"

// Whether to enable or disable
//  Will explicitly enable 
#define VALHALLA_GENERATE_FEATURES

// Whether 

//#define DUMMY_PLAYERNAME "Stranger"

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr int32_t APP_ID = 892970;

    //static const char* PLAYERNAME = "Stranger";

    // Valheim game version
    //  Located in Version.cs
    static const char* GAME = "0.214.2";

    // worldgenerator
    static constexpr int32_t WORLD = 29;

    // Used in WorldGenerator terrain
    static constexpr int32_t WORLDGEN = 2;

    // Used in ZDO
    static constexpr int32_t PGW = 99;

    // Used in ZoneSystem Feature-Prefabs
    static constexpr int32_t LOCATION = 26;

    // Player version
    // Used only in client
    //static constexpr int32_t PLAYER = 37;
}