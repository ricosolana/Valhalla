// main.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>

#include <steam_api.h>

int main(int argc, char **argv) {

    if (!SteamAPI_Init()) {
        std::cout << "Failed to init steamapi\n";
        return 0;
    }

    std::cout << "Successfully initialized SteamAPI!\n";

    SteamAPI_Shutdown();

	return 0;
}
