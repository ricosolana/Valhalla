#pragma once

#include "VUtils.h"

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
	unsigned int	zdoMaxCongestion;    // congestion rate
    unsigned int	zdoMinCongestion;    // congestion rate
    milliseconds    zdoSendInterval;
};
