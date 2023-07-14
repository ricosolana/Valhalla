#pragma once

// Valhalla project version
#define VH_VERSION "v1.0.5"

// Valhalla log file location (relative to data path)
#define VH_LOGFILE_PATH "logs/log.txt"

// Lua paths
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

// Whether to attempt to recover corrupt worlds
//  Save files in 0.216.* had changes to how ZDOs are saved that break while loading (8-bit integer overflow occurs while saving dungeons)
//  This attempts to read those 8-bit numbers with an additional set bit (assuming a larger number) for restoration
#if defined(VH_WORLD_RECOVERY)
    #if VH_WORLD_RECOVERY != 0
        #define VH_WORLD_RECOVERY_I_ VH_ON
    #else
        #define VH_WORLD_RECOVERY_I_ VH_OFF
    #endif
#else
    #define VH_WORLD_RECOVERY_I_ VH_DEFAULT_ON
#endif

#define VH_PORTABLE_ZDOS 1

// Whether to store prefab/type/distant/persistent directly inside zdo
//  Recommended for modded worlds and/or maintaining general compatibility when +4bits is not a concern
#if defined(VH_PORTABLE_ZDOS)
    #if VH_PORTABLE_ZDOS != 0
        #define VH_PORTABLE_ZDOS_I_ VH_ON
    #else
        #define VH_PORTABLE_ZDOS_I_ VH_OFF
    #endif
#else
    #define VH_PORTABLE_ZDOS_I_ VH_DEFAULT_OFF
#endif

#if defined(VH_USE_ZLIB)
    #if VH_USE_ZLIB != 0
        #define VH_USE_ZLIB_I_ VH_ON
    #else
        #define VH_USE_ZLIB_I_ VH_OFF
    #endif
#else
    #define VH_USE_ZLIB_I_ VH_DEFAULT_ON
#endif

// Whether to perform zdo dict conversions for version 29 and earlier
//  Enabled by default
#if defined(VH_CONVERT_LEGACY)
    #if VH_CONVERT_LEGACY != 0
        #define VH_CONVERT_LEGACY_I_ VH_ON
    #else
        #define VH_CONVERT_LEGACY_I_ VH_OFF
    #endif
#else
    #define VH_CONVERT_LEGACY_I_ VH_DEFAULT_ON
#endif

// Whether to use a dict for querying zdos by prefab instead of iterating all zdos in world
//  Enabled by default
#if defined(VH_ZDO_FAST_QUERY_BY_PREFAB)
    #if VH_ZDO_FAST_QUERY_BY_PREFAB != 0
        #define VH_ZDO_FAST_QUERY_BY_PREFAB_I_ VH_ON
    #else
        #define VH_ZDO_FAST_QUERY_BY_PREFAB_I_ VH_OFF
    #endif
#else
    #define VH_ZDO_FAST_QUERY_BY_PREFAB_I_ VH_DEFAULT_ON
#endif

// How many bits to reserve for owner index in ZDO
//  At least 2 bits must be allocated for minimum compatibility
#if defined(VH_ZDO_OWNER_BITS)
    #if VH_ZDO_OWNER_BITS < 2
        #error "VH_ZDO_OWNER_BITS must be at least 2"
    #else
        #define VH_ZDO_OWNER_BITS_I_ VH_ZDO_OWNER_BITS
    #endif
#else
    #define VH_ZDO_OWNER_BITS_I_ 4
#endif

// How many bits to reserve for prefab index in ZDO
#if defined(VH_ZDO_PREFAB_BITS)
    #if VH_ZDO_PREFAB_BITS >= 12
        #define VH_ZDO_PREFAB_BITS_I_ VH_PREFAB_BITS
    #else
        #error "prefab bits must be great enough to support all the Valheim prefabs (at least 12 bits)"
    #endif
#else
    #define VH_ZDO_PREFAB_BITS_I_ 12
#endif

