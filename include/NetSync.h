#pragma once

#include <robin_hood.h>
#include <type_traits>
#include "Quaternion.h"
#include "Vector.h"
#include "Utils.h"





template<typename T>
concept TrivialSyncType = std::same_as<T, float>
	|| std::same_as<T, int32_t>
	|| std::same_as<T, int64_t>
	|| std::same_as<T, Quaternion>
	|| std::same_as<T, Vector3>
	|| std::same_as<T, std::string>
	|| std::same_as<T, bytes_t>;

// NetSync is 500+ bytes (with all 7 maps)
// NetSync is 168 bytes (with 1 map only)
// NetSync is 144 bytes (combined member map, reduced members)
class NetSync {
public:
	static std::pair<hash_t, hash_t> ToHashPair(const std::string& key);

private:
	enum TypePrefix {
		TP_FLOAT = 0b10,
		TP_VECTOR = 0b101,
		TP_QUATERNION = 0b1011,
		TP_INT = 0b10110,
		TP_LONG = 0b101101,
		TP_STRING = 0b1011011,
		TP_ARRAY = 0b10110111
	}; 

	static hash_t to_prefix(hash_t hash, TypePrefix pref);
	static hash_t from_prefix(hash_t hash, TypePrefix pref);

	// Trivial getter
	template<TrivialSyncType T>
	const T* Get(hash_t key, TypePrefix prefix) {
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
		requires (!std::same_as<T, bytes_t>)
	const T& Get(hash_t key, TypePrefix prefix, const T& value) {
		key = to_prefix(key, prefix);
		auto&& find = m_members.find(key);
		if (find != m_members.end()
			&& prefix == find->second.first) {
			return *(T*)find->second.second;
		}
		return value;
	}

	template<TrivialSyncType T>
	void Set(hash_t key, const T& value, TypePrefix prefix) {
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

		Revise();
	}

private:
	int m_prefab = 0;
	Vector2i m_sector;
	Vector3 m_position;
	Quaternion m_rotation = Quaternion::IDENTITY;

	robin_hood::unordered_map<int64_t, std::pair<TypePrefix, void*>> m_members;

	void Revise() {
		m_dataRevision++;
	}

public:

	//~NetSync() = delete;
	~NetSync();

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





	// Trivial hash getters

	float GetFloat(hash_t key, float value = 0);
	int32_t GetInt(hash_t key, int32_t value = 0);
	int64_t GetLong(hash_t key, int64_t value = 0);
	const Quaternion& GetQuaternion(hash_t key, const Quaternion& value = Quaternion::IDENTITY);
	const Vector3& GetVector3(hash_t key, const Vector3& value);
	const std::string& GetString(hash_t key, const std::string& value = "");
	const bytes_t* GetBytes(hash_t key /* no default */);

	// Special hash getters

	bool GetBool(hash_t key, bool value = false);
	const ID GetID(const std::pair<hash_t, hash_t> &key /* no default */);



	// Trivial string getters

	float GetFloat(const std::string& key, float value = 0);
	int32_t GetInt(const std::string& key, int32_t value = 0);
	int64_t GetLong(const std::string& key, int64_t value = 0);
	const Quaternion& GetQuaternion(const std::string& key, const Quaternion& value = Quaternion::IDENTITY);
	const Vector3& GetVector3(const std::string& key, const Vector3& value);
	const std::string& GetString(const std::string& key, const std::string& value = "");
	const bytes_t* GetBytes(const std::string& key /* no default */);

	// Special string getters

	bool GetBool(const std::string &key, bool value = false);
	const ID GetID(const std::string &key /* no default */);



	// Trivial hash setters

	void Set(hash_t key, float value);
	void Set(hash_t key, int32_t value);
	void Set(hash_t key, int64_t value);
	void Set(hash_t key, const Quaternion &value);
	void Set(hash_t key, const Vector3 &value);
	void Set(hash_t key, const std::string &value);
	void Set(hash_t key, const bytes_t &value);

	// Special hash setters

	void Set(hash_t key, bool value);
	void Set(const std::pair<hash_t, hash_t> &key, const ID& value);



	// Trivial string setters (+bool)

	template<typename T> requires TrivialSyncType<T> || std::same_as<T, bool>
	void Set(const std::string &key, const T &value) { Set(Utils::GetStableHashCode(key), value); }
	void Set(const std::string& key, const std::string& value) { Set(Utils::GetStableHashCode(key), value); } // String overload

	// Special string setters

	void Set(const std::string& key, const ID& value) { Set(ToHashPair(key), value); }



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
};
