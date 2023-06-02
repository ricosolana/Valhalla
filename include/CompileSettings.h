#pragma once

//#include <chrono>

//using namespace std::chrono_literals;

//#define RUN_TESTS

#define VH_VERSION "v1.0.5"

// ELPP log file name
#define VH_LOGFILE_PATH "logs/log.txt"

#define VH_LUA_PATH "lua"
#define VH_LUA_CPATH "bin"
#define VH_MOD_PATH "mods"

#define VH_RAW_IS_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) != 0)
#define VH_RAW_IS_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3) == 0)
#define VH_RAW_IS_DEFAULT_ON(OP_SYMBOL) ((3 OP_SYMBOL 3) > 3)
#define VH_RAW_IS_DEFAULT_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3 OP_SYMBOL 3) < 0)

#define VH_IS_ON(OP_SYMBOL) VH_RAW_IS_ON(OP_SYMBOL ## _I_)
#define VH_IS_OFF(OP_SYMBOL) VH_RAW_IS_OFF(OP_SYMBOL ## _I_)
#define VH_IS_DEFAULT_ON(OP_SYMBOL) VH_RAW_IS_DEFAULT_ON(OP_SYMBOL ## _I_)
#define VH_IS_DEFAULT_OFF(OP_SYMBOL) VH_RAW_IS_DEFAULT_OFF(OP_SYMBOL ## _I_)

#define VH_ON          |
#define VH_OFF         ^
#define VH_DEFAULT_ON  +
#define VH_DEFAULT_OFF -

#if SIZE_MAX <= 0xFFFFULL
	#define VH_PLATFORM_16BIT_I_ VH_ON
	#define VH_PLATFORM_32BIT_I_ VH_OFF
	#define VH_PLATFORM_64BIT_I_ VH_OFF
#elif SIZE_MAX <= 0xFFFFFFFFULL
    #define VH_PLATFORM_16BIT_I_ VH_OFF
    #define VH_PLATFORM_32BIT_I_ VH_ON
    #define VH_PLATFORM_64BIT_I_ VH_OFF
#else
    #define VH_PLATFORM_16BIT_I_ VH_OFF
    #define VH_PLATFORM_32BIT_I_ VH_OFF
    #define VH_PLATFORM_64BIT_I_ VH_ON
#endif

#if defined(VH_PLATFORM_WINDOWS)
    #if VH_PLATFORM_WINDOWS != 0
        #define VH_PLATFORM_WINDOWS_I_ VH_ON
    #else
        #define VH_PLATFORM_WINDOWS_I_ VH_OFF
    #endif
#elif defined(_WIN32)
    #define VH_PLATFORM_WINDOWS_I_ VH_DEFAULT_ON
#else
    #define VH_PLATFORM_WINDOWS_I_ VH_DEFAULT_OFF
#endif

#if defined(VH_PLATFORM_ESP32)
    #if VH_PLATFORM_ESP32 != 0
        #define VH_PLATFORM_ESP32_I_ VH_ON
    #else
        #define VH_PLATFORM_ESP32_I_ VH_OFF
    #endif
#elif defined(ESP_PLATFORM)
    #define VH_PLATFORM_ESP32_I_ VH_DEFAULT_ON
#else
    #define VH_PLATFORM_ESP32_I_ VH_DEFAULT_OFF
#endif

// Valhalla already ensures rather fine optimal memory usage
//  But if this server is to run on an embedded system with little ram (esp32)
//  then prioritize memory as much as possible
// this will include nuking some core functionalities included within Valhiem
//      which might affect performance and extents of gameplay
#if defined(VH_MAX_MEMORY_EFFICIENCY)
    #if VH_MAX_MEMORY_EFFICIENCY != 0
        #define VH_MAX_MEMORY_EFFICIENCY_I_ VH_ON
    #else
        #define VH_MAX_MEMORY_EFFICIENCY_I_ VH_OFF
    #endif
