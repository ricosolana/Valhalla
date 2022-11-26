#pragma once

#include <string>

#include "VUtils.h"

struct World {
	std::string m_name;
	std::string m_seedName;
	int32_t m_seed;
	OWNER_t m_uid;
	int32_t m_worldGenVersion;
	//bool m_menu;
	//bool m_loadError;
	//bool m_versionError;

	World(std::string name,
		std::string seedName,
		int32_t seed,
		OWNER_t uid,
		int32_t worldGenVersion
	);

};

namespace WorldManager {

	World* GetWorld();

	//std::unique_ptr<World> GetOrCreateWorldMeta(const std::string &name);

	void Init();

}
