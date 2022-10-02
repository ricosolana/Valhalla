#pragma once

#include "Utils.h"

class ZoneSystem {

	void SendGlobalKeys(uuid_t target);

public:
	void OnNewPeer(uuid_t target) {
		SendGlobalKeys(target);
		//SendLocationIcons(target);
	}
};
