#include "NetSync.h"
#include "NetSyncManager.h"
#include <functional>
#include "ValhallaServer.h"


//constexpr std::pair<hash_t, hash_t> ToHashPair(const char* key) {
//	//constexpr auto s = "hello" " " "world";
//
//	return std::make_pair(
//		Utils::GetStableHashCode(key " "),
//		Utils::GetStableHashCode(key " ")
//	);
//}

std::pair<hash_t, hash_t> NetSync::ToHashPair(const std::string& key) {
	return std::make_pair(
		Utils::GetStableHashCode(std::string(key + "_u")),
		Utils::GetStableHashCode(std::string(key + "_i"))
	);
}



hash_t NetSync::to_prefix(hash_t hash, MemberShift pref) {
	auto tshift = static_cast<hash_t>(pref);

	return (hash
		+ (tshift * tshift)
			^ tshift)
				^ (tshift << tshift);

	//return 
	//	((hash ^ 
	//		(tshift << tshift)) 
	//			^ tshift)
	//				- (tshift * tshift);
}

hash_t NetSync::from_prefix(hash_t hash, MemberShift pref) {
	auto tshift = static_cast<hash_t>(pref);
	return 
		((hash 
			^ (tshift << tshift)) 
				^ tshift)
					- (tshift * tshift);
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
		case MemberShift::FLOAT:		delete (float*)			pair.second; break;
		case MemberShift::VECTOR3:		delete (Vector3*)		pair.second; break;
		case MemberShift::QUATERNION:	delete (Quaternion*)	pair.second; break;
		case MemberShift::INT:			delete (int32_t*)		pair.second; break;
		case MemberShift::LONG:			delete (int64_t*)		pair.second; break;
		case MemberShift::STRING:		delete (std::string*)	pair.second; break;
		case MemberShift::ARRAY:		delete (bytes_t*)		pair.second; break;
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
	return Get(key, MemberShift::FLOAT, value);
}

int32_t NetSync::GetInt(hash_t key, int32_t value) {
	return Get(key, MemberShift::INT, value);
}

int64_t NetSync::GetLong(hash_t key, int64_t value) {
	return Get(key, MemberShift::LONG, value);
}

const Quaternion& NetSync::GetQuaternion(hash_t key, const Quaternion& value) {
	return Get(key, MemberShift::QUATERNION, value);
}

const Vector3& NetSync::GetVector3(hash_t key, const Vector3& value) {
	return Get(key, MemberShift::VECTOR3, value);
}

const std::string& NetSync::GetString(hash_t key, const std::string& value) {
	return Get(key, MemberShift::STRING, value);
}

const bytes_t* NetSync::GetBytes(hash_t key) {
	return Get<bytes_t>(key, MemberShift::ARRAY);
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
	Set(key, value, MemberShift::FLOAT);
}

void NetSync::Set(hash_t key, int32_t value) {
	Set(key, value, MemberShift::INT);
}

void NetSync::Set(hash_t key, int64_t value) {
	Set(key, value, MemberShift::LONG);
}

void NetSync::Set(hash_t key, const Quaternion& value) {
	Set(key, value, MemberShift::QUATERNION);
}

void NetSync::Set(hash_t key, const Vector3& value) {
	Set(key, value, MemberShift::VECTOR3);
}

void NetSync::Set(hash_t key, const std::string& value) {
	Set(key, value, MemberShift::STRING);
}

