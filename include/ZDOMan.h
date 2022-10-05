#pragma once

#include <robin_hood.h>
#include <vector>
#include "ZDOID.h"
#include "ZDO.h"
#include "Vector2.h"
#include "HashUtils.h"
#include "NetPeer.h"

namespace ZDOManager {
	void OnNewPeer(NetPeer::Ptr peer);
}
