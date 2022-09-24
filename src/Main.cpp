// main.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include <steam_api.h>
#include <isteamgameserver.h>
#include <steam_gameserver.h>

// See https://partner.steamgames.com/doc/sdk/api for documentation
int main(int argc, char **argv) {

    //SteamUtils()->SetWarningMessageHook([](int severity, const char* text) {
    //    std::cout << "severity: " << severity << ", " << text << "\n";
    //});

    if (SteamAPI_RestartAppIfNecessary(892970)) {
        std::cout << "Restarting app through Steam?\n";
        exit(0);
    }

    if (!SteamGameServer_Init(0, 2456, 2457, EServerMode::eServerModeNoAuthentication, "1.0.0.0"))
        throw std::runtime_error("Failed to init steam game server");

    SteamGameServer()->SetProduct("valheim");   // for version checking
    SteamGameServer()->SetModDir("valheim");    // game save location
    SteamGameServer()->SetDedicatedServer(true);
    SteamGameServer()->SetMaxPlayerCount(64);
    SteamGameServer()->LogOnAnonymous();        // no steam login necessary

    std::cout << "Server ID: " << SteamGameServer()->GetSteamID().ConvertToUint64() << "\n";
    std::cout << "Authentication status: " << SteamGameServerNetworkingSockets()->InitAuthentication() << "\n";

    SteamGameServer_Shutdown();

	return 0;
}
