#pragma once

#include <chrono>

using namespace std::chrono_literals;

//#define RUN_TESTS

#define VH_SERVER_VERSION "v1.0.4"

// ELPP log file name
#define VH_LOGFILE_PATH "logs/log.txt"

#define VALHALLA_LUA_PATH "lua"
#define VALHALLA_LUA_CPATH "bin"
#define VALHALLA_MOD_PATH "mods"



// Enable the zone subsystem
#define VH_OPTION_ENABLE_ZONES

// Enable zone subsystem generation
#define VH_OPTION_ENABLE_ZONE_GENERATION

// Enable the ZoneManager features
#define VH_OPTION_ENABLE_ZONE_FEATURES

// Enable the ZoneManager vegetation
#define VH_OPTION_ENABLE_ZONE_VEGETATION



// Enable the mod subsystem
#define VH_OPTION_ENABLE_MODS

// Enable mod simulated mod rpc events
//#define VH_OPTION_ENABLE_MOD_SIMULATED_RPC_EVENTS

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr int32_t APP_ID = 892970;

    //static const char* PLAYERNAME = "Stranger";

    // Valheim game version
    //  Located in Version.cs
    static const char* GAME = "0.215.2";

    static constexpr uint32_t NETWORK = 1;

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