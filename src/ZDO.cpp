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

void ZDO::Save(DataWriter& pkg) const {
    pkg.Write(this->m_rev.m_ownerRev);
    pkg.Write(this->m_rev.m_dataRev);

    pkg.Write(this->m_prefab->FlagsAbsent(Prefab::Flags::Sessioned));

    pkg.Write<OWNER_t>(0); //pkg.Write(this->m_owner);
    pkg.Write(this->m_rev.m_ticksCreated.count());
    pkg.Write(VConstants::PGW);

    pkg.Write(this->m_prefab->m_type);
    pkg.Write(this->m_prefab->FlagsPresent(Prefab::Flags::Distant));
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
    this->m_rev.m_ownerRev = pkg.Read<uint32_t>();  // TODO this isnt necessary?
    this->m_rev.m_dataRev = pkg.Read<uint32_t>();   // TODO this isnt necessary?
    pkg.Read<bool>(); //this->m_persistent
    //this->m_owner = pkg.Read<OWNER_t>();
    pkg.Read<OWNER_t>(); // unused owner
    this->m_rev.m_ticksCreated = TICKS_t(pkg.Read<int64_t>());
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
        if (!(this->m_prefab = PrefabManager()->GetPrefab(GetInt("prefab", 0))))
            throw std::runtime_error("unknown zdo prefab");

    return modern;
}




// ZDO specific-methods

void ZDO::SetPosition(const Vector3& pos) {
    if (m_pos != pos) {
        ZDOManager()->InvalidateSector(*this);
        this->m_pos = pos;
        ZDOManager()->AddToSector(*this);

        if (IsLocal())
            Revise();
    }
}

ZoneID ZDO::Sector() const {
    return IZoneManager::WorldToZonePos(m_pos);
}

void ZDO::Serialize(DataWriter& pkg) const {
    //static_assert(sizeof(std::remove_pointer_t<decltype(m_prefab)>::m_persistent) == 1);
    //static_assert(sizeof(std::remove_pointer_t<decltype(m_prefab)>::m_distant) == 1);
    static_assert(sizeof(VConstants::PGW) == 4);
    static_assert(sizeof(Rev::m_ticksCreated) == 8);
    
    pkg.Write(m_prefab->FlagsAbsent(Prefab::Flags::Sessioned));
    pkg.Write(m_prefab->FlagsPresent(Prefab::Flags::Distant));

    pkg.Write(m_rev.m_ticksCreated.count());
    pkg.Write(VConstants::PGW);

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
    this->m_rev.m_ticksCreated = TICKS_t(pkg.Read<int64_t>());
    pkg.Read<int32_t>();    // m_pgwVersion
    pkg.Read<ObjectType>(); // this->m_type
    HASH_t prefabHash = pkg.Read<HASH_t>();
    if (!m_prefab)
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
