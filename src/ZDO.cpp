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

bool ZDO::Load31Pre(DataReader& pkg, int32_t worldVersion) {
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

bool ZDO::LoadPost31(DataReader& reader, int32_t version) {
    // The (premature) optimizations I tried to 
    //  implement never went anywhere because
    //  I never knew what I was doing and 
    //  it seemed a bit like overkill
    // -------------------
    // Some stuff I tried implementing:
    //  Shrinking ZDOs as much as possible
    //      encoding ZDOID to save memory (over time I made changes below:)
    //          i64, i32 (plus i32 for padding) = 128 bits
    //          i64, i32 packed = 92 bits
    //          finally u64 by taking advantage of AssemblyUtils GenerateUID algorithm specs regarding summed/set unioned integer range
    //      Valheim now includes encoded ZDOIDs, but differently:
    //          One consolidated ZDOID-UserID array for ZDOs to reference
    //          Returns a u16 index for ZDOs to refer to
    //          ZDOs still have a u32 member
    //      This makes little difference in C++ because padding will prevent memory preservation
    //          I do not know whether C# class structures contain padding or what, so I cant comment on this
    //          nvm actually I see '[StructLayout(0, Pack = 1)]'

    // Set the self incremental id (ZDOID is no longer saved to disk)
    this->m_id.SetUID(ZDOManager()->m_nextUid++);
    auto mask = reader.Read<uint16_t>();
    reader.Read<Vector2s>(); // lol why is sector still being saved, kinda redudant given all the other insane optimizations...
    this->m_pos = reader.Read<Vector3f>();
    m_prefab = PrefabManager()->RequirePrefab(reader.Read<HASH_t>());
    if (mask & 4096) this->m_rotation = reader.Read<Quaternion>();
    //if ((mask & 255) == 0) return;
    
    if (mask & 1) {
        reader.Read<ZDOConnector::Type>();
        auto hash = reader.Read<HASH_t>();
    }

    if (mask & 2) {
        auto count = reader.Read<uint8_t>();
    }
}

// maybe rename unpack?
ZDOConnector::Type ZDO::LoadFrom(DataReader& reader, int32_t version) {
    // Set the self incremental id (ZDOID is no longer saved to disk)
    this->m_id.SetUID(ZDOManager()->m_nextUid++);

    auto mask = reader.Read<uint16_t>();
    if (version)
        reader.Read<Vector2s>(); // lol why is sector still being saved, kinda redudant given all the other insane optimizations...
    this->m_pos = reader.Read<Vector3f>();
    m_prefab = PrefabManager()->RequirePrefab(reader.Read<HASH_t>());
    if (mask & 4096) this->m_rotation = reader.Read<Quaternion>();
    //if ((mask & 255) == 0) return;

    if (mask & 1) {
        reader.Read<ZDOConnector::Type>();
        auto hash = reader.Read<HASH_t>();
    }

    //static constexpr auto sizess = sizeof(std::bitset<24>)

    if (mask & 2) {
        auto count = reader.Read<uint8_t>();
    }
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
