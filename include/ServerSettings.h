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
    seconds         worldSaveInterval;   // set to 0 to disable

    bool            playerWhitelist;
    unsigned int    playerMax;
    bool            playerAuth;
    bool            playerList;
    //bool            playerArrivePing;
    bool            playerForceVisible;

    milliseconds    socketTimeout;          // ms
    unsigned int    zdoMaxCongestion;    // congestion rate
    unsigned int    zdoMinCongestion;    // congestion rate
    milliseconds    zdoSendInterval;

    bool            spawningCreatures;
    bool            spawningLocations;
    bool            spawningVegetation;
};
