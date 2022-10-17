#pragma once

#include <robin_hood.h>
#include <type_traits>
#include "Quaternion.h"
#include "Vector.h"
#include "Utils.h"

enum TypePrefix {
	//FLOAT = 'F',
	//VECTOR = 'V',
	//QUAT = 'Q',
	//INT = 'I',
	//LONG = 'L',
	//STR = 'S',
	//ARR = 'A',
	FLOAT = 0b10,
	VECTOR = 0b101,
	QUAT = 0b1011,
	INT = 0b10110,
	LONG = 0b101101,
	STR = 0b1011011,
	ARR = 0b10110111
};

hash_t to_prefix(hash_t hash, TypePrefix pref);
hash_t from_prefix(hash_t hash, TypePrefix pref);

// each NetSync is 0.5Kb minimum (with all 7 maps)
// each NetSync is .168Kb minimum (with 1 map only)
class NetSync {
	int m_prefab = 0;
	Vector2i m_sector;
	Vector3 m_position;
	Quaternion m_rotation = Quaternion::IDENTITY;


	// new version will consist of fewer maps reduce memory
	// and will contain prefixes for hashes or smth



	//static constexpr char PREFIX_FLOAT = 'F';
	//static constexpr char PREFIX_VECS = 'V';
	//static constexpr char PREFIX_QUATS = 'Q';
	//static constexpr char PREFIX_INTS = 'I';
	//static constexpr char PREFIX_LONGS = 'L';
	//static constexpr char PREFIX_STRINGS = 'S';
	//static constexpr char PREFIX_BYTES = 'B';

	// design criteria
	// require way to convert members to vanilla structure on demand
	// 64bit hash -> 32 bit hash
	// member retrieval (rather the easiest)

	// bytes[0] is the char, the remaining is the data
	robin_hood::unordered_map<int64_t, std::pair<TypePrefix, void*>> m_members;

	//robin_hood::unordered_map<hash_t, float> m_floats;
	//robin_hood::unordered_map<hash_t, Vector3> m_vectors;
	//robin_hood::unordered_map<hash_t, Quaternion> m_quaternions;
	//robin_hood::unordered_map<hash_t, int32_t> m_ints;
	//robin_hood::unordered_map<hash_t, int64_t> m_longs;
	//robin_hood::unordered_map<hash_t, std::string> m_strings;
	//robin_hood::unordered_map<hash_t, bytes_t> m_bytes;

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

	struct ID {
		explicit ID();
		explicit ID(int64_t userID, uint32_t id);

		std::string ToString();

		bool operator==(const ID& other) const;
		bool operator!=(const ID& other) const;

		// Return whether this has a value besides NONE
		explicit operator bool() const noexcept;

		static const ID NONE;

		uuid_t m_userID;
		uint32_t m_id;
		hash_t m_hash;
	};


	//enum class ObjectType {
	//	Default,
	//	Prioritized,
	//	Solid,
	//	Terrain
	//};

	// return the object, or nullptr if not found
	template<typename T>
	requires std::same_as<T, float>
		|| std::same_as<T, int32_t>
		|| std::same_as<T, int64_t>
		|| std::same_as<T, Quaternion>
		|| std::same_as<T, Vector3>
		|| std::same_as<T, std::string>
		|| std::same_as<T, bytes_t>
	const T* Get(hash_t key, TypePrefix prefix) {
		key = to_prefix(key, prefix);
		auto&& find = m_members.find(key);
		if (find != m_members.end() 
			&& prefix == find->second.first) {
			return (T*)find->second.second;
		}
		return nullptr;
	}




	// basic params
	float GetFloat(hash_t key, float value = 0);
	int32_t GetInt(hash_t key, int32_t value = 0);
	int64_t GetLong(hash_t key, int64_t value = 0);
	const Quaternion& GetQuaternion(hash_t key, const Quaternion& value = Quaternion::IDENTITY);
	const Vector3& GetVector3(hash_t key, const Vector3& value);
	const std::string& GetString(hash_t key, const std::string& value = "");
	const bytes_t* GetBytes(hash_t key);

	// special overloads
	bool GetBool(hash_t key, bool value = false);
	const ID& GetID(std::pair<hash_t, hash_t> key, const ID& value);

	float GetFloat(const std::string& key, float value = 0);
	int32_t GetInt(const std::string& key, int32_t value = 0);
	int64_t GetLong(const std::string& key, int64_t value = 0);
	const Quaternion& GetQuaternion(const std::string& key, const Quaternion &value = Quaternion::IDENTITY);
	const Vector3& GetVector3(const std::string& key, const Vector3& value);
	const std::string& GetString(const std::string& key, const std::string& value = "");
	const bytes_t* GetBytes(const std::string& key);

	bool GetBool(const std::string &key, bool value = false);
	const ID& GetID(std::pair<hash_t, hash_t> key, const ID& value);



	// basic params str->hash
	template<typename T>
	requires std::same_as<T, float>
		|| std::same_as<T, int32_t>
		|| std::same_as<T, int64_t>
		|| std::same_as<T, Quaternion>
		|| std::same_as<T, Vector3>
		|| std::same_as<T, std::string>
		|| std::same_as<T, bytes_t>
	T Get(const std::string & key, const T & value) { return Get(Utils::GetStableHashCode(key), value); }

