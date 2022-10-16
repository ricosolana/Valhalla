#pragma once

#include <robin_hood.h>
#include <vector>
#include "NetSyncID.h"
#include "NetSync.h"
#include "Vector.h"
#include "HashUtils.h"
#include "NetPeer.h"

namespace NetSyncManager {
	void OnNewPeer(NetPeer::Ptr peer);
}
