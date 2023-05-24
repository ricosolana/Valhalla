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

public:
	ZDOConnector() : m_type(Type::None) {}
	ZDOConnector(Type type) : m_type(type) {}
};

class ZDOConnectorData : public ZDOConnector {
public:
	HASH_t m_hash{};

public:
	ZDOConnectorData() {}
	ZDOConnectorData(Type type, HASH_t hash) : ZDOConnector(type), m_hash(hash) {}
};

class ZDOConnectorTargeted : public ZDOConnector {
public:
	ZDOID m_target{};

public:
	ZDOConnectorTargeted() {}
	ZDOConnectorTargeted(Type type, ZDOID target) : ZDOConnector(type), m_target(target) {}
};