#elif defined(ESP_PLATFORM)
    #define VH_MAX_MEMORY_EFFICIENCY_I_ VH_DEFAULT_ON
#else
    #define VH_MAX_MEMORY_EFFICIENCY_I_ VH_DEFAULT_OFF
#endif

/*
#if defined(VH_SMALL_ZDOID)
    #if VH_SMALL_ZDOID != 0
        #define VH_SMALL_ZDOID_I_ VH_ON
    #else
        #define VH_SMALL_ZDOID_I_ VH_OFF
    #endif
#else
    #define VH_SMALL_ZDOID_I_ VH_DEFAULT_OFF

    #if defined(VH_STANDARD_ZDOID)
        #if VH_STANDARD_ZDOID != 0
            #define VH_STANDARD_ZDOID_I_ VH_ON
        #else
            #define VH_STANDARD_ZDOID_I_ VH_OFF
        #endif
    #else
      
        #define VH_STANDARD_ZDOID_I_ VH_DEFAULT_OFF

        #if defined(VH_BIG_ZDOID)
            #if VH_BIG_ZDOID != 0
                #define VH_BIG_ZDOID_I_ VH_ON
            #else
                #define VH_BIG_ZDOID_I_ VH_OFF
            #endif    
        #else
#define VH_BIG_ZDOID_I_ VH_DEFAULT
        #endif

    #endif

#endif
*/

// Ensure that the bits
#if defined(VH_USER_BITS)
    // 8 is the current max (might change stuff to be more adaptable and flexible with compile settings)
    
    // ensure esp32 is always 32 bit
    // 
    #if VH_USER_BITS > 1 && VH_USER_BITS < 8
        #define VH_USER_BITS_I_ VH_USER_BITS
    #else
        #error "User bits must be between 1 and 8 (inclusive)"
    #endif
#else
    #if VH_IS_ON(VH_PLATFORM_ESP32)
        #define VH_USER_BITS_I_ 1
    #else
        #define VH_USER_BITS_I_ 6
    #endif
#endif

// The default are the remaining bits from VH_USER_BITS
#if defined(VH_ID_BITS)
    // 8 is the current max (might change stuff to be more adaptable and flexible with compile settings)
    #if VH_ID_BITS > 1 && VH_ID_BITS < 8
        #define VH_ID_BITS_I_ VH_ID_BITS
    #else
        #error "ID bits must be between 1 and 8 (inclusive)"
    #endif
#else
    //#if defined(VH_USER_BITS)
    //    #if VH_IS_ON(VH_PLATFORM_ESP32)
    //        #define VH_ID_BITS_I_ 1
    //    #else
    //        #define VH_ID_BITS_I_ 
    //    #endif
    //#else

        #define VH_ID_BITS_I_ (32 - VH_USER_BITS_I_)
    //#endif
#endif

#if defined(VH_USE_MODS)
    #if VH_USE_MODS != 0
        #define VH_USE_MODS_I_ VH_ON
    #else
        #define VH_USE_MODS_I_ VH_OFF
    #endif
#else
    #define VH_USE_MODS_I_ VH_DEFAULT_ON
#endif

// Whether events/rpcs/... called by Lua will self trigger other callbacks
//  Disabled by default
//  Enabling will lower performance slightly depending on mods
//  This name isn't the best
#if defined(VH_REFLECTIVE_MOD_EVENTS)
    #if VH_REFLECTIVE_MOD_EVENTS != 0
        #if VH_IS_OFF(VH_USE_MODS)
            #error "Mod visible events must have mods enabled"
        #else
            #define VH_REFLECTIVE_MOD_EVENTS_I_ VH_ON
        #endif
    #else
        #define VH_REFLECTIVE_MOD_EVENTS_I_ VH_OFF
    #endif
#else
    #define VH_REFLECTIVE_MOD_EVENTS_I_ VH_DEFAULT_OFF
#endif

