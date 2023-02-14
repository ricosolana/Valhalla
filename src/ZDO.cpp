#include <functional>

#include "ZDO.h"
#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "NetID.h"
#include "PrefabManager.h"
#include "Prefab.h"
#include "ZoneManager.h"
#include "NetManager.h"

std::pair<HASH_t, HASH_t> ZDO::ToHashPair(const std::string& key) {
    return std::make_pair(
        VUtils::String::GetStableHashCode(std::string(key + "_u")),
        VUtils::String::GetStableHashCode(std::string(key + "_i"))
    );
}

ZDO::ZDO(const NetID& id, const Vector3& pos)
    : m_id(id), m_position(pos) {
}

void ZDO::Save(DataWriter& pkg) const {
    pkg.Write(this->m_rev.m_ownerRev);
    pkg.Write(this->m_rev.m_dataRev);
    pkg.Write(this->m_prefab->m_persistent);      
    pkg.Write<OWNER_t>(0); //pkg.Write(this->m_owner);
    pkg.Write(this->m_rev.m_ticks.count());
    pkg.Write(VConstants::PGW);
    pkg.Write(this->m_prefab->m_type);
    pkg.Write(this->m_prefab->m_distant);
    pkg.Write(this->m_prefab->m_hash);
    pkg.Write(this->Sector());              //pkg.Write(IZoneManager::WorldToZonePos(this->m_position));
    pkg.Write(this->m_position);
    pkg.Write(this->m_rotation);
    
    // Save uses 2 bytes for counts (char in c# is 2 bytes..)
    _TryWriteType<float,        uint16_t>(pkg);
    _TryWriteType<Vector3,      uint16_t>(pkg);
    _TryWriteType<Quaternion,   uint16_t>(pkg);
    _TryWriteType<int32_t,      uint16_t>(pkg);
    _TryWriteType<int64_t,      uint16_t>(pkg);
    _TryWriteType<std::string,  uint16_t>(pkg);
    _TryWriteType<BYTES_t,      uint16_t>(pkg);
}

bool ZDO::Load(DataReader& pkg, int32_t worldVersion) {
    this->m_rev.m_ownerRev = pkg.Read<uint32_t>();  // TODO this isnt necessary?
    this->m_rev.m_dataRev = pkg.Read<uint32_t>();   // TODO this isnt necessary?
    pkg.Read<bool>(); //this->m_persistent
    //this->m_owner = pkg.Read<OWNER_t>();
    pkg.Read<OWNER_t>(); // unused owner
    this->m_rev.m_time = pkg.Read<int64_t>();
    bool modern = pkg.Read<int32_t>() == VConstants::PGW;

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        pkg.Read<ObjectType>(); //this->m_type

    if (worldVersion >= 22)
        pkg.Read<bool>(); //this->m_distant

    if (worldVersion < 13) {
        //assert(false, "c# char is utf8, NYI");
        //pkg.Read<int16_t>(); // condensed 2 reads
        pkg.ReadChar();
        pkg.ReadChar();
    }

    if (worldVersion >= 17)
        if (!(this->m_prefab = PrefabManager()->GetPrefab(pkg.Read<HASH_t>())))
            throw std::runtime_error("unknown zdo prefab");

    /*this->m_sector = */ pkg.Read<Vector2i>();
    this->m_position = pkg.Read<Vector3>();
    this->m_rotation = pkg.Read<Quaternion>();

    //AddToSector(this);

    // Load uses 2 bytes for counts (char in c# is 2 bytes..)
    // It of course has a weird encoding scheme according to UTF...
    // But values 127 and lower are normal
    _TryReadType<float,         uint16_t>(pkg);
    _TryReadType<Vector3,       uint16_t>(pkg);
    _TryReadType<Quaternion,    uint16_t>(pkg);
    _TryReadType<int32_t,       uint16_t>(pkg);
    _TryReadType<int64_t,       uint16_t>(pkg);
    _TryReadType<std::string,   uint16_t>(pkg);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t, int16_t>(pkg);

    if (worldVersion < 17)
        if (!(this->m_prefab = PrefabManager()->GetPrefab(GetInt("prefab", 0))))
            throw std::runtime_error("unknown zdo prefab");

    return modern;
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
    auto k = GetLong(key.first, 0);
    auto v = GetLong(key.second, 0);

    return NetID(k, v);
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
        ZDOManager()->InvalidateSector(this);
        this->m_position = pos;
        ZDOManager()->AddToSector(this);

        if (Local())
            Revise();
    }
}

