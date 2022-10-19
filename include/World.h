#pragma once

#include <string>

#include "Utils.h"

struct World {
	std::string m_name;
	std::string m_seedName;
	int32_t m_seed;
	UUID_t m_uid;
	int32_t m_worldGenVersion;
	bool m_menu;
	bool m_loadError;
	bool m_versionError;
};
