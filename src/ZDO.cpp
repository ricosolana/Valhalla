#include <functional>

#include "ZDO.h"
#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "ZDOID.h"
#include "PrefabManager.h"
#include "Prefab.h"
#include "ZoneManager.h"
#include "NetManager.h"

std::pair<HASH_t, HASH_t> ZDO::ToHashPair(const std::string& key) {
    return {
        VUtils::String::GetStableHashCode(key + "_u"),
        VUtils::String::GetStableHashCode(key + "_i") 
    };
}

//ZDO::ZDO(const ZDOID& id, const Vector3& pos)
//    : m_id(id), m_pos(pos) {
//}

//ZDO::ZDO(const ZDOID& id, const Vector3& pos, DataReader& load)
//    : m_id(id), m_pos(pos), Load() {
//}

//ZDO::ZDO(const ZDOID& id, const Vector3& pos, DataReader& deserialize, uint32_t ownerRev, uint32_t dataRev) {

//}

//ZDO::Rev::Rev(const Peer::Rev& rev) 
//    : m_dataRev(rev.m_dataRev), m_ownerRev(rev.m_ownerRev) {}
//


void ZDO::Save(DataWriter& pkg) const {
    pkg.Write(this->m_rev.m_ownerRev);
    pkg.Write(this->m_rev.m_dataRev);

    pkg.Write(this->m_prefab->FlagsPresent(Prefab::Flag::Persistent));

    pkg.Write<OWNER_t>(0);

    pkg.Write<int64_t>(this->m_timeCreated.count());
    pkg.Write(VConstants::PGW);

    pkg.Write(this->m_prefab->m_type);
    pkg.Write(this->m_prefab->FlagsPresent(Prefab::Flag::Persistent));
    pkg.Write(this->m_prefab->m_hash);

    pkg.Write(this->Sector());              //pkg.Write(IZoneManager::WorldToZonePos(this->m_pos));
    pkg.Write(this->m_pos);
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
    //this->m_rev.m_ownerRev = pkg.Read<uint32_t>();  // TODO this isnt necessary?
    //this->m_rev.m_dataRev = pkg.Read<uint32_t>();   // TODO this isnt necessary?
    pkg.Read<uint32_t>(); // owner rev
    pkg.Read<uint32_t>(); // data rev
    pkg.Read<bool>(); //this->m_persistent
    pkg.Read<OWNER_t>(); // unused owner
    this->m_timeCreated = TICKS_t(pkg.Read<int64_t>()); // Only needed for Terrain, which is wasteful for 99% of zdos
    bool modern = pkg.Read<int32_t>() == VConstants::PGW;

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        pkg.Read<ObjectType>(); // m_type

    if (worldVersion >= 22)
        pkg.Read<bool>(); // m_distant

    if (worldVersion < 13) {
        pkg.ReadChar();
        pkg.ReadChar();
    }

    if (worldVersion >= 17)
        this->m_prefab = &PrefabManager()->RequirePrefab(pkg.Read<HASH_t>());

    pkg.Read<Vector2i>(); // m_sector
    this->m_pos = pkg.Read<Vector3>();
    this->m_rotation = pkg.Read<Quaternion>();

    _TryReadType<float,         uint16_t>(pkg);
    _TryReadType<Vector3,       uint16_t>(pkg);
    _TryReadType<Quaternion,    uint16_t>(pkg);
    _TryReadType<int32_t,       uint16_t>(pkg);
    _TryReadType<int64_t,       uint16_t>(pkg);
    _TryReadType<std::string,   uint16_t>(pkg);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t, int16_t>(pkg);

    if (worldVersion < 17)
        this->m_prefab = &PrefabManager()->RequirePrefab(pkg.Read<HASH_t>());

    //if (m_prefab->FlagsPresent(Prefab::Flag::TerrainModifier)) {
    //    ZDOManager()->m_terrainModifiers[m_id] = timeCreated;
    //}

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
    return Get<BYTES_t>(key);
}

// Special

bool ZDO::GetBool(HASH_t key, bool value) const {
    return GetInt(key, value ? 1 : 0);
}

ZDOID ZDO::GetNetID(const std::pair<HASH_t, HASH_t>& key) const {
    auto k = GetLong(key.first, 0);
    auto v = GetLong(key.second, 0);

    return ZDOID(k, v);
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
    return Get<float>(key, value);
}

int32_t ZDO::GetInt(const std::string& key, int32_t value) const {
    return Get<int32_t>(key, value);
}

int64_t ZDO::GetLong(const std::string& key, int64_t value) const {
    return Get<int64_t>(key, value);
}

const Quaternion& ZDO::GetQuaternion(const std::string& key, const Quaternion& value) const {
    return Get<Quaternion>(VUtils::String::GetStableHashCode(key), value);
}

const Vector3& ZDO::GetVector3(const std::string& key, const Vector3& value) const {
    return Get<Vector3>(VUtils::String::GetStableHashCode(key), value);
}

const std::string& ZDO::GetString(const std::string& key, const std::string& value) const {
    return Get<std::string>(key, value);
}

const BYTES_t* ZDO::GetBytes(const std::string& key) {
    return Get<BYTES_t>(key);
}

// Special

bool ZDO::GetBool(const std::string& key, bool value) const {
    return Get<int32_t>(key, value);
}

ZDOID ZDO::GetNetID(const std::string& key) const {
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

void ZDO::Set(const std::pair<HASH_t, HASH_t>& key, const ZDOID& value) {
    Set(key.first, value.m_uuid);
    Set(key.second, (int64_t)value.m_id);
}



void ZDO::SetPosition(const Vector3& pos) {
    if (this->m_pos != pos) {
        ZDOManager()->InvalidateSector(*this);
        this->m_pos = pos;
        ZDOManager()->AddToSector(*this);

        if (Local())
            Revise();
    }
}

ZoneID ZDO::Sector() const {
    return IZoneManager::WorldToZonePos(m_pos);
}

void ZDO::Serialize(DataWriter& pkg) const {
    pkg.Write(m_prefab->FlagsPresent(Prefab::Flag::Persistent));
    pkg.Write(m_prefab->FlagsPresent(Prefab::Flag::Distant));

    pkg.Write<int64_t>(m_timeCreated.count());
    
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
    static_assert(sizeof(ObjectType) == 1);

    pkg.Read<bool>();       // m_persistent
    pkg.Read<bool>();       // m_distant

    m_timeCreated = TICKS_t(pkg.Read<int64_t>());

    pkg.Read<int32_t>();    // m_pgwVersion
    pkg.Read<ObjectType>(); // this->m_type

    HASH_t prefabHash = pkg.Read<HASH_t>();
    if (!this->m_prefab)
        this->m_prefab = &PrefabManager()->RequirePrefab(prefabHash);

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
