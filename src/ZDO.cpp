#include "ZDO.h"
#include "ZDOMan.h"


bool ZDO::GetBool(hash_t hash, bool def = false) {
	auto&& find = m_ints.find(hash);
	if (find != m_ints.end())
		return find->second == 1 ? true : false;
	return def;
}
bool ZDO::GetBool(const std::string& key, bool def = false) {
	return GetBool(Utils::GetStableHashCode(key), def);
}

const bytes_t* ZDO::GetBytes(hash_t hash) {
	auto&& find = m_byteArrays.find(hash);
	if (find != m_byteArrays.end())
		return &find->second;
	return nullptr;
}

const bytes_t* ZDO::GetBytes(const std::string& key) {
	return GetBytes(Utils::GetStableHashCode(key));
}

float ZDO::GetFloat(hash_t hash, float def = 0) {

}
float ZDO::GetFloat(const std::string& key, float def = 0) {

}

const ZDOID& ZDO::GetZDOID(std::pair<hash_t, hash_t>& key) {

}
std::pair<hash_t, hash_t> ZDO::GetHashZDOID(const std::string& key) {

}
const ZDOID& ZDO::GetZDOID(const std::string& key) {

}

int32_t ZDO::GetInt(hash_t hash, int32_t def = 0) {

}
int32_t ZDO::GetInt(const std::string& key, int32_t def = 0) {

}

int64_t ZDO::GetLong(hash_t hash, int64_t def = 0) {

}
int64_t ZDO::GetLong(const std::string& key, int64_t def = 0) {

}

const Quaternion& ZDO::GetQuaternion(hash_t hash) {

}
const Quaternion& ZDO::GetQuaternion(const std::string& key) {

}

const std::string& ZDO::GetString(hash_t hash, const std::string& def = "") {

}
const std::string& ZDO::GetString(const std::string& key, const std::string& def = "") {

}

const Vector3& ZDO::GetVector3(hash_t hash, const Vector3& def) {

}
const Vector3& ZDO::GetVector3(const std::string& key, const Vector3& def) {

}