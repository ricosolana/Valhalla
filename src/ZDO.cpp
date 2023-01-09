#include <functional>

#include "ZDO.h"
//#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "NetID.h"
//#include "ZoneSystem.h"

//constexpr std::pair<HASH_t, HASH_t> ToHashPair(const char* key) {
//	//constexpr auto s = "hello" " " "world";
//
//	return std::make_pair(
//		VUtils::GetStableHashCode(key " "),
//		VUtils::GetStableHashCode(key " ")
//	);
//}

std::pair<HASH_t, HASH_t> ZDO::ToHashPair(const std::string& key) {
	return std::make_pair(
		VUtils::String::GetStableHashCode(std::string(key + "_u")),
		VUtils::String::GetStableHashCode(std::string(key + "_i"))
	);
}


/*
HASH_t ZDO::ToShiftHash(HASH_t hash, MemberShift pref) {
	auto tshift = static_cast<HASH_t>(pref);
	return (hash
		+ (tshift * tshift)
			^ tshift)
				^ (tshift << tshift);
}
HASH_t ZDO::from_prefix(HASH_t hash, MemberShift pref) {
	auto tshift = static_cast<HASH_t>(pref);
	return
		((hash
			^ (tshift << tshift))
				^ tshift)
					- (tshift * tshift);
}
*/



ZDO::ZDO() {
	this->m_owner = Valhalla()->ID();
}

ZDO::ZDO(NetPackage pkg, int version) {
	this->m_rev.m_ownerRev = pkg.Read<uint32_t>();
	this->m_rev.m_dataRev = pkg.Read<uint32_t>();
	this->m_persistent = pkg.Read<bool>();
	this->m_owner = pkg.Read<OWNER_t>();
	this->m_rev.m_time = pkg.Read<int64_t>();
	this->m_pgwVersion = pkg.Read<int32_t>();

	if (version >= 16 && version < 24)
		pkg.Read<int32_t>();

	if (version >= 23)
		this->m_type = pkg.Read<ObjectType>();

	if (version >= 22)
		this->m_distant = pkg.Read<bool>();

	if (version < 13) {
		pkg.Read<char>();
		pkg.Read<char>();
	}

	if (version >= 17)
		this->m_prefab = pkg.Read<HASH_t>();

	this->m_sector = pkg.Read<Vector2i>();
	this->m_position = pkg.Read<Vector3>();
	this->m_rotation = pkg.Read<Quaternion>();

#define TYPE_LOAD(T) \
{ \
	auto count = pkg.Read<uint8_t>(); \
	while (count--) { \
		auto key = pkg.Read<HASH_t>(); \
		_Set(key, pkg.Read<T>()); \
	} \
}

	TYPE_LOAD(float);
	TYPE_LOAD(Vector3);
	TYPE_LOAD(Quaternion);
	TYPE_LOAD(int32_t);
	TYPE_LOAD(int64_t);
	TYPE_LOAD(std::string);
	TYPE_LOAD(BYTES_t);

	if (version < 17)
		this->m_prefab = GetInt("prefab", 0);
}

void ZDO::Save(NetPackage& pkg) {
	pkg.Write(this->m_rev.m_ownerRev);
	pkg.Write(this->m_rev.m_dataRev);
	pkg.Write(this->m_persistent);
	pkg.Write(this->m_owner);
	pkg.Write(this->m_rev.m_time);
	pkg.Write(this->m_pgwVersion);
	pkg.Write(this->m_type);
	pkg.Write(this->m_distant);
	pkg.Write(this->m_prefab);
	pkg.Write(this->m_sector);
	pkg.Write(this->m_position);
	pkg.Write(this->m_rotation);

	_TrySaveType<float>(pkg);
	_TrySaveType<Vector3>(pkg);
	_TrySaveType<Quaternion>(pkg);
	_TrySaveType<int32_t>(pkg);
	_TrySaveType<int64_t>(pkg);
	_TrySaveType<std::string>(pkg);
	_TrySaveType<BYTES_t>(pkg);
}

