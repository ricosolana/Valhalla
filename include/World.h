#pragma once

#include <string>

#include "VUtils.h"

namespace WorldManager {

	struct World {
		std::string m_name;
		std::string m_seedName;
		int32_t m_seed;
		OWNER_t m_uid;
		int32_t m_worldGenVersion;
		//bool m_menu;
		//bool m_loadError;
		//bool m_versionError;

	};

	std::unique_ptr<World> GetCreateWorld(const std::string &name);

}
