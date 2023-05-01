#pragma once

#include "VUtils.h"

enum class AssignAlgorithm {
    NONE,
    DYNAMIC_RADIUS
};

enum class PacketMode : int32_t {
    NORMAL,
    CAPTURE,
    PLAYBACK,
};

/*
static constexpr const char* VH_SETTING_KEY_SERVER = "server";
static constexpr const char* VH_SETTING_KEY_SERVER_NAME = "name";
static constexpr const char* VH_SETTING_KEY_SERVER_PORT = "port";
static constexpr const char* VH_SETTING_KEY_SERVER_PASSWORD = "password";
static constexpr const char* VH_SETTING_KEY_SERVER_PUBLIC = "public";
static constexpr const char* VH_SETTING_KEY_SERVER_DEDICATED = "dedicated";

static constexpr const char* VH_SETTING_KEY_DISCORD = "discord";
static constexpr const char* VH_SETTING_KEY_DISCORD_WEBHOOK = "webhook";

static constexpr const char* VH_SETTING_KEY_WORLD = "world";
static constexpr const char* VH_SETTING_KEY_WORLD_NAME = "name";
static constexpr const char* VH_SETTING_KEY_WORLD_SEED = "seed";
static constexpr const char* VH_SETTING_KEY_WORLD_PREGENERATE = "pregenerate";
static constexpr const char* VH_SETTING_KEY_WORLD_SAVE_INTERVAL = "save-interval-dS";
static constexpr const char* VH_SETTING_KEY_WORLD_MODERN = "modern";
static constexpr const char* VH_SETTING_KEY_WORLD_FEATURES = "features";
static constexpr const char* VH_SETTING_KEY_WORLD_VEGETATION = "vegetation";
static constexpr const char* VH_SETTING_KEY_WORLD_CREATURES = "creatures";

static constexpr const char* VH_SETTING_KEY_PACKET = "packet";
static constexpr const char* VH_SETTING_KEY_PACKET_MODE = "mode";
static constexpr const char* VH_SETTING_KEY_PACKET_FILE_UPPER_SIZE = "file-upper-size";
static constexpr const char* VH_SETTING_KEY_PACKET_CAPTURE_SESSION_INDEX = "capture-session-index";
static constexpr const char* VH_SETTING_KEY_PACKET_PLAYBACK_SESSION_INDEX = "playback-session-index";

static constexpr const char* VH_SETTING_KEY_PLAYER = "player";
static constexpr const char* VH_SETTING_KEY_PLAYER_WHITELIST = "whitelist";
static constexpr const char* VH_SETTING_KEY_PLAYER_MAX = "max";
static constexpr const char* VH_SETTING_KEY_PLAYER_OFFLINE = "offline";
static constexpr const char* VH_SETTING_KEY_PLAYER_TIMEOUT = "timeout-dS";
static constexpr const char* VH_SETTING_KEY_PLAYER_LIST_SEND_INTERVAL = "list-send-interval-dMS";
static constexpr const char* VH_SETTING_KEY_PLAYER_LIST_FORCE_VISIBLE = "list-force-visible";

static constexpr const char* VH_SETTING_KEY_ZDO = "zdo";
static constexpr const char* VH_SETTING_KEY_ZDO_MAX_CONGESTION = "max-congestion";
static constexpr const char* VH_SETTING_KEY_ZDO_MIN_CONGESTION = "min-congestion";
static constexpr const char* VH_SETTING_KEY_ZDO_SEND_INTERVAL = "send-interval-dMS";
static constexpr const char* VH_SETTING_KEY_ZDO_ASSIGN_INTERVAL = "assign-interval-dS";
static constexpr const char* VH_SETTING_KEY_ZDO_ASSIGN_ALGORITHM = "assign-algorithm";

//static constexpr const char* VH_SETTING_KEY_SPAWN_CREATURES = "";

static constexpr const char* VH_SETTING_KEY_DUNGEONS = "dungeons";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ENABLED = "enabled";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ENDCAPS = "endcaps";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ENDCAPS_ENABLED = "enabled";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ENDCAPS_INSETFRAC = "inset-frac";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_DOORS = "doors";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ROOMS = "rooms";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ROOMS_FLIPPED = "flipped";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ROOMS_ZONEBOUNDED = "zone-bounded";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_ROOMS_INSETSIZE = "inset-size";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_REGENERATION = "regeneration";
//static constexpr const char* VH_SETTING_KEY_DUNGEONS_REGENERATION_ENABLED = "enabled";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_REGENERATION_INTERVAL = "interval-dS";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_REGENERATION_MAXSTEP = "max-step";
static constexpr const char* VH_SETTING_KEY_DUNGEONS_SEEDED = "seeded";

static constexpr const char* VH_SETTING_KEY_EVENTS = "events";
//static constexpr const char* VH_SETTING_KEY_EVENTS_ENABLED = "enabled";
static constexpr const char* VH_SETTING_KEY_EVENTS_CHANCE = "chance";
static constexpr const char* VH_SETTING_KEY_EVENTS_INTERVAL = "interval-dS";
static constexpr const char* VH_SETTING_KEY_EVENTS_RADIUS = "radius";
static constexpr const char* VH_SETTING_KEY_EVENTS_REQUIREKEYS = "require-keys";*/

struct ServerSettings {
    std::string     serverName;
    uint16_t        serverPort;
    std::string     serverPassword;
    bool            serverPublic;
    bool            serverDedicated;

    bool            playerWhitelist;
    unsigned int    playerMax;
    bool            playerOnline;
    seconds         playerTimeout;
    milliseconds    playerListSendInterval;
    bool            playerListForceVisible;

    std::string     worldName;
    std::string     worldSeed;
    bool            worldPregenerate;
    seconds         worldSaveInterval;  // set to 0 to disable
    bool            worldModern;        // whether to purge old objects on load
    bool            worldFeatures;
    bool            worldVegetation;
    bool            worldCreatures;   
    
    unsigned int    zdoMaxCongestion;    // congestion rate
    unsigned int    zdoMinCongestion;    // congestion rate
    milliseconds    zdoSendInterval;
    seconds         zdoAssignInterval;
    AssignAlgorithm zdoAssignAlgorithm;
        
    bool            dungeonsEnabled;
    bool            dungeonsEndcapsEnabled;
    bool            dungeonsEndcapsInsetFrac;
    bool            dungeonsDoors;
    bool            dungeonsRoomsFlipped;
    bool            dungeonsRoomsZoneBounded;
    bool            dungeonsRoomsInsetSize;
    bool            dungeonsRoomsFurnishing;
    seconds         dungeonsRegenerationInterval;
    int             dungeonsRegenerationMaxSteps;
    bool            dungeonsSeeded;

    float           eventsChance;
    seconds         eventsInterval;
    float           eventsRadius;
    bool            eventsRequireKeys;

#ifdef VH_OPTION_ENABLE_CAPTURE
    PacketMode      packetMode;
    size_t          packetFileUpperSize;
    int             packetCaptureSessionIndex;
    int             packetPlaybackSessionIndex;
#endif

    std::string     discordWebhook;
    std::string     discordToken;
    int64_t         discordGuild;
    bool            discordAccountLinking;
    //bool            discordDeleteCommands;
};