/*
#if defined(VH_DATA_REV_BITS)
#define VH_DATA_REV_BITS_I_ VH_DATA_REV_BITS
#else
#define VH_DATA_REV_BITS_I_ 21
#endif

#if defined(VH_OWNER_REV_BITS)
#define VH_OWNER_REV_BITS_I_ VH_OWNER_REV_BITS
#else
#define VH_OWNER_REV_BITS_I_ (VH_DATA_REV_BITS_I_ )
#endif*/

// data that is the same no matter what across prefabs can be omitted to avoid unnecessary redundancy
//  the only reason against this is whether normally assumed data is required... which would means portions of the game have been 
//  selectively modified to require persistent/distant/type as they appear when received over network/file
// this will be behaviour for now on, because prefabs always use this data
//  the reason is due to how valheim prefabs all start off with the same data regardless of which similar-prefab is being used
//#if defined(VH_ZDO_ASSUMPTIONS)
//    #if VH_ZDO_ASSUMPTIONS != 0
//        #define VH_ZDO_ASSUMPTIONS_I_ VH_ON
//    #else
//        #define VH_ZDO_ASSUMPTIONS_I_ VH_OFF
//    #endif
//#else
//    #define VH_ZDO_ASSUMPTIONS_I_ VH_DEFAULT_ON
//#endif

/*
// The default are the remaining bits from VH_USER_BITS
#if defined(VH_ID_BITS)
    #error "Setting custom id bits not yet supported"

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
*/

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
#if defined(VH_TOTALLY_VANILLA)
    #if VH_TOTALLY_VANILLA != 0
        #define VH_TOTALLY_VANILLA_I_ VH_ON
    #else
        #define VH_TOTALLY_VANILLA_I_ VH_OFF
    #endif
#else
    #define VH_TOTALLY_VANILLA_I_ VH_DEFAULT_OFF
#endif*/


// Whether to disallow potentially malicious modded players
//  'Malicious' is defined as non-conforming or non-standard 
//  behaviour which can and will break server functionality,
//  such as errors, undefined behaviour, and general
//  strict assumptions about the game that require these standards
//  to be met
#if defined(VH_DISALLOW_MALICIOUS_PLAYERS)
    #if VH_DISALLOW_MALICIOUS_PLAYERS != 0
        #define VH_DISALLOW_MALICIOUS_PLAYERS_I_ VH_ON
    #else
        #define VH_DISALLOW_MALICIOUS_PLAYERS_I_ VH_OFF
    #endif
#else
    #define VH_DISALLOW_MALICIOUS_PLAYERS_I_ VH_DEFAULT_ON
#endif

// whether to allow non-standard behaviour that does not cause
//  conflict with the server or other players
//  might be an extended name 
#if defined(VH_DISALLOW_NON_CONFORMING_PLAYERS)
    #if VH_DISALLOW_NON_CONFORMING_PLAYERS != 0
        #define VH_DISALLOW_NON_CONFORMING_PLAYERS_I_ VH_ON
    #else
        #define VH_DISALLOW_NON_CONFORMING_PLAYERS_I_ VH_OFF
    #endif
#else
    #define VH_DISALLOW_NON_CONFORMING_PLAYERS_I_ VH_DEFAULT_OFF
#endif

/*
// Controls whether to scan player sent data for
//  obscure patterns of data and/or impossibilities
//  that would normally affect gameplay of server performance
// Minor overhead but also experimental if enabled
#if defined(VH_SANITIZE_EXTERNAL_DATA)

#else

#endif

// Same as VH_SANITIZE_EXTERNAL_DATA but for methods that are 
//  inherently internal and not likely to be used by an external
#if defined(VH_SANITIZE_INTERNAL_DATA)

#else

#endif
*/

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

//#define VH_PREFAB_INFO 0

