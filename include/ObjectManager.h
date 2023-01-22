#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "WorldGenerator.h"

class IObjectManager {
public:
	struct PrefabTemplate {

		//Heightmap::Biome 

		// Will require all serialized Unity members, such as:
		//	ZNetView

		//HASH_t m_hash; // might be able to be omitted

		Vector3 m_pos;
		Quaternion m_rot;
		Vector3 m_localScale; // used by m_syncInitialScale; if scale is not {1, 1, 1}, then assume m_syncInitialScale is true

		// Could use a bitmask for these if more members are added later for memory saving
		bool m_persistent;
		ZDO::ObjectType m_type;
		bool m_distant;


	};

private:
	robin_hood::unordered_map<HASH_t, std::unique_ptr<PrefabTemplate>> m_prefabs;

private:
	void Init();



public:
	ZDO* Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot);

};
