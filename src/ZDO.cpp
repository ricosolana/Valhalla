#include "ZDO.h"
#include "ZDOMan.h"


bool ZDO::GetBool(hash_t hash, bool def) {
	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return find->second == 1 ? true : false;
	return def;
}
bool ZDO::GetBool(const std::string& key, bool def) {
	return GetBool(Utils::GetStableHashCode(key), def);
}

std::optional<const bytes_t&> ZDO::GetBytes(hash_t hash) {
	auto&& find = m_byteArrays.find(hash);
	if (find != m_byteArrays.end())
		return find->second;
	return std::optional<const bytes_t&>();
}

std::optional<const bytes_t&> ZDO::GetBytes(const std::string& key) {
	return GetBytes(Utils::GetStableHashCode(key));
}

float ZDO::GetFloat(hash_t hash, float def) {
	auto&& find = m_floats.find(hash);
	if (find != m_floats.end())
		return find->second;
	return def;
}
float ZDO::GetFloat(const std::string& key, float def) {
	return GetFloat(Utils::GetStableHashCode(key), def);
}

const ZDOID& ZDO::GetZDOID(const std::pair<hash_t, hash_t>& pair) {
	auto k = GetLong(pair.first);
	auto v = GetLong(pair.second);
	if (k == 0 || v == 0)
		return ZDOID::NONE;
	return ZDOID(k, (uint32_t)v);
}
const ZDOID& ZDO::GetZDOID(const std::string& key) {
	return GetZDOID(ToHashPair(key));
}

int32_t ZDO::GetInt(hash_t hash, int32_t def) {
	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return find->second;
	return def;
}
int32_t ZDO::GetInt(const std::string& key, int32_t def) {
	return GetInt(Utils::GetStableHashCode(key), def);
}

int64_t ZDO::GetLong(hash_t hash, int64_t def) {
	auto&& find = m_longs.find(hash);
	if (find != m_longs.end())
		return find->second;
	return def;
}
int64_t ZDO::GetLong(const std::string& key, int64_t def) {
	return GetLong(Utils::GetStableHashCode(key));
}

const Quaternion& ZDO::GetQuaternion(hash_t hash, const Quaternion& def) {
	auto&& find = m_quats.find(hash);
	if (find != m_quats.end())
		return find->second;
	return def;
}
const Quaternion& ZDO::GetQuaternion(const std::string& key, const Quaternion& def) {
	return GetQuaternion(Utils::GetStableHashCode(key), def);
}

const std::string& ZDO::GetString(hash_t hash, const std::string& def = "") {
	auto&& find = m_strings.find(hash);
	if (find != m_strings.end())
		return find->second;
	return def;
}
const std::string& ZDO::GetString(const std::string& key, const std::string& def = "") {
	return GetString(Utils::GetStableHashCode(key), def);
}

const Vector3& ZDO::GetVector3(hash_t hash, const Vector3& def) {
	auto&& find = m_vecs.find(hash);
	if (find != m_vecs.end())
		return find->second;
	return def;
}
const Vector3& ZDO::GetVector3(const std::string& key, const Vector3& def) {
	return GetVector3(Utils::GetStableHashCode(key), def);
}






void ZDO::Set(hash_t hash, bool value) {
	return Set(hash, value ? 1 : 0);
}
void ZDO::Set(const std::string& key, bool value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void ZDO::Set(hash_t hash, const bytes_t& value) {
	m_byteArrays.insert({ hash, value });
	// increase data revision
}
void ZDO::Set(const std::string& key, const bytes_t& value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void ZDO::Set(hash_t hash, float value) {
	auto&& find = m_floats.find(hash);
	if (find != m_floats.end()
		&& find->second == value)
		return;
	m_floats.insert({ hash, value });
	// increase rev
}
void ZDO::Set(const std::string& key, float value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void ZDO::Set(const std::pair<hash_t, hash_t>& key, const ZDOID& value) {
	// it sets long for some reason for the key.value
	Set(key.first, value.m_userID);
	Set(key.second, (uuid_t)value.m_id);
}
void ZDO::Set(const std::string& key, const ZDOID& value) {
	return Set(ToHashPair(key), value);
}

void ZDO::Set(hash_t hash, int32_t value) {
	// alternative for performance?
	//auto insert = m_ints.insert({});

	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return;

	// Update data revision
	// IncreaseDataRevision
	m_ints.insert({ hash, value });
}
void ZDO::Set(const std::string& key, int32_t value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void ZDO::Set(hash_t hash, int64_t value) {
	auto&& find = m_longs.find(hash);
	if (find != m_longs.end() && value == find->second)
		return;
	m_longs.insert({ hash, value });
}
void ZDO::Set(const std::string& key, int64_t value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void ZDO::Set(hash_t hash, const Quaternion& value) {
	auto&& find = m_quats.find(hash);
	if (find != m_quats.end() && value == find->second)
		return;
	m_quats.insert({ hash, value });
}
void ZDO::Set(const std::string& key, const Quaternion& value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void ZDO::Set(hash_t hash, const std::string& value) {
	auto&& find = m_strings.find(hash);
	if (find != m_strings.end() && value == find->second)
		return;
	m_strings.insert({ hash, value });
}
void ZDO::Set(const std::string& key, const std::string& value) {
	return Set(Utils::GetStableHashCode(key), value);
}

void ZDO::Set(hash_t hash, const Vector3& value) {
	auto&& find = m_vecs.find(hash);
	if (find != m_vecs.end() && value == find->second)
		return;
	m_vecs.insert({ hash, value });
}
void ZDO::Set(const std::string& key, const Vector3& value) {
	return Set(Utils::GetStableHashCode(key), value);
}



std::pair<hash_t, hash_t> ZDO::ToHashPair(const std::string& key) {
	return std::make_pair(
		Utils::GetStableHashCode(std::string(key + "_u")),
		Utils::GetStableHashCode(std::string(key + "_i"))
	);
}
