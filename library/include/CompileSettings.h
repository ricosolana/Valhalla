#pragma once

#include <cstdint>

#define AVLEDET_VERSION "v1.0.1"

// ELPP log file name
//  TODO no longer used, because now using quill
#define AVLEDET_LOGFILE_PATH "logs/log.txt"

#define AVLEDET_LUA_PATH     "lua"
#define AVLEDET_LUA_CPATH    "bin"
#define AVLEDET_SCRIPTS_PATH "scripts"

#define AVL_RAW_IS_ON(OP_SYMBOL)          ((3 OP_SYMBOL 3) != 0)
#define AVL_RAW_IS_OFF(OP_SYMBOL)         ((3 OP_SYMBOL 3) == 0)
#define AVL_RAW_IS_DEFAULT_ON(OP_SYMBOL)  ((3 OP_SYMBOL 3) > 3)
#define AVL_RAW_IS_DEFAULT_OFF(OP_SYMBOL) ((3 OP_SYMBOL 3 OP_SYMBOL 3) < 0)

#define AVL_IS_ON(OP_SYMBOL)          AVL_RAW_IS_ON(OP_SYMBOL##_I_)
#define AVL_IS_OFF(OP_SYMBOL)         AVL_RAW_IS_OFF(OP_SYMBOL##_I_)
#define AVL_IS_DEFAULT_ON(OP_SYMBOL)  AVL_RAW_IS_DEFAULT_ON(OP_SYMBOL##_I_)
#define AVL_IS_DEFAULT_OFF(OP_SYMBOL) AVL_RAW_IS_DEFAULT_OFF(OP_SYMBOL##_I_)

#define AVL_ON          |
#define AVL_OFF         ^
#define AVL_DEFAULT_ON  +
#define AVL_DEFAULT_OFF -

#if SIZE_MAX <= 0xFFFFULL
    #define AVL_PLATFORM_16BIT_I_ AVL_ON
    #define AVL_PLATFORM_32BIT_I_ AVL_OFF
    #define AVL_PLATFORM_64BIT_I_ AVL_OFF
#elif SIZE_MAX <= 0xFFFFFFFFULL
    #define AVL_PLATFORM_16BIT_I_ AVL_OFF
    #define AVL_PLATFORM_32BIT_I_ AVL_ON
    #define AVL_PLATFORM_64BIT_I_ AVL_OFF
#else
    #define AVL_PLATFORM_16BIT_I_ AVL_OFF
    #define AVL_PLATFORM_32BIT_I_ AVL_OFF
    #define AVL_PLATFORM_64BIT_I_ AVL_ON
#endif

#if defined(AVL_PLATFORM_WINDOWS)
    #if AVL_PLATFORM_WINDOWS != 0
        #define AVL_PLATFORM_WINDOWS_I_ AVL_ON
    #else
        #define AVL_PLATFORM_WINDOWS_I_ AVL_OFF
    #endif
#elif defined(_WIN32)
    #define AVL_PLATFORM_WINDOWS_I_ AVL_DEFAULT_ON
#else
    #define AVL_PLATFORM_WINDOWS_I_ AVL_DEFAULT_OFF
#endif

// Ensure that the bits
#if defined(AVL_USERID_BITS)
    #if AVL_USERID_BITS >= 6 && AVL_USERID_BITS <= 12
        #define AVL_USERID_BITS_I_ AVL_USERID_BITS
    #else
        #error "User bits must be between 6 and 12 for optimal performance"
    #endif
#else
    #define AVL_USERID_BITS_I_ 12
#endif

// Bit allocation for ZDOIDs
//  ZDOIDs are 32bit, so good use of the bits must occur
//  During world load, as of version <?>, all ZDOIDs are trimmed away, and ZDOs are initialized with UserID=0, and an incrementing ID
//  If the world is especially large (1M+ ZDOs...), ID pool exhaustion could occur during world load
//  ---
//  Requirements:
//      UserID must remain stable (number must be intact for certain ZDOs which use it as a direct User/owner ref)
//  Impl:
//      On world load, simply load ZDOIDs as normal
//      When ID exhaustion occurs for a given UserID,
//      Create a new UserID entry in [UserID pool], at back, but equal to UserID
//  To perform [UserID / ID] lookup:
//      If significant-most bit in pack is 1: then assume pool exhaustion has occured for this node, and thus the different approach must be used
//      Otherwise, search for UserID[zdoid.UserIDIndex]
//
//  A good relationship of
// The default are the remaining bits from AVL_USER_BITS
#if defined(AVL_ID_BITS)
    #error "Setting custom ZDOID ID bits not yet supported"

// 8 is the current max (might change stuff to be more adaptable and flexible with compile settings)
//#if AVL_ID_BITS > 1 && AVL_ID_BITS < 8
//    #define AVL_ID_BITS_I_ AVL_ID_BITS
//#else
//    #error "ID bits must be between 1 and 8 (inclusive)"
//#endif
#else
    #if defined(AVL_USERID_BITS)
        #define AVL_ID_BITS_I_
    #else
        #define AVL_ID_BITS_I_ (32 - AVL_USERID_BITS_I_)
    #endif
