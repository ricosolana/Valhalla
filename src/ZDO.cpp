#include <functional>

#include "ZDO.h"
#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "ZDOID.h"
#include "PrefabManager.h"
#include "Prefab.h"
#include "ZoneManager.h"
#include "NetManager.h"
#include "VUtilsResource.h"



ZDO::ZDO() 
    : m_prefab(Prefab::NONE) {

}

ZDO::ZDO(ZDOID id, Vector3f pos)
    : m_id(id), m_pos(pos), m_prefab(Prefab::NONE)
{
    //m_rev.m_ticksCreated = Valhalla()->GetWorldTicks();
}

//ZDO::ZDO(const ZDOID& id, const Vector3f& pos, HASH_t prefab)
//    : m_id(id), m_pos(pos), m_prefab(PrefabManager()->RequirePrefab(prefab))
//{
//    m_rev.m_ticksCreated = Valhalla()->GetWorldTicks();
//}

void ZDO::Save(DataWriter& pkg) const {
    auto&& prefab = GetPrefab();

    pkg.Write(this->GetOwnerRevision());
    pkg.Write(this->m_dataRev);

    pkg.Write(prefab.AnyFlagsAbsent(Prefab::Flag::SESSIONED));

    pkg.Write<OWNER_t>(0); //pkg.Write(this->m_owner);
    //pkg.Write(this->m_rev.m_ticksCreated.count());
    pkg.Write(GetTimeCreated().count());
    pkg.Write(VConstants::PGW);

    pkg.Write(prefab.m_type);
    pkg.Write(prefab.AllFlagsPresent(Prefab::Flag::DISTANT));
    pkg.Write(prefab.m_hash);

    pkg.Write(this->GetZone());              //pkg.Write(IZoneManager::WorldToZonePos(this->m_pos));
    pkg.Write(this->m_pos);
    pkg.Write(this->m_rotation);
    
    // Save uses 2 bytes for counts (char in c# is 2 bytes..)
    _TryWriteType<float,            char16_t>(pkg);
    _TryWriteType<Vector3f,         char16_t>(pkg);
    _TryWriteType<Quaternion,       char16_t>(pkg);
    _TryWriteType<int32_t,          char16_t>(pkg);
    _TryWriteType<int64_t,          char16_t>(pkg);
    _TryWriteType<std::string,      char16_t>(pkg);
    _TryWriteType<BYTES_t,          char16_t>(pkg);
}

bool ZDO::Load(DataReader& pkg, int32_t worldVersion) {
    this->SetOwnerRevision(pkg.Read<uint32_t>());   // ownerRev; TODO redundant?
    this->m_dataRev = pkg.Read<uint32_t>();   // dataRev; TODO redundant?
    pkg.Read<bool>();       // persistent
    pkg.Read<OWNER_t>();    // owner
    auto timeCreated = TICKS_t(pkg.Read<int64_t>());
    bool modern = pkg.Read<int32_t>() == VConstants::PGW;

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        pkg.Read<Prefab::Type>(); // m_type

    if (worldVersion >= 22)
        pkg.Read<bool>(); // m_distant

    if (worldVersion < 13) {
        pkg.Read<char16_t>();
        pkg.Read<char16_t>();
    }

    if (worldVersion >= 17)
        this->m_prefab = PrefabManager()->RequirePrefab(pkg.Read<HASH_t>());

    pkg.Read<ZoneID>(); // m_sector
    this->m_pos = pkg.Read<Vector3f>();
    this->m_rotation = pkg.Read<Quaternion>();

    _TryReadType<float,         char16_t>(pkg);
    _TryReadType<Vector3f,      char16_t>(pkg);
    _TryReadType<Quaternion,    char16_t>(pkg);
    _TryReadType<int32_t,       char16_t>(pkg);
    _TryReadType<int64_t,       char16_t>(pkg);
    _TryReadType<std::string,   char16_t>(pkg);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t,   char16_t>(pkg);

    if (worldVersion < 17)
        this->m_prefab = PrefabManager()->RequirePrefab(GetInt("prefab"));

    SetTimeCreated(timeCreated);

    return modern;
}




// ZDO specific-methods

