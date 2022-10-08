#pragma once

#include <robin_hood.h>
#include <type_traits>
#include "Quaternion.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Utils.h"
#include "ZDOID.h"

// each ZDO is 0.5Kb minimum (with all 7 maps)
// each ZDO is .168Kb minimum (with 1 map only)
class ZDO {
	int m_prefab = 0;
	Vector2i m_sector;
	Vector3 m_position;
	Quaternion m_rotation = Quaternion::IDENTITY;

	robin_hood::unordered_map<hash_t, float> m_floats;
	robin_hood::unordered_map<hash_t, Vector3> m_vec3;
	robin_hood::unordered_map<hash_t, Quaternion> m_quats;
	robin_hood::unordered_map<hash_t, int> m_ints;
	robin_hood::unordered_map<hash_t, long> m_longs;
	robin_hood::unordered_map<hash_t, std::string> m_strings;
	robin_hood::unordered_map<hash_t, std::vector<byte_t>> m_byteArrays;

	// thinking about an [] operatoro override?
	// how to remove all the maps above
	// it takes up an extremely large amount of memory and is confusing
	// idea for assign:
	// 
	// this will only work assuming original hash names are unique
	//   across an entire zdos varmap hashes
	// this probably isnt a problem because using shared names will
	//   be confusing anyways in a technical standpoint

	// map of hash and raw members
	// members are structured as raw data in the form on the type assembled
	// from th3 package read
	// in other words, the value is in void*
	// strings will not work as intended because of the non-contiguous ptr
	// involved in construction
	// 
	// 594 cooking station is proof of this system not gonna work
	// two keys of same name is utilized

	// 
	// robin_hood::unordered_map<hash_t, std::vector<byte_t>> m_vars;

public:
	enum class ObjectType {
		Default,
		Prioritized,
		Solid,
		Terrain
	};


	
	bool GetBool(hash_t hash, bool def = false);
	bool GetBool(const std::string& key, bool def = false);
	
	const bytes_t* GetBytes(hash_t hash);
	const bytes_t* GetBytes(const std::string& key);

	float GetFloat(hash_t hash, float def = 0);
	float GetFloat(const std::string& key, float def = 0);

	const ZDOID& GetZDOID(std::pair<hash_t, hash_t>& key);
	std::pair<hash_t, hash_t> GetHashZDOID(const std::string& key);
	const ZDOID& GetZDOID(const std::string& key);

	int32_t GetInt(hash_t hash, int32_t def = 0);
	int32_t GetInt(const std::string& key, int32_t def = 0);

	int64_t GetLong(hash_t hash, int64_t def = 0);
	int64_t GetLong(const std::string& key, int64_t def = 0);

	const Quaternion& GetQuaternion(hash_t hash);
	const Quaternion& GetQuaternion(const std::string& key);

	const std::string& GetString(hash_t hash, const std::string& def = "");
	const std::string& GetString(const std::string& key, const std::string& def = "");

	const Vector3& GetVector3(hash_t hash, const Vector3 &def);
	const Vector3& GetVector3(const std::string& key, const Vector3& def);



	uuid_t m_owner;
	ZDOID m_zdoid;
	bool m_persistent = false;
	bool m_distant = false;
	//int64_t m_owner = 0; // is equal to server uuid as the server owns it
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