// TODO consider retiring this?
// Whether to use the smallest form prefabs possible
//  Each prefab generally takes up min 72 bytes
//  Each prefab minimally needs only flags to work well (not extra redundant hashes)
#if defined(VH_PREFAB_INFO)
    #if VH_PREFAB_INFO != 0
        #define VH_PREFAB_INFO_I_ VH_ON
    #else
        #define VH_PREFAB_INFO_I_ VH_OFF
    #endif
#else
    #define VH_PREFAB_INFO_I_ VH_DEFAULT_ON
#endif

//#define VH_ZONE_GENERATION 0

#if defined(VH_ZONE_GENERATION)
    #if VH_ZONE_GENERATION != 0
        #if VH_IS_OFF(VH_PREFAB_INFO)
            #error "Zone generation VH_ZONE_GENERATION requires VH_PREFAB_INFO to be on"
        #endif

        #define VH_ZONE_GENERATION_I_ VH_ON
    #else
        #define VH_ZONE_GENERATION_I_ VH_OFF
    #endif
#else
    #define VH_ZONE_GENERATION_I_ VH_PREFAB_INFO_I_
#endif

#if defined(VH_RANDOM_EVENTS)
    #if VH_RANDOM_EVENTS != 0
        #if VH_IS_OFF(VH_PREFAB_INFO)
            #error "VH_RANDOM_EVENTS requires VH_PREFAB_INFO"
        #endif

        #define VH_RANDOM_EVENTS_I_ VH_ON
    #else
        #define VH_RANDOM_EVENTS_I_ VH_OFF
    #endif
#else
    #define VH_RANDOM_EVENTS_I_ VH_PREFAB_INFO_I_
#endif

#if defined(VH_DUNGEON_GENERATION)
    #if VH_DUNGEON_GENERATION != 0
        #if VH_IS_OFF(VH_PREFAB_INFO)
            #error("VH_DUNGEON_GENERATION requires VH_PREFAB_INFO")
        #endif

        #define VH_DUNGEON_GENERATION_I_ VH_ON
    #else
        #define VH_DUNGEON_GENERATION_I_ VH_OFF
    #endif
#else
    #define VH_DUNGEON_GENERATION_I_ VH_PREFAB_INFO_I_
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

// This is the replay functionality
#if defined(VH_PLAYER_CAPTURE)
    #if VH_DISCORD_INTEGRATION != 0
        #define VH_PLAYER_CAPTURE_I_ VH_ON
    #else
        #define VH_PLAYER_CAPTURE_I_ VH_OFF
    #endif
#else
    #define VH_PLAYER_CAPTURE_I_ VH_DEFAULT_OFF
#endif

#define VH_PACKET_CAPTURE 1

// Whether to capture packets only (no replay functionality)
//  This is experimental and should not be used outside of testing
//  Only input packets will be captured and subsequently saved to disk
#if defined(VH_PACKET_CAPTURE)
    #if VH_PACKET_CAPTURE != 0
        #define VH_PACKET_CAPTURE_I_ VH_ON
    #else
        #define VH_PACKET_CAPTURE_I_ VH_OFF
    #endif
#else
    #define VH_PACKET_CAPTURE_I_ VH_DEFAULT_OFF
#endif

// Structure ideas for proxy server:
//  Steam sockets will be isolated from game loop side
//  Several special considerations for proxy:
//      - Combined Steam sockets and TCP in same build
//      - Only Steam sockets in build
//      - Only TCP sockets in build
//  Or just split Steam and TCP into 2 separate executables
//      Gonna have to experiment with cmake build structure to
//      automate everything without manually changing compile settings
//  Or just do it all manually with compile settings (simple)

//#define VH_PROXY_SERVER 1

//#define VH_PACKET_REDIRECTION 1

// Whether to enable TCP proxy support
// Whether to start Steam server
// Whether to start TCP server
//#if defined(VH_PACKET_REDIRECTION)
//    #if VH_PACKET_REDIRECTION != 0
//        #define VH_PACKET_REDIRECTION_I_ VH_ON
//    #else
//        #define VH_PACKET_REDIRECTION_I_ VH_OFF
//    #endif
//#else
//    #define VH_PACKET_REDIRECTION_I_ VH_DEFAULT_OFF
//#endif

