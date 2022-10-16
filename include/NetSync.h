#pragma once

#include <robin_hood.h>
#include <type_traits>
#include "Quaternion.h"
#include "Vector.h"
#include "Utils.h"
#include "NetSyncID.h"

// each NetSync is 0.5Kb minimum (with all 7 maps)
// each NetSync is .168Kb minimum (with 1 map only)
class NetSync {
	int m_prefab = 0;
	Vector2i m_sector;
	Vector3 m_position;
	Quaternion m_rotation = Quaternion::IDENTITY;

	robin_hood::unordered_map<hash_t, float> m_floats;
	robin_hood::unordered_map<hash_t, Vector3> m_vectors;
	robin_hood::unordered_map<hash_t, Quaternion> m_quaternions;
	robin_hood::unordered_map<hash_t, int32_t> m_ints;
	robin_hood::unordered_map<hash_t, int64_t> m_longs;
	robin_hood::unordered_map<hash_t, std::string> m_strings;
	robin_hood::unordered_map<hash_t, bytes_t> m_bytes;

	void Revise() {
		m_dataRevision++;
	}

	// thinking about an [] operatoro override?
	// how to remove all the maps above
	// it takes up an extremely large amount of memory and is confusing
	// idea for assign:
	// 
	// this will only work assuming original hash names are unique
	//   across an entire NetSyncs varmap hashes
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
	//enum class ObjectType {
	//	Default,
	//	Prioritized,
	//	Solid,
	//	Terrain
	//};


	
	bool GetBool(hash_t hash, bool def = false);
	bool GetBool(const std::string& key, bool def = false);
	
	/*std::optional<const bytes_t&>*/ bytes_t* GetBytes(hash_t hash);
	/*std::optional<const bytes_t&>*/ bytes_t* GetBytes(const std::string& key);

	float GetFloat(hash_t hash, float def = 0);
	float GetFloat(const std::string& key, float def = 0);

	const NetSyncID& GetNetSyncID(const std::pair<hash_t, hash_t>& key);
	std::pair<hash_t, hash_t> ToHashPair(const std::string& key);
	const NetSyncID& GetNetSyncID(const std::string& key);

	int32_t GetInt(hash_t hash, int32_t def = 0);
	int32_t GetInt(const std::string& key, int32_t def = 0);

	int64_t GetLong(hash_t hash, int64_t def = 0);
	int64_t GetLong(const std::string& key, int64_t def = 0);

	const Quaternion& GetQuaternion(hash_t hash, const Quaternion& def);
	const Quaternion& GetQuaternion(const std::string& key, const Quaternion& def);

	const std::string& GetString(hash_t hash, const std::string& def = "");
	const std::string& GetString(const std::string& key, const std::string& def = "");

	const Vector3& GetVector3(hash_t hash, const Vector3 &def);
	const Vector3& GetVector3(const std::string& key, const Vector3& def);



	void Set(hash_t hash, bool value);
	void Set(const std::string& key, bool value);

	void Set(hash_t hash, const bytes_t& value);
	void Set(const std::string& key, const bytes_t& value);

	void Set(hash_t hash, float value);
	void Set(const std::string& key, float value);

	// Set a NetSyncID with key as hashed key pair
	void Set(const std::pair<hash_t, hash_t>& key, const NetSyncID& value);
	void Set(const std::string& key, const NetSyncID& value);

	void Set(hash_t hash, int32_t value);
	void Set(const std::string& key, int32_t value);

	void Set(hash_t hash, int64_t value);
	void Set(const std::string& key, int64_t value);

	void Set(hash_t hash, const Quaternion& value);
	void Set(const std::string& key, const Quaternion& value);

	void Set(hash_t hash, const std::string& value);
	void Set(const std::string& key, const std::string& value);

	void Set(hash_t hash, const Vector3& value);
	void Set(const std::string& key, const Vector3& value);


	// id of the client or server
	///uuid_t m_owner;

	// contains id of the client or server, and the object id
	NetSyncID m_NetSyncID;
	//bool m_persistent = false;	// set by ZNetView
	//bool m_distant = false;		// set by ZNetView
	uuid_t m_owner = 0;			// this seems to equal the local id, although it is not entirely confirmed
	int64_t m_timeCreated = 0;
	uint32_t m_ownerRevision = 0;
	uint32_t m_dataRevision = 0;
	//int32_t m_pgwVersion = 0; // 53 const mostly by ZNetView
	//ObjectType m_type = ObjectType::Default; // ultimately decided by ZNetView script object instance
	//float m_tempSortValue = 0; // only used in sending priority
	//bool m_tempHaveRevision = 0; // appears to be unused besides assignments
	int32_t m_tempRemovedAt = -1; // equal to frame counter at intervals
	int32_t m_tempCreatedAt = -1; // ^

	// earmark might refer to a unique marker/designator
};
