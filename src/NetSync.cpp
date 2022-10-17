#include "NetSync.h"
#include "NetSyncManager.h"
#include <functional>
#include "ValhallaServer.h"

std::pair<hash_t, hash_t> NetSync::ToHashPair(const std::string& key) {
	return std::make_pair(
		Utils::GetStableHashCode(std::string(key + "_u")),
		Utils::GetStableHashCode(std::string(key + "_i"))
	);
}



hash_t NetSync::to_prefix(hash_t hash, TypePrefix pref) {
	return ((hash + pref) ^ pref)
		^ (pref << 13);
}

hash_t NetSync::from_prefix(hash_t hash, TypePrefix pref) {
	return (hash ^ (pref << 13)
		^ pref) - pref;
}



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





NetSync::~NetSync() {
	for (auto&& m : m_members) {
		auto&& pair = m.second;
		switch (pair.first) {
		case TP_FLOAT:		delete (float*)			pair.second; break;
		case TP_VECTOR:		delete (Vector3*)		pair.second; break;
		case TP_QUATERNION: delete (Quaternion*)	pair.second; break;
		case TP_INT:		delete (int32_t*)		pair.second; break;
		case TP_LONG:		delete (int64_t*)		pair.second; break;
		case TP_STRING:		delete (std::string*)	pair.second; break;
		case TP_ARRAY:		delete (bytes_t*)		pair.second; break;
		default:
			LOG(ERROR) << "Invalid type assigned to NetSync";
		}
	}
}


/*
* 
* 
*		 Hash Getters
* 
* 
*/

// Trivial

float NetSync::GetFloat(hash_t key, float value) {
	return Get(key, TP_FLOAT, value);
}

int32_t NetSync::GetInt(hash_t key, int32_t value) {
	return Get(key, TP_INT, value);
}

int64_t NetSync::GetLong(hash_t key, int64_t value) {
	return Get(key, TP_LONG, value);
}

const Quaternion& NetSync::GetQuaternion(hash_t key, const Quaternion& value) {
	return Get(key, TP_QUATERNION, value);
}

const Vector3& NetSync::GetVector3(hash_t key, const Vector3& value) {
	return Get(key, TP_VECTOR, value);
}

const std::string& NetSync::GetString(hash_t key, const std::string& value) {
	return Get(key, TP_STRING, value);
}

const bytes_t* NetSync::GetBytes(hash_t key) {
	return Get<bytes_t>(key, TP_ARRAY);
}

// Special

bool NetSync::GetBool(hash_t key, bool value) {
	return GetInt(key, value ? 1 : 0);
}

const NetSync::ID NetSync::GetID(const std::pair<hash_t, hash_t>& key) {
	auto k = GetLong(key.first);
	auto v = GetLong(key.second);
	if (k == 0 || v == 0)
		return NetSync::ID::NONE;
	return NetSync::ID(k, (uint32_t)v);
}



/*
*
*
*		 String Getters
*
*
*/

// Trivial

float NetSync::GetFloat(const std::string& key, float value) {
	return GetFloat(Utils::GetStableHashCode(key), value);
}

int32_t NetSync::GetInt(const std::string& key, int32_t value) {
	return GetInt(Utils::GetStableHashCode(key), value);
}

int64_t NetSync::GetLong(const std::string& key, int64_t value) {
	return GetLong(Utils::GetStableHashCode(key), value);
}

const Quaternion& NetSync::GetQuaternion(const std::string& key, const Quaternion& value) {
	return GetQuaternion(Utils::GetStableHashCode(key), value);
}

const Vector3& NetSync::GetVector3(const std::string& key, const Vector3& value) {
	return GetVector3(Utils::GetStableHashCode(key), value);
}

const std::string& NetSync::GetString(const std::string& key, const std::string& value) {
	return GetString(Utils::GetStableHashCode(key), value);
}

const bytes_t* NetSync::GetBytes(const std::string& key) {
	return GetBytes(Utils::GetStableHashCode(key));
}

// Special

bool NetSync::GetBool(const std::string& key, bool value) {
	return GetBool(Utils::GetStableHashCode(key), value);
}

const NetSync::ID NetSync::GetID(const std::string& key) {
	return GetID(ToHashPair(key));
}



/*
*
*
*		 Hash Setters
*
*
*/

// Trivial

void NetSync::Set(hash_t key, float value) {
	Set(key, value, TP_FLOAT);
}

void NetSync::Set(hash_t key, int32_t value) {
	Set(key, value, TP_INT);
}

void NetSync::Set(hash_t key, int64_t value) {
	Set(key, value, TP_LONG);
}

void NetSync::Set(hash_t key, const Quaternion& value) {
	Set(key, value, TP_QUATERNION);
}

void NetSync::Set(hash_t key, const Vector3& value) {
	Set(key, value, TP_VECTOR);
}

void NetSync::Set(hash_t key, const std::string& value) {
	Set(key, value, TP_STRING);
}

void NetSync::Set(hash_t key, const bytes_t &value) {
	Set(key, value, TP_ARRAY);
}

// Special

void NetSync::Set(hash_t key, bool value) {
	Set(key, value ? (int32_t)1 : 0);
}

void NetSync::Set(const std::pair<hash_t, hash_t> &key, const ID& value) {
	Set(key.first, value.m_userID);
	Set(key.second, (int64_t)value.m_id);
}



bool NetSync::Local() {
	return m_owner == Valhalla()->m_serverUuid;
}

void NetSync::SetLocal() {
	SetOwner(Valhalla()->m_serverUuid);
}
