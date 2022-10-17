#include "NetSync.h"
#include "NetSyncManager.h"
#include <functional>
#include "ValhallaServer.h"



const NetSync::ID NetSync::ID::NONE(0, 0);

NetSync::ID::ID()
	: ID(NONE)
{}

NetSync::ID::ID(int64_t userID, uint32_t id)
	: m_userID(userID), m_id(id), m_hash(HashUtils::Hasher{}(m_userID) ^ HashUtils::Hasher{}(m_id))
{}

std::string NetSync::ID::ToString() {
	return std::to_string(m_userID) + ":" + std::to_string(m_id);
}

bool NetSync::ID::operator==(const NetSync::ID& other) const {
	return m_userID == other.m_userID
		&& m_id == other.m_id;
}

bool NetSync::ID::operator!=(const NetSync::ID& other) const {
	return !(*this == other);
}

NetSync::ID::operator bool() const noexcept {
	return *this != NONE;
}





/*
// a better hashing system could utilize the 32 bits, somehow xorshift in accordance with the 
// prefix
hash_t to_prefix(hash_t hash, TypePrefix pref) {
	return ((hash ^ (hash_t) pref) 
		^ ((hash_t)pref << (hash_t)pref));
}

hash_t from_prefix(hash_t hash, TypePrefix pref) {
	return hash ^ ((hash_t)pref << (hash_t)pref)
		^ (hash_t)pref;
}

hash_t hash64to32(int64_t hash, char pref) {
	hash /= (int)pref;
	hash_t ret = hash - ((int)pref) * 256;
	return ret;
}



// convert the 32 bit hash to 64
int64_t hash32to64(int32_t hash, char pref) {
	int64_t ret = hash + ((int)pref) * 256;
	ret *= ((int)pref);
	return ret;
}

hash_t hash64to32(int64_t hash, char pref) {
	hash /= (int) pref;
	hash_t ret = hash - ((int)pref) * 256;
	return ret;
}*/

hash_t to_prefix(hash_t hash, TypePrefix pref) {
	return ((hash + pref) ^ pref)
		^ (pref << 13);
}

hash_t from_prefix(hash_t hash, TypePrefix pref) {
	return (hash ^ (pref << 13)
		^ pref) - pref;
}





bool NetSync::GetBool(hash_t hash, bool def) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::INT));
	if (find != m_members.end() && find->second.first == TypePrefix::INT) {
		return *((int*)find->second.second) == 1 ? true : false;
	}
	return def;
}
bool NetSync::GetBool(const std::string& key, bool def) const {
	return GetBool(Utils::GetStableHashCode(key), def);
}

const bytes_t* NetSync::GetBytes(hash_t hash) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::ARR));
	if (find != m_members.end() && find->second.first == TypePrefix::ARR) {
		return (bytes_t*)find->second.second;
	}
	return nullptr;
}

const bytes_t* NetSync::GetBytes(const std::string& key) const {
	return GetBytes(Utils::GetStableHashCode(key));
}

float NetSync::GetFloat(hash_t hash, float def) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::FLOAT));
	if (find != m_members.end() && find->second.first == TypePrefix::FLOAT) {
		return *(float*)find->second.second;
	}
	return def;
}
float NetSync::GetFloat(const std::string& key, float def) const {
	return GetFloat(Utils::GetStableHashCode(key), def);
}

NetSync::ID NetSync::GetNetSyncID(const std::pair<hash_t, hash_t>& pair) const {
	auto k = GetLong(pair.first);
	auto v = GetLong(pair.second);
	if (k == 0 || v == 0)
		return NetSync::ID::NONE;
	return NetSync::ID(k, (uint32_t)v);
}
NetSync::ID NetSync::GetNetSyncID(const std::string& key) const {
	return GetNetSyncID(ToHashPair(key));
}

int32_t NetSync::GetInt(hash_t hash, int32_t def) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::INT));
	if (find != m_members.end() && find->second.first == TypePrefix::INT) {
		return *(int32_t*)find->second.second;
	}
	return def;
}
int32_t NetSync::GetInt(const std::string& key, int32_t def) const {
	return GetInt(Utils::GetStableHashCode(key), def);
}

int64_t NetSync::GetLong(hash_t hash, int64_t def) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::LONG));
	if (find != m_members.end() && find->second.first == TypePrefix::LONG) {
		return *(int64_t*)find->second.second;
	}
	return def;
}
int64_t NetSync::GetLong(const std::string& key, int64_t def) const {
	return GetLong(Utils::GetStableHashCode(key));
}

const Quaternion& NetSync::GetQuaternion(hash_t hash, const Quaternion& def) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::QUAT));
	if (find != m_members.end() && find->second.first == TypePrefix::QUAT) {
		return *(Quaternion*)find->second.second;
	}
	return def;
}
const Quaternion& NetSync::GetQuaternion(const std::string& key, const Quaternion& def) const {
	return GetQuaternion(Utils::GetStableHashCode(key), def);
}

const std::string& NetSync::GetString(hash_t hash, const std::string& def) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::STR));
	if (find != m_members.end() && find->second.first == TypePrefix::STR) {
		return *(std::string*)find->second.second;
	}
	return def;
}
const std::string& NetSync::GetString(const std::string& key, const std::string& def) const {
	return GetString(Utils::GetStableHashCode(key), def);
}

const Vector3& NetSync::GetVector3(hash_t hash, const Vector3& def) const {
	auto&& find = m_members.find(to_prefix(hash, TypePrefix::VECTOR));
	if (find != m_members.end() && find->second.first == TypePrefix::VECTOR) {
		return *(Vector3*)find->second.second;
	}
	return def;
}
const Vector3& NetSync::GetVector3(const std::string& key, const Vector3& def) const {
	return GetVector3(Utils::GetStableHashCode(key), def);
}





void NetSync::Set(hash_t key, float value) {
	return Set(key, value, FLOAT);
}

void NetSync::Set(hash_t key, int32_t value) {
	return Set(key, value, INT);
}

void NetSync::Set(hash_t key, int64_t value) {
	return Set(key, value, LONG);
}

void NetSync::Set(hash_t key, const Quaternion& value) {
	return Set(key, value, QUAT);
}

void NetSync::Set(hash_t key, const Vector3& value) {
	return Set(key, value, VECTOR);
}

void NetSync::Set(hash_t key, const std::string& value) {
	return Set(key, value, STR);
}

void NetSync::Set(hash_t key, const bytes_t &value) {
	return Set(key, value, ARR);
}



// special overloads
void NetSync::Set(hash_t key, bool value) {
	return Set(key, value ? (int32_t)1 : 0);
}

void NetSync::Set(std::pair<hash_t, hash_t> key, const ID& value) {
	//return Set(key, value ?);
	Set(key.first, value.m_userID);
	Set(key.second, (int64_t)value.m_id);
}



// special overloads str->hash
void NetSync::Set(const std::string& key, bool value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(const std::string& key, const ID& value) {
	return Set(ToHashPair(key), value);
}




std::pair<hash_t, hash_t> NetSync::ToHashPair(const std::string& key) {
	return std::make_pair(
		Utils::GetStableHashCode(std::string(key + "_u")),
		Utils::GetStableHashCode(std::string(key + "_i"))
	);
}



bool NetSync::Local() {
	return m_owner == Valhalla()->m_serverUuid;
}

void NetSync::SetLocal() {
	SetOwner(Valhalla()->m_serverUuid);
}


