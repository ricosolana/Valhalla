#pragma once

#include <robin_hood.h>
#include <type_traits>

#include "Quaternion.h"
#include "Vector.h"
#include "Utils.h"
#include "NetPackage.h"

template<typename T>
concept TrivialSyncType = std::same_as<T, float>
	|| std::same_as<T, int32_t>
	|| std::same_as<T, int64_t>
	|| std::same_as<T, Quaternion>
	|| std::same_as<T, Vector3>
	|| std::same_as<T, std::string>
	|| std::same_as<T, BYTES_t>;

// NetSync is 500+ bytes (with all 7 maps)
// NetSync is 168 bytes (with 1 map only)
// NetSync is 144 bytes (combined member map, reduced members)
class NetSync {
public:
	static std::pair<HASH_t, HASH_t> ToHashPair(const std::string& key);

private:
	//enum TypePrefix {
	//	TP_FLOAT = 0b10,
	//	TP_VECTOR = 0b101,
	//	TP_QUATERNION = 0b1011,
	//	TP_INT = 0b10110,
	//	TP_LONG = 0b101101,
	//	TP_STRING = 0b1011011,
	//	TP_ARRAY = 0b10110111
	//}; 


	// PGW might stand for 'primary game world' version
	// packet gateway format (unlikely)
	// the static game PGW version is stored in ZoneSystem, which controls locations and sectoring
	// It might be closer to 'player game world' version
	static constexpr int32_t PGW_VERSION = 53;

	// https://stackoverflow.com/a/1122109
	enum class MemberShift : uint8_t {
		FLOAT = 0, // shift by 0 bits
		VECTOR3,
		QUATERNION,
		INT,
		STRING,
		LONG,
		ARRAY, // should be shift by 6 bits

		//MASK_FLOAT =		0b1,
		//MASK_VECTOR3 =		0b1 << 1,
		//MASK_QUATERNION =	0b1 << 2,
		//MASK_INT =			0b1 << 3,
		//MASK_STRING =		0b1 << 4,
		//MASK_LONG =			0b1 << 5,
		//MASK_ARRAY =		0b1 << 6
	};

	static HASH_t to_prefix(HASH_t hash, MemberShift pref);
	static HASH_t from_prefix(HASH_t hash, MemberShift pref);

	// Trivial getter
	template<TrivialSyncType T>
	const T* Get(HASH_t key, MemberShift prefix) {
		key = to_prefix(key, prefix);
		auto&& find = m_members.find(key);
		if (find != m_members.end()
			&& prefix == find->second.first) {
			return (T*)find->second.second;
		}
		return nullptr;
	}

	// Trivial getter w/ defaults
	template<TrivialSyncType T>
		requires (!std::same_as<T, BYTES_t>)
	const T& Get(HASH_t key, MemberShift prefix, const T& value) {
		key = to_prefix(key, prefix);
		auto&& find = m_members.find(key);
		if (find != m_members.end()
			&& prefix == find->second.first) {
			return *(T*)find->second.second;
		}
		return value;
	}

	template<TrivialSyncType T>
	void SetWith(HASH_t key, const T& value, MemberShift prefix) {
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
			m_members.insert({ key, std::make_pair(prefix, new T(value)) });
		}
	}

	template<TrivialSyncType T>
	void Set(HASH_t key, const T& value, MemberShift prefix) {
		SetWith(key, value, prefix);

		Revise();
	}

private:
	robin_hood::unordered_map<HASH_t, std::pair<MemberShift, void*>> m_members;

	void Revise() {
		m_dataRevision++;
	}

public:

	//~NetSync() = delete;
	~NetSync();



	enum class ObjectType : int8_t {
		Default,
		Prioritized,
		Solid,
		Terrain
	};





	// Trivial hash getters

	float GetFloat(HASH_t key, float value = 0);
	int32_t GetInt(HASH_t key, int32_t value = 0);
	int64_t GetLong(HASH_t key, int64_t value = 0);
	const Quaternion& GetQuaternion(HASH_t key, const Quaternion& value = Quaternion::IDENTITY);
	const Vector3& GetVector3(HASH_t key, const Vector3& value);
	const std::string& GetString(HASH_t key, const std::string& value = "");
	const BYTES_t* GetBytes(HASH_t key /* no default */);

	// Special hash getters

	bool GetBool(HASH_t key, bool value = false);
	const NetID GetID(const std::pair<HASH_t, HASH_t> &key /* no default */);



	// Trivial string getters

	float GetFloat(const std::string& key, float value = 0);
	int32_t GetInt(const std::string& key, int32_t value = 0);
	int64_t GetLong(const std::string& key, int64_t value = 0);
	const Quaternion& GetQuaternion(const std::string& key, const Quaternion& value = Quaternion::IDENTITY);
	const Vector3& GetVector3(const std::string& key, const Vector3& value);
	const std::string& GetString(const std::string& key, const std::string& value = "");
	const BYTES_t* GetBytes(const std::string& key /* no default */);

	// Special string getters

	bool GetBool(const std::string &key, bool value = false);
	const NetID GetID(const std::string &key /* no default */);



	// Trivial hash setters

	void Set(HASH_t key, float value);
	void Set(HASH_t key, int32_t value);
	void Set(HASH_t key, int64_t value);
	void Set(HASH_t key, const Quaternion &value);
	void Set(HASH_t key, const Vector3 &value);
	void Set(HASH_t key, const std::string &value);
	void Set(HASH_t key, const BYTES_t &value);

	// Special hash setters

	void Set(HASH_t key, bool value);
	void Set(const std::pair<HASH_t, HASH_t> &key, const NetID& value);



	// Trivial string setters (+bool)

	template<typename T> requires TrivialSyncType<T> || std::same_as<T, bool>
	void Set(const std::string& key, const T &value) { Set(Utils::GetStableHashCode(key), value); }
	void Set(const std::string& key, const std::string& value) { Set(Utils::GetStableHashCode(key), value); } // String overload
	void Set(const char* key, const std::string &value) { Set(Utils::GetStableHashCode(key), value); } // string constexpr overload

	// Special string setters

	void Set(const std::string& key, const NetID& value) { Set(ToHashPair(key), value); }
	void Set(const char* key, const NetID& value) { Set(ToHashPair(key), value); }





	uint8_t m_dataMask = 0;
	//uint8_t sizes[7];

	HASH_t m_prefab = 0;
	Vector2i m_sector;
	Vector3 m_position;
	Quaternion m_rotation = Quaternion::IDENTITY;

	// contains id of the client or server, and the object id
	NetID m_id;
	bool m_persistent = false;	// set by ZNetView; use bitmask
	bool m_distant = false;		// set by ZNetView; use bitmask
	UUID_t m_owner = 0;			// this seems to equal the local id, although it is not entirely confirmed
	int64_t m_timeCreated = 0;
	uint32_t m_ownerRevision = 0;
	uint32_t m_dataRevision = 0;
	//int32_t m_pgwVersion = 0; // 53 const mostly by ZNetView
	ObjectType m_type = ObjectType::Default; // set by ZNetView; use bitmask
	//float m_tempSortValue = 0; // only used in sending priority
	//bool m_tempHaveRevision = 0; // appears to be unused besides assignments
	//int32_t m_tempRemovedAt = -1; // equal to frame counter at intervals
	//int32_t m_tempCreatedAt = -1; // ^

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
	void SetOwner(UUID_t owner) {
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

	void Serialize(NetPackage::Ptr pkg);
	void Deserialize(NetPackage::Ptr pkg);
};
