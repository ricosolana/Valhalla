#include <functional>

#include "ZDO.h"
//#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "NetID.h"
//#include "ZoneSystem.h"

//constexpr std::pair<HASH_t, HASH_t> ToHashPair(const char* key) {
//    //constexpr auto s = "hello" " " "world";
//
//    return std::make_pair(
//        VUtils::GetStableHashCode(key " "),
//        VUtils::GetStableHashCode(key " ")
//    );
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



void ZDO::Save(NetPackage& pkg) const {
    pkg.Write(this->m_rev.m_ownerRev);  static_assert(sizeof(Rev::m_ownerRev) == 4);
    pkg.Write(this->m_rev.m_dataRev);   static_assert(sizeof(Rev::m_dataRev) == 4);
    pkg.Write(this->m_persistent);      
    pkg.Write(this->m_owner);
    pkg.Write(this->m_rev.m_time);      static_assert(sizeof(Rev::m_time) == 8);
    pkg.Write(this->m_pgwVersion);      static_assert(sizeof(m_pgwVersion) == 4);
    pkg.Write(this->m_type);            static_assert(sizeof(m_type) == 1);
    pkg.Write(this->m_distant);
    pkg.Write(this->m_prefab);
    pkg.Write(this->m_sector);
    pkg.Write(this->m_position);
    pkg.Write(this->m_rotation);
    
    // Save uses 2 bytes for counts (char in c# is 2 bytes..)
    _TryWriteType<float, int16_t>(pkg);
    _TryWriteType<Vector3, int16_t>(pkg);
    _TryWriteType<Quaternion, int16_t>(pkg);
    _TryWriteType<int32_t, int16_t>(pkg);
    _TryWriteType<int64_t, int16_t>(pkg);
    _TryWriteType<std::string, int16_t>(pkg);
    _TryWriteType<BYTES_t, int16_t>(pkg);
}

void ZDO::Load(NetPackage& pkg, int32_t worldVersion) {
    this->m_rev.m_ownerRev = pkg.Read<uint32_t>();
    this->m_rev.m_dataRev = pkg.Read<uint32_t>();
    this->m_persistent = pkg.Read<bool>();
    this->m_owner = pkg.Read<OWNER_t>();
    this->m_rev.m_time = pkg.Read<int64_t>();
    this->m_pgwVersion = pkg.Read<int32_t>();

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        this->m_type = pkg.Read<ObjectType>();

    if (worldVersion >= 22)
        this->m_distant = pkg.Read<bool>();

    if (worldVersion < 13)
        pkg.Read<int16_t>(); // condensed 2 reads

    if (worldVersion >= 17)
        this->m_prefab = pkg.Read<HASH_t>();

    this->m_sector = pkg.Read<Vector2i>();
    this->m_position = pkg.Read<Vector3>();
    this->m_rotation = pkg.Read<Quaternion>();

    // Load uses 2 bytes for counts (char in c# is 2 bytes..)
    // It of course has a weird encoding scheme according to UTF...
    // But values 127 and lower are normal
    _TryReadType<float, int16_t>(pkg);
    _TryReadType<Vector3, int16_t>(pkg);
    _TryReadType<Quaternion, int16_t>(pkg);
    _TryReadType<int32_t, int16_t>(pkg);
    _TryReadType<int64_t, int16_t>(pkg);
    _TryReadType<std::string, int16_t>(pkg);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t, int16_t>(pkg);

    if (worldVersion < 17)
        this->m_prefab = GetInt("prefab", 0);
}