#endif

/*
// Whether to include only core Valheim functionality
// Included:
//  - World loading/saving
//  - Zone generation
//  - Dungeon generation
//  - Random events
//  - Core features
// Excluded:
//  - Mods
//  - Extra features
//  - Dungeon regeneration
//  - Discord integration
//  - Player packet capture
#if defined(AVL_TOTALLY_VANILLA)
    #if AVL_TOTALLY_VANILLA != 0
        #define AVL_TOTALLY_VANILLA_I_ AVL_ON
    #else
        #define AVL_TOTALLY_VANILLA_I_ AVL_OFF
    #endif
#else
    #define AVL_TOTALLY_VANILLA_I_ AVL_DEFAULT_OFF
#endif*/


// Whether to disallow potentially malicious modded players
//  'Malicious' is defined as non-conforming or non-standard
//  behaviour which can and will break server functionality,
//  such as errors, undefined behaviour, and general
//  strict assumptions about the game that require these standards
//  to be met
// TODO rename UNEXPECTED_PACKET_METADATA...
#if defined(AVL_DISALLOW_MALICIOUS_PLAYERS)
    #if AVL_DISALLOW_MALICIOUS_PLAYERS != 0
        #define AVL_DISALLOW_MALICIOUS_PLAYERS_I_ AVL_ON
    #else
        #define AVL_DISALLOW_MALICIOUS_PLAYERS_I_ AVL_OFF
    #endif
#else
    #define AVL_DISALLOW_MALICIOUS_PLAYERS_I_ AVL_DEFAULT_ON
#endif

// whether to allow non-standard behaviour that does not cause
//  conflict with the server or other players
//  might be an extended name
#if defined(AVL_DISALLOW_NON_CONFORMING_PLAYERS)
    #if AVL_DISALLOW_NON_CONFORMING_PLAYERS != 0
        #define AVL_DISALLOW_NON_CONFORMING_PLAYERS_I_ AVL_ON
    #else
        #define AVL_DISALLOW_NON_CONFORMING_PLAYERS_I_ AVL_OFF
    #endif
#else
    #define AVL_DISALLOW_NON_CONFORMING_PLAYERS_I_ AVL_DEFAULT_OFF
#endif

/*
// Controls whether to scan player sent data for
//  obscure patterns of data and/or impossibilities
//  that would normally affect gameplay of server performance
// Minor overhead but also experimental if enabled
#if defined(AVL_SANITIZE_EXTERNAL_DATA)

#else

#endif

// Same as AVL_SANITIZE_EXTERNAL_DATA but for methods that are 
//  inherently internal and not likely to be used by an external
#if defined(AVL_SANITIZE_INTERNAL_DATA)

#else

#endif
*/

#if defined(AVL_ENABLE_SCRIPTING)
    #if AVL_ENABLE_SCRIPTING != 0
        #define AVL_ENABLE_SCRIPTING_I_ AVL_ON
    #else
        #define AVL_ENABLE_SCRIPTING_I_ AVL_OFF
    #endif
#else
    #define AVL_ENABLE_SCRIPTING_I_ AVL_DEFAULT_ON
#endif

// Whether events/rpcs/... called by Lua will self trigger other callbacks
//  Disabled by default
//  Enabling will lower performance slightly depending on mods
//  This name isn't the best
#if defined(AVL_REFLECTIVE_MOD_EVENTS)
    #if AVL_REFLECTIVE_MOD_EVENTS != 0
        #if AVL_IS_OFF(AVL_ENABLE_SCRIPTING)
            #error "Mod visible events must have mods enabled"
        #else
            #define AVL_REFLECTIVE_MOD_EVENTS_I_ AVL_ON
        #endif
    #else
        #define AVL_REFLECTIVE_MOD_EVENTS_I_ AVL_OFF
    #endif
#else
    #define AVL_REFLECTIVE_MOD_EVENTS_I_ AVL_DEFAULT_OFF
#endif

//#define AVL_ZONE_GENERATION 0

#if defined(AVL_ZONE_GENERATION)
    #if AVL_ZONE_GENERATION != 0
        #define AVL_ZONE_GENERATION_I_ AVL_ON
    #else
        #define AVL_ZONE_GENERATION_I_ AVL_OFF
    #endif
#else
    #define AVL_ZONE_GENERATION_I_ AVL_DEFAULT_ON
#endif

#if defined(AVL_RANDOM_EVENTS)
    #if AVL_RANDOM_EVENTS != 0
        #define AVL_RANDOM_EVENTS_I_ AVL_ON
    #else
        #define AVL_RANDOM_EVENTS_I_ AVL_OFF
    #endif
#else
    #define AVL_RANDOM_EVENTS_I_ AVL_DEFAULT_ON
#endif

#if defined(AVL_DUNGEON_GENERATION)
    #if AVL_DUNGEON_GENERATION != 0
        #define AVL_DUNGEON_GENERATION_I_ AVL_ON
    #else
        #define AVL_DUNGEON_GENERATION_I_ AVL_OFF
    #endif
