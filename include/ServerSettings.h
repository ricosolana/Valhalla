#pragma once

#include "VUtils.h"

struct ServerSettings {
    std::string     serverName;
    uint16_t        serverPort;
    std::string     serverPassword;
    bool            serverPublic;

    std::string     worldName;
    std::string     worldSeed;
    //HASH_t          worldSeed;
    bool            worldSave;
    seconds         worldSaveInterval;  // set to 0 to disable
    bool            worldModern;        // whether to purge old objects on load
    
    bool            playerAutoPassword;
    bool            playerWhitelist;
    unsigned int    playerMax;
    bool            playerAuth;
    bool            playerList;
    //bool            playerArrivePing;
    bool            playerForceVisible;
    bool            playerSleep;        // enable time skip when all players sleeping
    bool            playerSleepSolo;    // whether only 1 player needs to sleep to enable time skip
    
    milliseconds    socketTimeout;          // ms

    unsigned int    zdoMaxCongestion;    // congestion rate
    unsigned int    zdoMinCongestion;    // congestion rate
    milliseconds    zdoSendInterval;
    seconds         zdoAssignInterval;
    bool            zdoSmartAssign;     // experimental feature that attempts to reduce lagg

    bool            spawningCreatures;
    bool            spawningLocations;
    bool            spawningVegetation;
    bool            spawningDungeons;

    bool            dungeonEndCaps;
    bool            dungeonDoors;
    bool            dungeonFlipRooms;
    bool            dungeonZoneLimit;
    bool            dungeonRoomShrink;
    bool            dungeonReset;
    seconds         dungeonResetTime;
    //milliseconds    dungeonIncrementalResetTime;
    int             dungeonIncrementalResetCount;
    bool            dungeonRandomGeneration;
};