// copy constructor
ZDO::ZDO(const ZDO& other) {
    // Member copy
    this->m_persistent = other.m_persistent;
    this->m_distant = other.m_distant;
    this->m_type = other.m_type;
    this->m_prefab = other.m_prefab;
    this->m_rotation = other.m_rotation;
    //this->m_dataMask = other.m_dataMask;
    this->m_ordinalMask = other.m_ordinalMask;

    this->m_sector = other.m_sector;
    this->m_position = other.m_position;
    this->m_id = other.m_id;
    this->m_owner = other.m_owner;

    this->m_rev = other.m_rev;
    this->m_pgwVersion = other.m_pgwVersion;

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
*         Hash Getters
*
*
*/

// Trivial

float ZDO::GetFloat(HASH_t key, float value) const {
    return Get<float>(key, value);
}

int32_t ZDO::GetInt(HASH_t key, int32_t value) const {
    return Get<int32_t>(key, value);
}

int64_t ZDO::GetLong(HASH_t key, int64_t value) const {
    return Get<int64_t>(key, value);
}

const Quaternion& ZDO::GetQuaternion(HASH_t key, const Quaternion& value) const {
    return Get<Quaternion>(key, value);
}

const Vector3& ZDO::GetVector3(HASH_t key, const Vector3& value) const {
    return Get<Vector3>(key, value);
}

const std::string& ZDO::GetString(HASH_t key, const std::string& value) const {
    return Get<std::string>(key, value);
}

const BYTES_t* ZDO::GetBytes(HASH_t key) const {
    return _Get<BYTES_t>(key);
}

// Special

bool ZDO::GetBool(HASH_t key, bool value) const {
    return GetInt(key, value ? 1 : 0);
}

NetID ZDO::GetNetID(const std::pair<HASH_t, HASH_t>& key) const {
    auto k = GetLong(key.first);
    auto v = GetLong(key.second);
    if (k == 0 || v == 0)
        return NetID::NONE;
    return NetID(k, (uint32_t)v);
}



/*
*
*
*         String Getters
*
*
*/

// Trivial

float ZDO::GetFloat(const std::string& key, float value) const {
    return GetFloat(VUtils::String::GetStableHashCode(key), value);
}

int32_t ZDO::GetInt(const std::string& key, int32_t value) const {
    return GetInt(VUtils::String::GetStableHashCode(key), value);
}

int64_t ZDO::GetLong(const std::string& key, int64_t value) const {
    return GetLong(VUtils::String::GetStableHashCode(key), value);
}

const Quaternion& ZDO::GetQuaternion(const std::string& key, const Quaternion& value) const {
    return GetQuaternion(VUtils::String::GetStableHashCode(key), value);
}

const Vector3& ZDO::GetVector3(const std::string& key, const Vector3& value) const {
    return GetVector3(VUtils::String::GetStableHashCode(key), value);
}

const std::string& ZDO::GetString(const std::string& key, const std::string& value) const {
    return GetString(VUtils::String::GetStableHashCode(key), value);
}

const BYTES_t* ZDO::GetBytes(const std::string& key) {
    return GetBytes(VUtils::String::GetStableHashCode(key));
}

// Special

bool ZDO::GetBool(const std::string& key, bool value) const {
    return GetBool(VUtils::String::GetStableHashCode(key), value);
}

NetID ZDO::GetNetID(const std::string& key) const {
    return GetNetID(ToHashPair(key));
}



/*
*
*
*         Hash Setters
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



bool ZDO::Local() const {
    return m_owner == Valhalla()->ID();
}

bool ZDO::SetLocal() {
    if (Local())
        return false;
    return SetOwner(Valhalla()->ID());
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
            case ORD_FLOAT:        delete (float*)         pair.second; break;
            case ORD_VECTOR3:      delete (Vector3*)       pair.second; break;
            case ORD_QUATERNION:   delete (Quaternion*)    pair.second; break;
            case ORD_INT:          delete (int32_t*)       pair.second; break;
            case ORD_LONG:         delete (int64_t*)       pair.second; break;
            case ORD_STRING:       delete (std::string*)   pair.second; break;
            case ORD_ARRAY:        delete (BYTES_t*)       pair.second; break;
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

void ZDO::Serialize(NetPackage& pkg) const {
    pkg.Write(m_persistent);
    pkg.Write(m_distant);
    static_assert(sizeof(Rev::m_time) == 8);
    pkg.Write(m_rev.m_time);
    pkg.Write(m_pgwVersion);
    static_assert(sizeof(m_type) == 1);
    pkg.Write(m_type); // sbyte
    pkg.Write(m_prefab);
    pkg.Write(m_rotation);

    // sections organized like this:
    //    32 bit mask: F V Q I S L A
    //  for each present type in order...
    
    // Writing a signed/unsigned mask doesnt matter
    //  same when positive (for both)
    pkg.Write((int32_t) m_ordinalMask);

    _TryWriteType<float,            int8_t>(pkg);
    _TryWriteType<Vector3,          int8_t>(pkg);
    _TryWriteType<Quaternion,       int8_t>(pkg);
    _TryWriteType<int32_t,          int8_t>(pkg);
    _TryWriteType<int64_t,          int8_t>(pkg);
    _TryWriteType<std::string,      int8_t>(pkg);
    _TryWriteType<BYTES_t,          int8_t>(pkg);
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
    
    //this->m_ordinalMask = (uint8_t) pkg.Read<int32_t>();

    this->m_ordinalMask = (Ordinal) pkg.Read<int32_t>();

    // double check this; 
    if (m_ordinalMask & GetOrdinalMask<float>())
        _TryReadType<float,         int8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<Vector3>())
        _TryReadType<Vector3,       int8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<Quaternion>())
        _TryReadType<Quaternion,    int8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<int32_t>())
        _TryReadType<int32_t,       int8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<int64_t>())
        _TryReadType<int64_t,       int8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<std::string>())
        _TryReadType<std::string,   int8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<BYTES_t>())
        _TryReadType<BYTES_t,       int8_t>(pkg);
}