// copy constructor
ZDO::ZDO(const ZDO& other) {
	// Member copy
	this->m_persistent = other.m_persistent;
	this->m_distant = other.m_distant;
	this->m_pgwVersion = other.m_pgwVersion;
	this->m_type = other.m_type;
	this->m_prefab = other.m_prefab;
	this->m_rotation = other.m_rotation;
	this->m_dataMask = other.m_dataMask;

	this->m_sector = other.m_sector;
	this->m_position = other.m_position;
	this->m_id = other.m_id;
	this->m_owner = other.m_owner;

	this->m_rev = other.m_rev;

	// Pool copy
	for (auto&& pair1 : other.m_members) {
		_Set(pair1.first, pair1.second.second, pair1.second.first);
	}
}



ZDO::~ZDO() {
	FreeMembers();
}



/*
*
*
*		 Hash Getters
*
*
*/

// Trivial

float ZDO::GetFloat(HASH_t key, float value) {
	return Get<float>(key, value);
}

int32_t ZDO::GetInt(HASH_t key, int32_t value) {
	return Get<int32_t>(key, value);
}

int64_t ZDO::GetLong(HASH_t key, int64_t value) {
	return Get<int64_t>(key, value);
}

const Quaternion& ZDO::GetQuaternion(HASH_t key, const Quaternion& value) {
	return Get<Quaternion>(key, value);
}

const Vector3& ZDO::GetVector3(HASH_t key, const Vector3& value) {
	return Get<Vector3>(key, value);
}

const std::string& ZDO::GetString(HASH_t key, const std::string& value) {
	return Get<std::string>(key, value);
}

const BYTES_t* ZDO::GetBytes(HASH_t key) {
	return _Get<BYTES_t>(key);
}

// Special

bool ZDO::GetBool(HASH_t key, bool value) {
	return GetInt(key, value ? 1 : 0);
}

NetID ZDO::GetNetID(const std::pair<HASH_t, HASH_t>& key) {
	auto k = GetLong(key.first);
	auto v = GetLong(key.second);
	if (k == 0 || v == 0)
		return NetID::NONE;
	return NetID(k, (uint32_t)v);
}



/*
*
*
*		 String Getters
*
*
*/

// Trivial

float ZDO::GetFloat(const std::string& key, float value) {
	return GetFloat(VUtils::String::GetStableHashCode(key), value);
}

int32_t ZDO::GetInt(const std::string& key, int32_t value) {
	return GetInt(VUtils::String::GetStableHashCode(key), value);
}

int64_t ZDO::GetLong(const std::string& key, int64_t value) {
	return GetLong(VUtils::String::GetStableHashCode(key), value);
}

const Quaternion& ZDO::GetQuaternion(const std::string& key, const Quaternion& value) {
	return GetQuaternion(VUtils::String::GetStableHashCode(key), value);
}

const Vector3& ZDO::GetVector3(const std::string& key, const Vector3& value) {
	return GetVector3(VUtils::String::GetStableHashCode(key), value);
}

const std::string& ZDO::GetString(const std::string& key, const std::string& value) {
	return GetString(VUtils::String::GetStableHashCode(key), value);
}

const BYTES_t* ZDO::GetBytes(const std::string& key) {
	return GetBytes(VUtils::String::GetStableHashCode(key));
}

// Special

bool ZDO::GetBool(const std::string& key, bool value) {
	return GetBool(VUtils::String::GetStableHashCode(key), value);
}

NetID ZDO::GetNetID(const std::string& key) {
	return GetNetID(ToHashPair(key));
}



/*
*
*
*		 Hash Setters
*
*
*/

// Trivial

// Special

void ZDO::Set(HASH_t key, bool value) {
	Set(key, value ? (int32_t)1 : 0);
}

void ZDO::Set(const std::pair<HASH_t, HASH_t>& key, const NetID& value) {
	Set(key.first, value.m_uuid);
	Set(key.second, (int64_t)value.m_id);
}



bool ZDO::Local() {
	return m_owner == Valhalla()->ID();
}

void ZDO::SetLocal() {
	SetOwner(Valhalla()->ID());
}

void ZDO::SetPosition(const Vector3& pos) {
	if (m_position != pos) {
		m_position = pos;
		assert(false);
		//SetSector(ZoneSystem::GetZoneCoords(m_position));
		if (Local())
			Revise();
	}
}

void ZDO::SetSector(const Vector2i& sector) {
	if (m_sector == sector) {
		return;
	}
	assert(false);
	//ZDOManager::RemoveFromSector(this, m_sector);
	m_sector = sector;
	//ZDOManager::AddToSector(this, m_sector);
	//ZDOManager::ZDOSectorInvalidated(this);
}

