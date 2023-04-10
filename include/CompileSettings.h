#pragma once

#include <chrono>

using namespace std::chrono_literals;

//#define RUN_TESTS

#define VALHALLA_SERVER_VERSION "v1.0.4"

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
//  not exactly implemented
#define VALHALLA_GENERATE_FEATURES

#define VALHALLA_LUA_PATH "lua"
#define VALHALLA_LUA_CPATH "bin"
#define VALHALLA_MOD_PATH "mods"

// Whether to evoke callbacks for RPC/route/event handlers invoked from within Lua 
//  this is named terribly
//#define MOD_EVENT_RESPONSE

#define VALHALLA_WORLD_RECORDING_PATH "captures"

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr int32_t APP_ID = 892970;

    //static const char* PLAYERNAME = "Stranger";

    // Valheim game version
    //  Located in Version.cs
    static const char* GAME = "0.214.300";

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