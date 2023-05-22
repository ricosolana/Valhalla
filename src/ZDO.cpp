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



decltype(ZDO::ZDO_TYPES) ZDO::ZDO_TYPES;
decltype(ZDO::ZDO_CONNECTORS) ZDO::ZDO_CONNECTORS;
decltype(ZDO::ZDO_OWNERS) ZDO::ZDO_OWNERS;
//decltype(ZDO::ZDO_AGES) ZDO::ZDO_AGES;

ZDO::ZDO() 
    : m_prefab(Prefab::NONE) {

}

ZDO::ZDO(ZDOID id, Vector3f pos)
    : m_id(id), m_pos(pos), m_prefab(Prefab::NONE)
{
    //m_rev.m_ticksCreated = Valhalla()->GetWorldTicks();
}



void ZDO::Load31Pre(DataReader& pkg, int32_t worldVersion) {
    pkg.Read<uint32_t>();   // owner rev
    pkg.Read<uint32_t>();   // data rev
    m_flags |= pkg.Read<bool>() ? Flag::Marker_Persistent : (Flag)0;       // persistent
    pkg.Read<int64_t>();    // owner
    auto timeCreated = pkg.Read<int64_t>();
    pkg.Read<int32_t>();    // pgw

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        m_flags |= pkg.Read<uint8_t>() << Data::Marker_Type1; // m_type

    if (worldVersion >= 22)
        m_flags |= pkg.Read<bool>() ? Flag::Marker_Distant : (Flag)0;   // m_distant

    if (worldVersion < 13) {
        pkg.Read<char16_t>();
        pkg.Read<char16_t>();
    }

    if (worldVersion >= 17)
        this->m_prefab = PrefabManager()->RequirePrefab(pkg.Read<HASH_t>());

    pkg.Read<Vector2i>(); // m_sector
    this->m_pos = pkg.Read<Vector3f>();
    this->m_rotation = pkg.Read<Quaternion>().EulerAngles();

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

    if (GetPrefab().AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER))
         Set(Hashes::ZDO::TerrainModifier::TIME_CREATED, timeCreated);
}

ZDOConnector::Type ZDO::Unpack(DataReader& reader, int32_t version) {
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
    if (version)
        this->m_id.SetUID(ZDOManager()->m_nextUid++);

    this->m_flags = reader.Read<uint16_t>();

    if (version) {
        reader.Read<Vector2s>(); // redundant
        this->m_pos = reader.Read<Vector3f>();
    }

    // TODO load prefab once
    m_prefab = PrefabManager()->RequirePrefab(reader.Read<HASH_t>());

    if (_HasData(Data::Marker_Rotation)) this->m_rotation = reader.Read<Vector3f>();

    ZDOConnector::Type type = ZDOConnector::Type::None;
    if (_HasData(Data::Member_Connection)) {
        type = reader.Read<ZDOConnector::Type>();
        if (version) {
            auto hash = reader.Read<HASH_t>();
        }
        else {
            auto zdoid = reader.Read<ZDOID>();
            // set connection
            type &= ~ZDOConnector::Type::Target;
        }        
    }

    if (_HasData(Data::Member_Float)) _TryReadType<float, uint8_t>(reader);
    if (_HasData(Data::Member_Vec3)) _TryReadType<Vector3f, uint8_t>(reader);
    if (_HasData(Data::Member_Quat)) _TryReadType<Quaternion, uint8_t>(reader);
    if (_HasData(Data::Member_Int)) _TryReadType<int32_t, uint8_t>(reader);
    if (_HasData(Data::Member_Long)) _TryReadType<int64_t, uint8_t>(reader);
    if (_HasData(Data::Member_String)) _TryReadType<std::string, uint8_t>(reader);
    if (_HasData(Data::Member_ByteArray)) _TryReadType<BYTES_t, uint8_t>(reader);

    return type;
}



// ZDO specific-methods

void ZDO::SetPosition(Vector3f pos) {
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



void ZDO::Pack(DataWriter& writer, bool network) const {
    writer.Write(m_flags);
    if (!network) {
        writer.Write(GetZone());
        writer.Write(Position());
    }
    writer.Write(GetPrefab().m_hash);
    if (_HasData(Data::Marker_Rotation))
        writer.Write(m_rotation);

    //if (m_flags & (1 << std::to_underlying(Flags::Data_Rotation)))
    if (_HasData(Data::Marker_Rotation))
        writer.Write(this->m_rotation);

    // TODO add connector
    if (_HasData(Data::Member_Connection)) {
        auto&& find = ZDO_CONNECTORS.find(ID());
        if (find != ZDO_CONNECTORS.end()) {
            auto&& connector = find->second;
            writer.Write(connector.m_type);
            writer.Write(connector.m_hash);
        }
        else {
            ///LOG_WARNING(LOGGER, "zdo connector flag is set but no connector found");
            assert(false);
        }
    }

    if (_HasAnyFlags(Flag::Member_Float | Flag::Member_Vec3 | Flag::Member_Quat | Flag::Member_Int | Flag::Member_Long | Flag::Member_String | Flag::Member_ByteArray)) {
        auto&& find = ZDO_TYPES.find(ID());
        if (find != ZDO_TYPES.end()) {
            auto&& types = find->second;

            _TryWriteType<float>(writer, types);
            _TryWriteType<Vector3f>(writer, types);
            _TryWriteType<Quaternion>(writer, types);
            _TryWriteType<int32_t>(writer, types);
            _TryWriteType<int64_t>(writer, types);
            _TryWriteType<std::string>(writer, types);
            _TryWriteType<BYTES_t>(writer, types);
        }
    }
}
