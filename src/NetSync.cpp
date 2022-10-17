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



bool NetSync::GetBool(hash_t hash, bool def) const {
	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return find->second == 1 ? true : false;
	return def;
}
bool NetSync::GetBool(const std::string& key, bool def) const {
	return GetBool(Utils::GetStableHashCode(key), def);
}

const bytes_t* NetSync::GetBytes(hash_t hash) const {
	auto&& find = m_bytes.find(hash);
	if (find != m_bytes.end())
		return &find->second;
	return nullptr;
}

const bytes_t* NetSync::GetBytes(const std::string& key) const {
	return GetBytes(Utils::GetStableHashCode(key));
}

float NetSync::GetFloat(hash_t hash, float def) const {
	auto&& find = m_floats.find(hash);
	if (find != m_floats.end())
		return find->second;
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
	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return find->second;
	return def;
}
int32_t NetSync::GetInt(const std::string& key, int32_t def) const {
	return GetInt(Utils::GetStableHashCode(key), def);
}

int64_t NetSync::GetLong(hash_t hash, int64_t def) const {
	auto&& find = m_longs.find(hash);
	if (find != m_longs.end())
		return find->second;
	return def;
}
int64_t NetSync::GetLong(const std::string& key, int64_t def) const {
	return GetLong(Utils::GetStableHashCode(key));
}

const Quaternion& NetSync::GetQuaternion(hash_t hash, const Quaternion& def) const {
	auto&& find = m_quaternions.find(hash);
	if (find != m_quaternions.end())
		return find->second;
	return def;
}
const Quaternion& NetSync::GetQuaternion(const std::string& key, const Quaternion& def) const {
	return GetQuaternion(Utils::GetStableHashCode(key), def);
}

const std::string& NetSync::GetString(hash_t hash, const std::string& def) const {
	auto&& find = m_strings.find(hash);
	if (find != m_strings.end())
		return find->second;
	return def;
}
const std::string& NetSync::GetString(const std::string& key, const std::string& def) const {
	return GetString(Utils::GetStableHashCode(key), def);
}

const Vector3& NetSync::GetVector3(hash_t hash, const Vector3& def) const {
	auto&& find = m_vectors.find(hash);
	if (find != m_vectors.end())
		return find->second;
	return def;
}
const Vector3& NetSync::GetVector3(const std::string& key, const Vector3& def) const {
	return GetVector3(Utils::GetStableHashCode(key), def);
}






void NetSync::Set(hash_t hash, bool value) {
	return Set(hash, value ? 1 : 0);
}
void NetSync::Set(const std::string& key, bool value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(hash_t hash, const bytes_t& value) {
	m_bytes.insert({ hash, value });
	// increase data revision
	Revise();
}
void NetSync::Set(const std::string& key, const bytes_t& value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(hash_t hash, float value) {
	auto&& find = m_floats.find(hash);
	if (find != m_floats.end()
		&& find->second == value)
		return;
	m_floats.insert({ hash, value });
	Revise();
	// increase rev
}
void NetSync::Set(const std::string& key, float value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(const std::pair<hash_t, hash_t>& key, const NetSync::ID& value) {
	// it sets long for some reason for the key.value
	Set(key.first, value.m_userID);
	Set(key.second, (uuid_t)value.m_id);
}
void NetSync::Set(const std::string& key, const NetSync::ID& value) {
	return Set(ToHashPair(key), value);
}

void NetSync::Set(hash_t hash, int32_t value) {
	// alternative for performance?
	//auto insert = m_ints.insert({});

	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return;

	// Update data revision
	// IncreaseDataRevision
	m_ints.insert({ hash, value });
	Revise();
}
void NetSync::Set(const std::string& key, int32_t value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(hash_t hash, int64_t value) {
	auto&& find = m_longs.find(hash);
	if (find != m_longs.end() && value == find->second)
		return;
	m_longs.insert({ hash, value });
	Revise();
}
void NetSync::Set(const std::string& key, int64_t value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(hash_t hash, const Quaternion& value) {
	auto&& find = m_quaternions.find(hash);
	if (find != m_quaternions.end() && value == find->second)
		return;
	m_quaternions.insert({ hash, value });
	Revise();
}
void NetSync::Set(const std::string& key, const Quaternion& value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(hash_t hash, const std::string& value) {
	auto&& find = m_strings.find(hash);
	if (find != m_strings.end() && value == find->second)
		return;
	m_strings.insert({ hash, value });
	Revise();
}
void NetSync::Set(const std::string& key, const std::string& value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void NetSync::Set(hash_t hash, const Vector3& value) {
	auto&& find = m_vectors.find(hash);
	if (find != m_vectors.end() && value == find->second)
		return;
	m_vectors.insert({ hash, value });
	Revise();
}
void NetSync::Set(const std::string& key, const Vector3& value) {
	return Set(Utils::GetStableHashCode(key), value);
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