void ZDO::SetPosition(const Vector3f& pos) {
    if (m_pos != pos) {
        ZDOManager()->InvalidateZDOZone(*this);
        this->m_pos = pos;
        ZDOManager()->AddZDOToZone(*this);

        if (IsLocal())
            Revise();
    }
}

ZoneID ZDO::GetZone() const {
    return IZoneManager::WorldToZonePos(m_pos);
}

void ZDO::Serialize(DataWriter& pkg) const {
    static_assert(sizeof(VConstants::PGW) == 4);

    auto&& prefab = GetPrefab();
    
    pkg.Write(prefab.AnyFlagsAbsent(Prefab::Flag::SESSIONED));
    pkg.Write(prefab.AllFlagsPresent(Prefab::Flag::DISTANT));

    pkg.Write(GetTimeCreated().count());
    //pkg.Write(m_rev.m_ticksCreated.count());
    pkg.Write(VConstants::PGW);

    pkg.Write(prefab.m_type); // sbyte
    pkg.Write(prefab.m_hash);

    pkg.Write(m_rotation);

    // sections organized like this:
    //    32 bit mask: F V Q I S L A
    //  for each present type in order...
    
    // Writing a signed/unsigned mask doesnt matter
    //  same when positive (for both)
    auto ordinalMask = GetOrdinalMask();
    if (ordinalMask & GetOrdinalMask<BYTES_t>()) {
        ordinalMask &= ~GetOrdinalMask<BYTES_t>();
        ordinalMask |= 0b10000000;
    }
    pkg.Write(static_cast<int32_t>(ordinalMask));

    _TryWriteType<float,            uint8_t>(pkg);
    _TryWriteType<Vector3f,         uint8_t>(pkg);
    _TryWriteType<Quaternion,       uint8_t>(pkg);
    _TryWriteType<int32_t,          uint8_t>(pkg);
    _TryWriteType<int64_t,          uint8_t>(pkg);
    _TryWriteType<std::string,      uint8_t>(pkg);
    _TryWriteType<BYTES_t,          uint8_t>(pkg);
}

void ZDO::Deserialize(DataReader& pkg) {
    static_assert(sizeof(Prefab::Type) == 1);
    
    pkg.Read<bool>();       // m_persistent
    pkg.Read<bool>();       // m_distant
    //this->m_rev.m_ticksCreated = TICKS_t(pkg.Read<int64_t>());
    auto timeCreated = TICKS_t(pkg.Read<int64_t>());
    pkg.Read<int32_t>();    // m_pgwVersion
    pkg.Read<Prefab::Type>(); // this->m_type
    auto prefabHash = pkg.Read<HASH_t>();
    if (!m_prefab.get()) {
        this->m_prefab = PrefabManager()->RequirePrefab(prefabHash);
        SetTimeCreated(timeCreated);
    }
    
    this->m_rotation = pkg.Read<Quaternion>();

    auto ordinalMask = static_cast<Ordinal>(pkg.Read<int32_t>());
    if (ordinalMask & 0b10000000) {
        ordinalMask &= 0b01111111; // Clear the top array bit
        ordinalMask |= GetOrdinalMask<BYTES_t>(); // reassign the array bit to 5th
    }

    this->SetOrdinalMask(ordinalMask);

    // double check this; 
    if (ordinalMask & GetOrdinalMask<float>())
        _TryReadType<float,         uint8_t>(pkg);
    if (ordinalMask & GetOrdinalMask<Vector3f>())
        _TryReadType<Vector3f,      uint8_t>(pkg);
    if (ordinalMask & GetOrdinalMask<Quaternion>())
        _TryReadType<Quaternion,    uint8_t>(pkg);
    if (ordinalMask & GetOrdinalMask<int32_t>())
        _TryReadType<int32_t,       uint8_t>(pkg);
    if (ordinalMask & GetOrdinalMask<int64_t>())
        _TryReadType<int64_t,       uint8_t>(pkg);
    if (ordinalMask & GetOrdinalMask<std::string>())
        _TryReadType<std::string,   uint8_t>(pkg);
    if (ordinalMask & GetOrdinalMask<BYTES_t>())
        _TryReadType<BYTES_t,       uint8_t>(pkg);
}
