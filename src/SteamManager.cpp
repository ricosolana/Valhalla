#include <easylogging++.h>

#include "SteamManager.h"
#include "NetSocket.h"

void InitSteam(uint16_t port) {
    if (SteamAPI_RestartAppIfNecessary(892970)) {
        LOG(INFO) << "Restarting app through Steam";
        exit(0);
    }
    
    if (!SteamGameServer_Init(0, port, port+1, EServerMode::eServerModeNoAuthentication, "1.0.0.0")) {
        LOG(ERROR) << "Failed to init steam game server";
        exit(0);
    }

    SteamGameServer()->SetProduct("valheim");   // for version checking
    SteamGameServer()->SetModDir("valheim");    // game save location
    SteamGameServer()->SetDedicatedServer(true);
    SteamGameServer()->SetMaxPlayerCount(64);
    SteamGameServer()->LogOnAnonymous();        // no steam login necessary

    //SteamGameServer()->SetServerName()
    //
    ////this.UnregisterServer();
    //SteamGameServer.SetServerName(name);
    //SteamGameServer.SetMapName(name);
    //SteamGameServer.SetPasswordProtected(password);
    //SteamGameServer.SetGameTags(version);
    //if (publicServer)
    //{
    //    SteamGameServer.EnableHeartbeats(true);
    //}

    LOG(INFO) << "Server ID: " << SteamGameServer()->GetSteamID().ConvertToUint64();
    // waiting (2) seems to be ok (Valheim has this too)
    LOG(INFO) << "Authentication status: " << SteamGameServerNetworkingSockets()->InitAuthentication();
}

void UnInitSteam() {
    SteamGameServer_Shutdown();
}
