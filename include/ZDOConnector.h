#pragma once

#include <stdint.h>

class ZDOConenctor {
public:
	public enum ConnectionType : uint8_t {
		None = 0,
		Portal = 1,
		SyncTransform = 2,
		Spawned = 3,
		Target = 16
	};

public:
	ConnectionType m_type;
	HASH_t m_hash;
};
