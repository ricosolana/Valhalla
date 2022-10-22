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

	float			socketTimeout;
	unsigned int	socketCongestion;


};
