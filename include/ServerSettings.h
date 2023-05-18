#pragma once

#include "VUtils.h"

struct ServerSettings {
    std::string     serverName = "Valhalla ESP32 Server";
    uint16_t        serverPort = 2456;
    std::string     serverPassword = "";
    bool            serverPublic = false;
    bool            serverDedicated = true;

    bool                        playerWhitelist = true;
    unsigned int                playerMax = 10;
    bool                        playerOnline = true;
    std::chrono::seconds        playerTimeout = 30s;
    std::chrono::milliseconds   playerListSendInterval = 2s;
    bool                        playerListForceVisible = false;

    std::string     worldName = "world";
    std::string     worldSeed = "warldo";
    std::chrono::seconds         worldSaveInterval = 30min;  // set to 0 to disable
    bool            worldModern = false;
    
    unsigned int    zdoMaxCongestion = 10240;    // congestion rate
    unsigned int    zdoMinCongestion = 2048;    // congestion rate
    std::chrono::milliseconds    zdoSendInterval = 50ms;
    std::chrono::seconds         zdoAssignInterval = 2s;
};
