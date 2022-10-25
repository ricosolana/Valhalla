#pragma once

#include "Utils.h"
#include "NetPeer.h"

namespace ZoneSystem {
	void OnNewPeer(NetPeer::Ptr peer);

	Vector2i GetZoneCoords(const Vector3 &point);

	void Init();

	static constexpr int ACTIVE_AREA = 1;
	static constexpr int ACTIVE_DISTANT_AREA = 1;
}
