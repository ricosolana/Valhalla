#include <easylogging++.h>

#include <steam_api.h>
#include <steam_gameserver.h>

#include "SteamManager.h"

void InitSteam() {
    if (SteamAPI_RestartAppIfNecessary(892970)) {
        LOG(INFO) << "Restarting app through Steam";
        exit(0);
    }

    if (!SteamGameServer_Init(0, 2456, 2457, EServerMode::eServerModeNoAuthentication, "1.0.0.0")) {
        LOG(ERROR) << "Failed to init steam game server";
        exit(0);
    }

    SteamGameServer()->SetProduct("valheim");   // for version checking
    SteamGameServer()->SetModDir("valheim");    // game save location
    SteamGameServer()->SetDedicatedServer(true);
    SteamGameServer()->SetMaxPlayerCount(64);
    SteamGameServer()->LogOnAnonymous();        // no steam login necessary

    LOG(INFO) << "Server ID: " << SteamGameServer()->GetSteamID().ConvertToUint64();
    LOG(INFO) << "Authentication status: " << SteamGameServerNetworkingSockets()->InitAuthentication();

    float timeout = 30000.0f;
    int32 offline = 1;
    int32 sendrate = 153600;

    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_TimeoutConnected,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Float, &timeout);
    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_IP_AllowWithoutAuth,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Int32, &offline);
    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_SendRateMin,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Int32, &sendrate);
    SteamNetworkingUtils()->SetConfigValue(k_ESteamNetworkingConfig_SendRateMax,
        k_ESteamNetworkingConfig_Global, 0,
        k_ESteamNetworkingConfig_Int32, &sendrate);
}

void UnInitSteam() {
    SteamGameServer_Shutdown();
}
