#include "NetSync.h"
#include "NetSyncManager.h"
#include <functional>

bool NetSync::GetBool(hash_t hash, bool def) {
	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return find->second == 1 ? true : false;
	return def;
}
bool NetSync::GetBool(const std::string& key, bool def) {
	return GetBool(Utils::GetStableHashCode(key), def);
}

/*std::optional<std::reference_wrapper<bytes_t>>*/ bytes_t* NetSync::GetBytes(hash_t hash) {
	auto&& find = m_bytes.find(hash);
	if (find != m_bytes.end())
		return &find->second;
	return nullptr; //std::optional<const bytes_t&>();
}

bytes_t* NetSync::GetBytes(const std::string& key) {
	return GetBytes(Utils::GetStableHashCode(key));
}

float NetSync::GetFloat(hash_t hash, float def) {
	auto&& find = m_floats.find(hash);
	if (find != m_floats.end())
		return find->second;
	return def;
}
float NetSync::GetFloat(const std::string& key, float def) {
	return GetFloat(Utils::GetStableHashCode(key), def);
}

const NetSyncID& NetSync::GetNetSyncID(const std::pair<hash_t, hash_t>& pair) {
	auto k = GetLong(pair.first);
	auto v = GetLong(pair.second);
	if (k == 0 || v == 0)
		return NetSyncID::NONE;
	return NetSyncID(k, (uint32_t)v);
}
const NetSyncID& NetSync::GetNetSyncID(const std::string& key) {
	return GetNetSyncID(ToHashPair(key));
}

int32_t NetSync::GetInt(hash_t hash, int32_t def) {
	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return find->second;
	return def;
}
int32_t NetSync::GetInt(const std::string& key, int32_t def) {
	return GetInt(Utils::GetStableHashCode(key), def);
}

int64_t NetSync::GetLong(hash_t hash, int64_t def) {
	auto&& find = m_longs.find(hash);
	if (find != m_longs.end())
		return find->second;
	return def;
}
int64_t NetSync::GetLong(const std::string& key, int64_t def) {
	return GetLong(Utils::GetStableHashCode(key));
}

const Quaternion& NetSync::GetQuaternion(hash_t hash, const Quaternion& def) {
	auto&& find = m_quaternions.find(hash);
	if (find != m_quaternions.end())
		return find->second;
	return def;
}
const Quaternion& NetSync::GetQuaternion(const std::string& key, const Quaternion& def) {
	return GetQuaternion(Utils::GetStableHashCode(key), def);
}

const std::string& NetSync::GetString(hash_t hash, const std::string& def) {
	auto&& find = m_strings.find(hash);
	if (find != m_strings.end())
		return find->second;
	return def;
}
const std::string& NetSync::GetString(const std::string& key, const std::string& def) {
	return GetString(Utils::GetStableHashCode(key), def);
}

const Vector3& NetSync::GetVector3(hash_t hash, const Vector3& def) {
	auto&& find = m_vectors.find(hash);
	if (find != m_vectors.end())
		return find->second;
	return def;
}
const Vector3& NetSync::GetVector3(const std::string& key, const Vector3& def) {
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

void NetSync::Set(const std::pair<hash_t, hash_t>& key, const NetSyncID& value) {
	// it sets long for some reason for the key.value
	Set(key.first, value.m_userID);
	Set(key.second, (uuid_t)value.m_id);
}
void NetSync::Set(const std::string& key, const NetSyncID& value) {
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
