#pragma once

#include <robin_hood.h>
#include "Quaternion.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Utils.hpp"
#include "ZDOID.hpp"

class ZDOMan;

class ZDO {
	int m_prefab = 0;
	Vector2i m_sector;
	Vector3 m_position;
	Quaternion m_rotation = Quaternion::identity;

	robin_hood::unordered_map<int, float> m_floats;
	robin_hood::unordered_map<int, Vector3> m_vec3;
	robin_hood::unordered_map<int, Quaternion> m_quats;
	robin_hood::unordered_map<int, int> m_ints;
	robin_hood::unordered_map<int, long> m_longs;
	robin_hood::unordered_map<int, std::string> m_strings;
	//robin_hood::unordered_map<int, byte[]> m_byteArrays;
	robin_hood::unordered_map<int, std::vector<byte>> m_byteArrays;
	//ZDOMan m_zdoMan;
	ZDOMan* m_zdoMan = nullptr;

public:
	enum ObjectType
	{
		Default,
		Prioritized,
		Solid,
		Terrain
	};

	ZDOID m_uid;
	bool m_persistent = false;
	bool m_distant = false;
	int64_t m_owner = 0;
	int64_t m_timeCreated = 0;
	uint32_t m_ownerRevision = 0;
	uint32_t m_dataRevision = 0;
	int32_t m_pgwVersion = 0;
	ObjectType m_type = ObjectType::Default;
	float m_tempSortValue = 0;
	bool m_tempHaveRevision = 0;
	int32_t m_tempRemoveEarmark = -1;
	int32_t m_tempCreateEarmark = -1;
};