//#define VH_STANDARD_PREFABS 0

// Whether to use the smallest form prefabs possible
//  Each prefab generally takes up min 72 bytes
//  Each prefab minimally needs only flags to work well (not extra redundant hashes)
#if defined(VH_STANDARD_PREFABS)
    #if VH_STANDARD_PREFABS != 0
        #define VH_STANDARD_PREFABS_I_ VH_ON
    #else
        #define VH_STANDARD_PREFABS_I_ VH_OFF
    #endif
#else
    #define VH_STANDARD_PREFABS_I_ VH_DEFAULT_ON
#endif

//#define VH_ZONE_GENERATION 0

#if defined(VH_ZONE_GENERATION)
    #if VH_ZONE_GENERATION != 0
        // zone geenration requires prefabs, 
        //  so if its manually disabled, then error
        #if VH_IS_OFF(VH_STANDARD_PREFABS)
            #error "Zone generation VH_ZONE_GENERATION requires VH_STANDARD_PREFABS to be on"
        #else
            #define VH_ZONE_GENERATION_I_ VH_ON
            //#define VH_USE_HEIGHTMAPS_I_ VH_ON
            //#define VH_USE_GEOGRAPHY_I_ VH_ON
        #endif
    #else
        #define VH_ZONE_GENERATION_I_ VH_OFF
        //#define VH_USE_HEIGHTMAPS_I_ VH_OFF
        //#define VH_USE_GEOGRAPHY_I_ VH_OFF
    #endif
#else
    #define VH_ZONE_GENERATION_I_ VH_STANDARD_PREFABS_I_
#endif

#if defined(VH_RANDOM_EVENTS)
#error "Not fully implemented"
    #if VH_RANDOM_EVENTS != 0
        #define VH_RANDOM_EVENTS_I_ VH_ON
    #else
        #define VH_RANDOM_EVENTS_I_ VH_OFF
    #endif
#else
    #define VH_RANDOM_EVENTS_I_ VH_STANDARD_PREFABS_I_
#endif

#if defined(VH_DUNGEON_GENERATION)
    #if VH_IS_ON(VH_STANDARD_PREFABS)
        #if VH_DUNGEON_GENERATION != 0
            #define VH_DUNGEON_GENERATION_I_ VH_ON
        #else
            #define VH_DUNGEON_GENERATION_I_ VH_OFF
        #endif
    #else
        #error("VH_DUNGEON_GENERATION requires VH_STANDARD_PREFABS")
    #endif
#else
    #define VH_DUNGEON_GENERATION_I_ VH_STANDARD_PREFABS_I_
#endif

#if defined(VH_DISCORD_INTEGRATION)
    #if VH_DISCORD_INTEGRATION != 0
        #define VH_DISCORD_INTEGRATION_I_ VH_ON
    #else
        #define VH_DISCORD_INTEGRATION_I_ VH_OFF
    #endif
#else
    #define VH_DISCORD_INTEGRATION_I_ VH_DEFAULT_ON
#endif

#if defined(VH_PLAYER_CAPTURE)
    #if VH_DISCORD_INTEGRATION != 0
        #define VH_PLAYER_CAPTURE_I_ VH_ON
    #else
        #define VH_PLAYER_CAPTURE_I_ VH_OFF
    #endif
#else
    #define VH_PLAYER_CAPTURE_I_ VH_DEFAULT_OFF
#endif

// Whether to enable:
//  - portal linking
//  - sleeping/time skip

// Should this be enabled by default if mods are disabled?
#if defined(VH_CORE_FEATURES)
    #if VH_CORE_FEATURES != 0
        #define VH_CORE_FEATURES_I_ VH_ON
    #else
        #define VH_CORE_FEATURES_I_ VH_OFF
    #endif
#else
    #define VH_CORE_FEATURES_I_ VH_DEFAULT_ON
#endif

