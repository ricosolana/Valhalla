#pragma once

#include <stdint.h>

#include "VUtils.h"

class ZDOConnector {
public:
	enum class Type : uint8_t {
		None = 0,
		Portal = 1,
		SyncTransform = 2,
		Spawned = 3,
		Target = 16
	};

public:
	Type m_type;
	HASH_t m_hash;
};