	// special overloads str->hash
	void Set(const std::string& key, bool value) { return Set(Utils::GetStableHashCode(key), value); }
	void Set(const std::string& key, const ID& value) { return Set(ToHashPair(key), value); }


	
	bool GetBool(hash_t hash, bool def = false) const;
	bool GetBool(const std::string& key, bool def = false) const;
	
	const bytes_t* GetBytes(hash_t hash) const;
	const bytes_t* GetBytes(const std::string& key) const;

	float GetFloat(hash_t hash, float def = 0) const;
	float GetFloat(const std::string& key, float def = 0) const;

	ID GetNetSyncID(const std::pair<hash_t, hash_t>& key) const;
	ID GetNetSyncID(const std::string& key) const;

	int32_t GetInt(hash_t hash, int32_t def = 0) const;
	int32_t GetInt(const std::string& key, int32_t def = 0) const;

	int64_t GetLong(hash_t hash, int64_t def = 0) const;
	int64_t GetLong(const std::string& key, int64_t def = 0) const;

	const Quaternion& GetQuaternion(hash_t hash, const Quaternion& def) const;
	const Quaternion& GetQuaternion(const std::string& key, const Quaternion& def) const;

	const std::string& GetString(hash_t hash, const std::string& def = "") const;
	const std::string& GetString(const std::string& key, const std::string& def = "") const;

	const Vector3& GetVector3(hash_t hash, const Vector3 &def) const;
	const Vector3& GetVector3(const std::string& key, const Vector3& def) const;






	template<typename T>
	requires std::same_as<T, float>
		|| std::same_as<T, int32_t>
		|| std::same_as<T, int64_t>
		|| std::same_as<T, Quaternion>
		|| std::same_as<T, Vector3>
		|| std::same_as<T, std::string>
		|| std::same_as<T, bytes_t>
	void Set(hash_t key, const T &value, TypePrefix prefix) {
		key = to_prefix(key, prefix);
		auto&& find = m_members.find(key);
		if (find != m_members.end()) {
			// test check if the var is the correct type
			if (prefix != find->second.first)
				return;

			// reassign if changed
			auto&& v = (T*)find->second.second;
			if (*v == value)
				return;
			*v = value;
		}
		else {
			m_members.insert({ key, new T(value) });
		}

		Revise();
	}

	// basic params
	void Set(hash_t key, float value);
	void Set(hash_t key, int32_t value);
	void Set(hash_t key, int64_t value);
	void Set(hash_t key, const Quaternion &value);
	void Set(hash_t key, const Vector3 &value);
	void Set(hash_t key, const std::string &value);
	void Set(hash_t key, const bytes_t &value);

	// special overloads
	void Set(hash_t key, bool value);
	void Set(std::pair<hash_t, hash_t> key, const ID& value);




	// basic params str->hash
	template<typename T>
	requires std::same_as<T, float>
		|| std::same_as<T, int32_t>
		|| std::same_as<T, int64_t>
		|| std::same_as<T, Quaternion>
		|| std::same_as<T, Vector3>
		|| std::same_as<T, std::string>
		|| std::same_as<T, bytes_t>
	void Set(const std::string &key, const T &value) { return Set(Utils::GetStableHashCode(key), value); }

	// special overloads str->hash
	void Set(const std::string& key, bool value) { return Set(Utils::GetStableHashCode(key), value); }
	void Set(const std::string& key, const ID& value) { return Set(ToHashPair(key), value); }



	//void Set(const std::string& key, float value) { return Set(Utils::GetStableHashCode(key), value); }
	//void Set(const std::string& key, int32_t value) { return Set(Utils::GetStableHashCode(key), value); }
	//void Set(const std::string& key, int64_t value) { return Set(Utils::GetStableHashCode(key), value); }
	//void Set(const std::string& key, const Quaternion& value) { return Set(Utils::GetStableHashCode(key), value); }
	//void Set(const std::string& key, const Vector3& value) { return Set(Utils::GetStableHashCode(key), value); }


	// complicated overloads
	// zdoid, string, array, bool, 



	// id of the client or server
	///uuid_t m_owner;

	// contains id of the client or server, and the object id
	ID m_id;
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

	// Return whether the ZDO instance is self hosted or remotely hosted
	bool Local();

	// Whether an owner has been assigned to this ZDO
	// I really dislike the null owner operability structure of Valheim server
	// Why would owner ever be 0? What circumstances ever allow this?
	bool HasOwner() {
		return m_owner;
	}

	// Claim ownership over this ZDO
	void SetLocal();

	// set the owner of the ZDO
	void SetOwner(uuid_t owner) {
		if (m_owner != owner) {
			m_owner = owner;
			m_ownerRevision++;
		}
	}

	bool Valid() {
		if (m_id)
			return true;
		return false;
	}

	static std::pair<hash_t, hash_t> ToHashPair(const std::string& key);

	// earmark might refer to a unique marker/designator
};