#if defined(VH_PORTAL_LINKING)
    #if VH_PORTAL_LINKING != 0
        #define VH_PORTAL_LINKING_I_ VH_ON
    #else
        #define VH_PORTAL_LINKING_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_CORE_FEATURES)
        #define VH_PORTAL_LINKING_I_ VH_DEFAULT_ON
    #else
        #define VH_PORTAL_LINKING_I_ VH_DEFAULT_OFF
    #endif
#endif

#if defined(VH_PLAYER_SLEEP)
    #if VH_PLAYER_SLEEP != 0
        #define VH_PLAYER_SLEEP_I_ VH_ON
    #else
        #define VH_PLAYER_SLEEP_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_CORE_FEATURES)
        #define VH_PLAYER_SLEEP_I_ VH_DEFAULT_ON
    #else
        #define VH_PLAYER_SLEEP_I_ VH_DEFAULT_OFF
    #endif
#endif

#define VH_EXTRA_FEATURES 1

// Whether to include extra useful builtins
//  - Built-in BetterNetworking mod (in C++)
//  - Dungeon regeneration
//  - experimental ZDO assignment algorithm
//  - 
#if defined(VH_EXTRA_FEATURES)
    #if VH_EXTRA_FEATURES != 0
        #define VH_EXTRA_FEATURES_I_ VH_ON
    #else
        #define VH_EXTRA_FEATURES_I_ VH_OFF
    #endif
#else
    #define VH_EXTRA_FEATURES_I_ VH_DEFAULT_OFF
#endif

// https://github.com/T3kla/ValMods/blob/master/~DungeonReset/Scripts/Extensions.cs
#if defined(VH_DUNGEON_REGENERATION)
    #if VH_DUNGEON_REGENERATION != 0
        #define VH_DUNGEON_REGENERATION_I_ VH_ON
    #else
        #define VH_DUNGEON_REGENERATION_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_EXTRA_FEATURES)
        #define VH_DUNGEON_REGENERATION_I_ VH_DEFAULT_ON
    #else
        #define VH_DUNGEON_REGENERATION_I_ VH_DEFAULT_OFF
    #endif
#endif


// Whether to support loading worlds older than the latest version
//  Can be disabled to very slightly reduce executable size
//#if defined(VH_LEGACY_WORLD_COMPATABILITY)
//    #if VH_LEGACY_WORLD_COMPATABILITY != 0
//        #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_ON
//    #else
//        #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_OFF
//    #endif
//#else
//    #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_DEFAULT_ON
//#endif

// Enable the zone subsystem
//#define VH_OPTION_ENABLE_ZONES

// Enable zone subsystem generation
//#define VH_OPTION_ENABLE_ZONE_GENERATION

// Enable the ZoneManager features
//#define VH_OPTION_ENABLE_ZONE_FEATURES

// Enable the ZoneManager vegetation
//#define VH_OPTION_ENABLE_ZONE_VEGETATION

//#define VH_ZDOID_USER_BITS 6

// Whether to use prefab objects 
//  They are low overhead but file loading is required
//  Also required for feature and vegetation generation
//#define VH_OPTION_ENABLE_PREFABS

// Enable the mod subsystem
//#define VH_OPTION_ENABLE_MODS

// Enable mod simulated mod rpc events
//#define VH_OPTION_ENABLE_MOD_SIMULATED_RPC_EVENTS

// Packet capture path
#define VH_CAPTURE_PATH "captures"

// Enable incoming packet capture
//#define VH_OPTION_ENABLE_CAPTURE

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr int32_t APP_ID = 892970;

    // Valheim game version
    //  Located in Version.cs
    static const char* GAME = "0.216.8";

    static constexpr uint32_t NETWORK = 4;

    // worldgenerator
    static constexpr int32_t WORLD = 31;

    // Used in WorldGenerator terrain
    static constexpr int32_t WORLDGEN = 2;

    // Used in ZoneSystem Feature-Prefabs
    static constexpr int32_t LOCATION = 26;
}