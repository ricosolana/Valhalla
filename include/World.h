#pragma once

#include "VUtils.h"

struct World {
	std::string m_name;
	std::string m_seedName;
	int32_t m_seed;
	OWNER_t m_uid;
	int32_t m_worldGenVersion;
};