#else
    #define AVL_DUNGEON_GENERATION_I_ AVL_DEFAULT_ON
#endif

#if defined(AVL_DISCORD_INTEGRATION)
    #if AVL_DISCORD_INTEGRATION != 0
        #define AVL_DISCORD_INTEGRATION_I_ AVL_ON
    #else
        #define AVL_DISCORD_INTEGRATION_I_ AVL_OFF
    #endif
#else
    #define AVL_DISCORD_INTEGRATION_I_ AVL_DEFAULT_ON
#endif

#define AVL_CORE_FEATURES 1

// Whether to enable:
//  - portal linking
//  - sleeping/time skip
// Disabled by default if mods are enabled
#if defined(AVL_CORE_FEATURES)
    #if AVL_CORE_FEATURES != 0
        #define AVL_CORE_FEATURES_I_ AVL_ON
    #else
        #define AVL_CORE_FEATURES_I_ AVL_OFF
    #endif
#else
    #if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
        #define AVL_CORE_FEATURES_I_ AVL_DEFAULT_OFF
    #else
        #define AVL_CORE_FEATURES_I_ AVL_DEFAULT_ON
    #endif
#endif

#if defined(AVL_PORTAL_LINKING)
    #if AVL_PORTAL_LINKING != 0
        #define AVL_PORTAL_LINKING_I_ AVL_ON
    #else
        #define AVL_PORTAL_LINKING_I_ AVL_OFF
    #endif
#else
    #if AVL_IS_ON(AVL_CORE_FEATURES)
        #define AVL_PORTAL_LINKING_I_ AVL_DEFAULT_ON
    #else
        #define AVL_PORTAL_LINKING_I_ AVL_DEFAULT_OFF
    #endif
#endif

#if defined(AVL_PLAYER_SLEEP)
    #if AVL_PLAYER_SLEEP != 0
        #define AVL_PLAYER_SLEEP_I_ AVL_ON
    #else
        #define AVL_PLAYER_SLEEP_I_ AVL_OFF
    #endif
#else
    #if AVL_IS_ON(AVL_CORE_FEATURES)
        #define AVL_PLAYER_SLEEP_I_ AVL_DEFAULT_ON
    #else
        #define AVL_PLAYER_SLEEP_I_ AVL_DEFAULT_OFF
    #endif
#endif

// Whether to include extra useful builtins
//  - Built-in BetterNetworking mod (in C++)
//  - Dungeon regeneration
//  - experimental ZDO assignment algorithm
//  -
#if defined(AVL_EXTRA_FEATURES)
    #if AVL_EXTRA_FEATURES != 0
        #define AVL_EXTRA_FEATURES_I_ AVL_ON
    #else
        #define AVL_EXTRA_FEATURES_I_ AVL_OFF
    #endif
#else
    #define AVL_EXTRA_FEATURES_I_ AVL_DEFAULT_OFF
#endif

// https://github.com/T3kla/ValMods/blob/master/~DungeonReset/Scripts/Extensions.cs
#if defined(AVL_DUNGEON_REGENERATION)
    #if AVL_DUNGEON_REGENERATION != 0
        #if AVL_IS_OFF(AVL_DUNGEON_GENERATION)
            #error "AVL_DUNGEON_REGENERATION requires AVL_DUNGEON_GENERATION"
        #endif

        #define AVL_DUNGEON_REGENERATION_I_ AVL_ON
    #else
        #define AVL_DUNGEON_REGENERATION_I_ AVL_OFF
    #endif
#else
    #if AVL_IS_ON(AVL_EXTRA_FEATURES)
        #define AVL_DUNGEON_REGENERATION_I_ AVL_DUNGEON_GENERATION_I_
    #else
        #define AVL_DUNGEON_REGENERATION_I_ AVL_DEFAULT_OFF
    #endif
#endif

// Whether to support loading worlds older than the latest version
//  Can be disabled to very slightly reduce executable size
#if defined(AVL_LEGACY_WORLD_LOADING)
    #if AVL_LEGACY_WORLD_LOADING != 0
        #define AVL_LEGACY_WORLD_LOADING_I_ AVL_ON
    #else
        #define AVL_LEGACY_WORLD_LOADING_I_ AVL_OFF
    #endif
#else
    #define AVL_LEGACY_WORLD_LOADING_I_ AVL_DEFAULT_ON
#endif

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr std::int32_t APP_ID = 892970;

    // Valheim game version
    //  Located in Version.cs
    static constexpr char const *const GAME
            = "0.220.5";// WARNING: do NOT change the type of this to anything besides const char*!!! see ModManager...

    static constexpr std::uint32_t NETWORK = 34;

    // Used while loading world from file (ZNet/ZoneSystem/ZDOMan/RandEventSystem)
    // 32: Hildir beta
    static constexpr std::int32_t WORLD = 35;

    // Used in WorldGenerator terrain
    static constexpr std::int32_t WORLDGEN = 2;

    // Used in ZoneSystem Feature-Prefabs
    static constexpr std::int32_t LOCATION = 26;
}// namespace VConstants