ZoneID ZDO::Sector() const {
    return IZoneManager::WorldToZonePos(m_position);
}

void ZDO::Serialize(DataWriter& pkg) const {
    static_assert(sizeof(std::remove_pointer_t<decltype(m_prefab)>::m_persistent) == 1);
    static_assert(sizeof(std::remove_pointer_t<decltype(m_prefab)>::m_distant) == 1);
    static_assert(sizeof(VConstants::PGW) == 4);
    static_assert(sizeof(Rev::m_ticks) == 8);

    assert(m_prefab);
    pkg.Write(m_prefab->m_persistent);
    pkg.Write(m_prefab->m_distant);
    pkg.Write(m_rev.m_ticks.count());
    pkg.Write(VConstants::PGW); // pkg.Write(m_pgwVersion);
    pkg.Write(m_prefab->m_type); // sbyte
    pkg.Write(m_prefab->m_hash);
    pkg.Write(m_rotation);

    // sections organized like this:
    //    32 bit mask: F V Q I S L A
    //  for each present type in order...
    
    // Writing a signed/unsigned mask doesnt matter
    //  same when positive (for both)
    pkg.Write((int32_t) m_ordinalMask);

    _TryWriteType<float,            uint8_t>(pkg);
    _TryWriteType<Vector3,          uint8_t>(pkg);
    _TryWriteType<Quaternion,       uint8_t>(pkg);
    _TryWriteType<int32_t,          uint8_t>(pkg);
    _TryWriteType<int64_t,          uint8_t>(pkg);
    _TryWriteType<std::string,      uint8_t>(pkg);
    _TryWriteType<BYTES_t,          uint8_t>(pkg);
}

void ZDO::Deserialize(DataReader& pkg) {
    // Since the data is arriving from the client, must assert things
    // Filter the client inputs

    static_assert(sizeof(ObjectType) == 1);

    static constexpr size_t sz = sizeof(ZDO);

    pkg.Read<bool>();       // m_persistent
    pkg.Read<bool>();       // m_distant
    this->m_rev.m_ticks = TICKS_t(pkg.Read<int64_t>());
    pkg.Read<int32_t>();    // m_pgwVersion
    pkg.Read<ObjectType>(); // this->m_type
    HASH_t prefabHash = pkg.Read<HASH_t>();
    if (!m_prefab && !(this->m_prefab = PrefabManager()->GetPrefab(prefabHash))) {
        // If the prefab is initially being assigned and the prefab entry does not exist, throw
        //  (client exception, since clients fault)
        assert(!m_prefab);
        throw VUtils::data_error("zdo unknown prefab hash");
    }

    assert(m_prefab);

    this->m_rotation = pkg.Read<Quaternion>();    
    this->m_ordinalMask = (Ordinal) pkg.Read<int32_t>();

    // double check this; 
    if (m_ordinalMask & GetOrdinalMask<float>())
        _TryReadType<float,         uint8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<Vector3>())
        _TryReadType<Vector3,       uint8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<Quaternion>())
        _TryReadType<Quaternion,    uint8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<int32_t>())
        _TryReadType<int32_t,       uint8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<int64_t>())
        _TryReadType<int64_t,       uint8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<std::string>())
        _TryReadType<std::string,   uint8_t>(pkg);
    if (m_ordinalMask & GetOrdinalMask<BYTES_t>())
        _TryReadType<BYTES_t,       uint8_t>(pkg);
}
