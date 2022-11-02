#pragma once

#include "Utils.h"

struct ServerSettings {

	std::string		serverName;
	uint16_t		serverPort;
	std::string		serverPassword;
	bool			serverPublic;

	std::string		worldName;
	std::string		worldSeedName;
	HASH_t			worldSeed;

	bool			playerWhitelist;
	unsigned int	playerMax;
	bool			playerAuth;
	bool			playerList;
	bool			playerArrivePing;

	milliseconds	socketTimeout;          // ms
	unsigned int	socketMaxCongestion;    // congestion rate
    unsigned int	socketMinCongestion;    // congestion rate

    bool            rconEnabled;
    uint16_t        rconPort;
    std::string     rconPassword;
    //uint32_t      rconDelay;              // delay in seconds between failed logins
    std::vector<std::string> rconKeys;      // not part of protocol, just extra security measure
};