void ZDO::FreeMembers() {
	for (auto&& m : m_members) {
		auto&& pair = m.second;
		switch (pair.first) {
		case MemberShift::FLOAT:		delete (float*)pair.second; break;
		case MemberShift::VECTOR3:		delete (Vector3*)pair.second; break;
		case MemberShift::QUATERNION:	delete (Quaternion*)pair.second; break;
		case MemberShift::INT:			delete (int32_t*)pair.second; break;
		case MemberShift::LONG:			delete (int64_t*)pair.second; break;
		case MemberShift::STRING:		delete (std::string*)pair.second; break;
		case MemberShift::ARRAY:		delete (BYTES_t*)pair.second; break;
		}
	}
}

void ZDO::Invalidate() {
	this->m_id = ZDOID::NONE;
	this->m_persistent = false;
	this->m_owner = 0;
	this->m_rev.m_time = 0;
	this->m_rev.m_ownerRev = 0;
	this->m_rev.m_dataRev = 0;
	this->m_pgwVersion = 0;
	this->m_distant = false;
	//this->m_tempSortValue = 0;
	//this->m_tempHaveRevision = false;
	this->m_prefab = 0;
	this->m_sector = Vector2i::ZERO;
	this->m_position = Vector3::ZERO;
	this->m_rotation = Quaternion::IDENTITY;

	FreeMembers();
}

void ZDO::Serialize(NetPackage& pkg) {
	pkg.Write(m_persistent);
	pkg.Write(m_distant);
	static_assert(sizeof(Rev::m_time) == 8, "Fix this");
	pkg.Write(m_rev.m_time);
	pkg.Write(m_pgwVersion);
	pkg.Write(m_type); // sbyte
	pkg.Write(m_prefab);
	pkg.Write(m_rotation);

	// sections organized like this:
	//	32 bit mask: F V Q I S L A
	//  for each present type in order...

	pkg.Write((int32_t)m_dataMask);

#define TYPE_SERIALIZE(T) \
{ \
	if ((m_dataMask >> static_cast<uint8_t>(GetShift<T>())) & 0b1) { \
		auto size_mark = pkg.m_stream.Position(); \
		uint8_t size = 0; \
		pkg.Write(size); \
		for (auto&& pair : m_members) { \
			if (pair.second.first != GetShift<T>()) \
				continue; \
			size++; \
			pkg.Write(FromShiftHash<T>(pair.first)); \
			pkg.Write(*(T*)pair.second.second); \
		} \
		auto end_mark = pkg.m_stream.Position(); \
		pkg.m_stream.SetPos(size_mark); \
		pkg.Write(size); \
		pkg.m_stream.SetPos(end_mark); \
	} \
}

	TYPE_SERIALIZE(float);
	TYPE_SERIALIZE(Vector3);
	TYPE_SERIALIZE(Quaternion);
	TYPE_SERIALIZE(int32_t);
	TYPE_SERIALIZE(int64_t);
	TYPE_SERIALIZE(std::string);
	TYPE_SERIALIZE(BYTES_t);
}

void ZDO::Deserialize(NetPackage& pkg) {
	// Since the data is arriving from the client, must assert things
	// Filter the client inputs

	this->m_persistent = pkg.Read<bool>();
	this->m_distant = pkg.Read<bool>();
	this->m_rev.m_time = pkg.Read<int64_t>();
	this->m_pgwVersion = pkg.Read<int32_t>();
	this->m_type = pkg.Read<ObjectType>();
	this->m_prefab = pkg.Read<HASH_t>();
	this->m_rotation = pkg.Read<Quaternion>();

	this->m_dataMask = pkg.Read<int32_t>();

#define TYPE_DESERIALIZE(T) \
{ \
	if ((m_dataMask >> static_cast<uint8_t>(GetShift<T>())) & 0b1) { \
		auto count = pkg.Read<uint8_t>(); \
		while (count--) { \
			auto key = pkg.Read<HASH_t>(); \
			_Set(key, pkg.Read<T>()); \
		} \
	} \
}

	TYPE_DESERIALIZE(float);
	TYPE_DESERIALIZE(Vector3);
	TYPE_DESERIALIZE(Quaternion);
	TYPE_DESERIALIZE(int32_t);
	TYPE_DESERIALIZE(int64_t);
	TYPE_DESERIALIZE(std::string);
	TYPE_DESERIALIZE(BYTES_t);
}