// Whether to support packet forwarding:
//  to and from the backend game logic server
//  to and from the frontend steam server
//#if defined(VH_PACKET_REDIRECTION)
//    #if VH_PACKET_REDIRECTION != 0
//        #define VH_PACKET_REDIRECTION_I_ VH_ON
//    #else
//        #define VH_PACKET_REDIRECTION_I_ VH_OFF
//    #endif
//#else
//    #define VH_PACKET_REDIRECTION_I_ VH_DEFAULT_OFF
//#endif

#define VH_PACKET_REDIRECTION 1

// Whether to support packet forwarding:
//  im the (middleman) steam server, relaying packets
//  between clients and the logic server
#if defined(VH_PACKET_REDIRECTION)
    #if VH_PACKET_REDIRECTION != 0
        #define VH_PACKET_REDIRECTION_I_ VH_ON
    #else
        #define VH_PACKET_REDIRECTION_I_ VH_OFF
    #endif
#else
    #define VH_PACKET_REDIRECTION_I_ VH_DEFAULT_OFF
#endif

#define VH_CORE_FEATURES 1

// Whether to enable:
//  - portal linking
//  - sleeping/time skip
// Disabled by default if mods are enabled
#if defined(VH_CORE_FEATURES)
    #if VH_CORE_FEATURES != 0
        #define VH_CORE_FEATURES_I_ VH_ON
    #else
        #define VH_CORE_FEATURES_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_USE_MODS)
        #define VH_CORE_FEATURES_I_ VH_DEFAULT_OFF
    #else
        #define VH_CORE_FEATURES_I_ VH_DEFAULT_ON
    #endif
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
        #if VH_IS_OFF(VH_DUNGEON_GENERATION)
            #error "VH_DUNGEON_REGENERATION requires VH_DUNGEON_GENERATION"
        #endif

        #define VH_DUNGEON_REGENERATION_I_ VH_ON
    #else
        #define VH_DUNGEON_REGENERATION_I_ VH_OFF
    #endif
#else
    #if VH_IS_ON(VH_EXTRA_FEATURES)
        #define VH_DUNGEON_REGENERATION_I_ VH_DUNGEON_GENERATION_I_
    #else
        #define VH_DUNGEON_REGENERATION_I_ VH_DEFAULT_OFF
    #endif
#endif

// Whether to support loading worlds older than the latest version
//  Can be disabled to very slightly reduce executable size
#if defined(VH_LEGACY_WORLD_COMPATABILITY)
    #if VH_LEGACY_WORLD_COMPATABILITY != 0
        #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_ON
    #else
        #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_OFF
    #endif
#else
    #define VH_LEGACY_WORLD_COMPATABILITY_I_ VH_DEFAULT_ON
#endif

// Packet capture path
#define VH_CAPTURE_PATH "captures"

// Valheim latest versionings
//    Includes game, worldgen, zdo, zonelocation, ...
namespace VConstants {
    // Valheim Steam app id
    static constexpr int32_t APP_ID = 892970;

    // Valheim game version
    //  Located in Version.cs
    static const char* GAME = "0.216.9";

    static constexpr uint32_t NETWORK = 5;

    // worldgenerator
    //  32: 0.217.4 (Hildir beta) June 16, 2023
    //  31: 0.216.9 (Long awaited optimizations) June 12, 2023
    //  29: 0.212.7 (Mistlands) Dec 6, 2022
    //  28: ? .. 0.211.11 (Playfab fixes) Oct 28, 2022
    static constexpr int32_t WORLD = 31;

    // Used in WorldGenerator terrain
    static constexpr int32_t WORLDGEN = 2;

    // Used in ZoneSystem Feature-Prefabs
    static constexpr int32_t LOCATION = 26;
}