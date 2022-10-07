#pragma once

#include "Utils.h"
#include "NetPeer.h"
#include "NetPackage.h"

namespace ZoneSystem {
	void OnNewPeer(NetPeer::Ptr peer);

	Vector2i GetZoneCoords(const Vector3 &point);

	void Init();
}