void NetSync::Set(hash_t key, const bytes_t &value) {
	Set(key, value, MemberShift::ARRAY);
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



void NetSync::Serialize(NetPackage::Ptr pkg) {
	pkg->Write(m_persistent);
	pkg->Write(m_distant);
	pkg->Write(m_timeCreated);
	pkg->Write(PGW_VERSION);
	pkg->Write(m_type); // sbyte
	pkg->Write(m_prefab);
	pkg->Write(m_rotation);
	int num = 0;

	// sections organized like this:
	//	32 bit mask: F V Q I S L A
	//  for each present type in order:
	//		

	/*
	if (this.m_floats != null && this.m_floats.Count > 0)
	{
		num |= 1;
	}
	if (this.m_vec3 != null && this.m_vec3.Count > 0)
	{
		num |= 2;
	}
	if (this.m_quats != null && this.m_quats.Count > 0)
	{
		num |= 4;
	}
	if (this.m_ints != null && this.m_ints.Count > 0)
	{
		num |= 8;
	}
	if (this.m_strings != null && this.m_strings.Count > 0)
	{
		num |= 16;
	}
	if (this.m_longs != null && this.m_longs.Count > 0)
	{
		num |= 64;
	}
	if (this.m_byteArrays != null && this.m_byteArrays.Count > 0)
	{
		num |= 128;
	}*/

	// pkg.Write(num);
	pkg->Write((int32_t)m_dataMask);
	
#define TYPE_SERIALIZE(type, mtype)	\
	if ((m_dataMask >> static_cast<uint8_t>(mtype)) & 0b1) { \
		auto size_mark = pkg->m_stream.Marker(); \
		uint8_t size = 0; \
		pkg->Write(size); \
		for (auto&& pair : m_members) { \
			if (pair.second.first != mtype) \
				continue; \
			size++; \
			pkg->Write(from_prefix(pair.first, mtype)); \
			pkg->Write(*(type*)pair.second.second); \
		} \
		auto end_mark = pkg->m_stream.Marker(); \
		pkg->m_stream.SetMarker(size_mark); \
		pkg->Write(size); \
		pkg->m_stream.SetMarker(end_mark); \
	}



	// 2 Optimization ideas:
	//	- using an array[7] with sizes, used to write size directly
	//		- pros: type amount known with random access; lower cpu
	//		- cons: requires a 7 byte array for every ZDO; higher ram usage; more complexity for every set/get
	//	- bitmask to determine presence, skip a pkg byte for later size write, then iteration of members while counting
	//		- pros: straightforward; low memory utilization; only complex in sends/receives
	//		- cons: higher iterations, even useless

	// Proposed optimization #1
	//pkg->Write(sizes[shift]);
	//uint8_t num = 0;
	//for (auto&& pair : m_members) {
	//	// break early for optimization
	//	if (pair.second.first != MemberShift::FLOAT || num == sizes[shift])
	//		break;
	//
	//	num++;
	//	
	//	pkg->Write(from_prefix(pair.first,));
	//}

	TYPE_SERIALIZE(float, MemberShift::FLOAT);
	TYPE_SERIALIZE(Vector3, MemberShift::VECTOR3);
	TYPE_SERIALIZE(Quaternion, MemberShift::QUATERNION);
	TYPE_SERIALIZE(int32_t, MemberShift::INT);
	TYPE_SERIALIZE(std::string, MemberShift::STRING);
	TYPE_SERIALIZE(int64_t, MemberShift::LONG);
	TYPE_SERIALIZE(bytes_t, MemberShift::ARRAY);
}

void NetSync::Deserialize(NetPackage::Ptr pkg) {
	m_persistent = pkg->Read<bool>();
	m_distant = pkg->Read<bool>();
	m_timeCreated = pkg->Read<int64_t>();
	auto m_pgwVersion = pkg->Read<int32_t>(); // version 
	m_type = pkg->Read<ObjectType>();
	m_prefab = pkg->Read<hash_t>();
	m_rotation = pkg->Read<Quaternion>();

	m_dataMask = pkg->Read<int32_t>();

#define TYPE_DESERIALIZE(type, mtype) \
	if ((m_dataMask >> static_cast<uint8_t>(mtype)) & 0b1) { \
		auto count = pkg->Read<uint8_t>(); \
		while (count--) { \
			auto key = pkg->Read<hash_t>(); \
			SetWith(key, pkg->Read<type>(), mtype); \
		} \
	}

	TYPE_DESERIALIZE(float, MemberShift::FLOAT);
	TYPE_DESERIALIZE(Vector3, MemberShift::VECTOR3);
	TYPE_DESERIALIZE(Quaternion, MemberShift::QUATERNION);
	TYPE_DESERIALIZE(int32_t, MemberShift::INT);
	TYPE_DESERIALIZE(std::string, MemberShift::STRING);
	TYPE_DESERIALIZE(int64_t, MemberShift::LONG);
	TYPE_DESERIALIZE(bytes_t, MemberShift::ARRAY);
}