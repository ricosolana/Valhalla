#include <functional>
#include <signal.h>

#include "ZDO.h"
#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "ZDOID.h"
#include "PrefabManager.h"
#include "Prefab.h"
#include "ZoneManager.h"
#include "NetManager.h"
#include "VUtilsResource.h"



//decltype(ZDO::ZDO_MEMBERS) ZDO::ZDO_MEMBERS;
//decltype(ZDO::ZDO_CONNECTORS) ZDO::ZDO_CONNECTORS;
//decltype(ZDO::ZDO_TARGETED_CONNECTORS) ZDO::ZDO_TARGETED_CONNECTORS;
//
//decltype(ZDO::ZDO_INDEXED_OWNERS) ZDO::ZDO_INDEXED_OWNERS;
//decltype(ZDO::HINT_NEXT_EMPTY_INDEX) ZDO::HINT_NEXT_EMPTY_INDEX = 1;
//decltype(ZDO::HINT_LAST_SET_INDEX) ZDO::HINT_LAST_SET_INDEX = 1;

//auto ZDO::ZDO_MEMBERS;
//auto ZDO::ZDO_CONNECTORS;
//auto ZDO::ZDO_TARGETED_CONNECTORS;
//
//auto ZDO::ZDO_INDEXED_OWNERS;
//auto ZDO::HINT_NEXT_EMPTY_INDEX = 1;
//auto ZDO::HINT_LAST_SET_INDEX = 1;

ZDO::ZDO() 
    : ZDO(ZDOID::NONE, Vector3f::Zero()) {}

ZDO::ZDO(ZDOID id, Vector3f pos) : m_id(id), m_pos(pos) {
#if VH_IS_OFF(VH_PORTABLE_ZDOS)
    m_pack.Set<PREFAB_PACK_INDEX>(m_pack.capacity_v<PREFAB_PACK_INDEX>);
#endif
}



//#define VH_ASSERT(cond) if (!(cond)) { DebugBreak(); }
#define VH_ASSERT(cond, msg, ...) if (!(cond)) { LOG_WARNING(LOGGER, msg, __VA_ARGS__); }
//#define VH_ASSERT(cond, ...) {}

#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
void ZDO::Load31Pre(DataReader& pkg, int32_t worldVersion) {
    pkg.Read<uint32_t>();       // owner rev
    pkg.Read<uint32_t>();       // data rev
    {
        auto persistent = pkg.Read<uint8_t>();        // persistent
        VH_ASSERT(persistent <= 0b1, "illegal persistent bool {:#b}", persistent);
#if VH_IS_ON(VH_PORTABLE_ZDOS) 
        this->m_pack.Set<PERSISTENT_PACK_INDEX>(static_cast<bool>(persistent));
#endif 
    }

    pkg.Read<int64_t>();        // owner
    auto timeCreated = pkg.Read<int64_t>();
    pkg.Read<int32_t>();        // pgw
    
    if (worldVersion >= 16 && worldVersion < 24) {
        pkg.Read<int32_t>();
    }

    if (worldVersion >= 23) {
        auto type = pkg.Read<uint8_t>();    // m_type
        VH_ASSERT(type <= 0b11, "illegal type mask {:#b}", type);
        //if (type > 0b11) LOG_BACKTRACE(LOGGER, "Unexpected type value '{}'", type);
#if VH_IS_ON(VH_PORTABLE_ZDOS) 
        this->m_pack.Set<TYPE_PACK_INDEX>((type >> GlobalDenotion::Marker_Type1) 
            & (GlobalDenotion::Marker_Type1 | GlobalDenotion::Marker_Type2));
#endif
    }

    if (worldVersion >= 22) {
        auto distant = pkg.Read<uint8_t>();       // m_distant
        VH_ASSERT(distant <= 0b1, "illegal distant bool {:#b}", distant);
        //if (distant > 0b1) LOG_BACKTRACE(LOGGER, "Unexpected distant value '{}'", distant);
#if VH_IS_ON(VH_PORTABLE_ZDOS) 
        this->m_pack.Set<DISTANT_PACK_INDEX>(static_cast<bool>(distant));
#endif            
    }

    if (worldVersion < 13) {
        pkg.Read<char16_t>();
        pkg.Read<char16_t>();
    }

#if VH_IS_OFF(VH_PORTABLE_ZDOS) && VH_IS_ON(VH_PREFAB_INFO)
    const Prefab* prefab = nullptr;
#endif

    if (worldVersion >= 17) {
        HASH_t prefabHash = pkg.Read<HASH_t>();
#if VH_IS_ON(VH_PORTABLE_ZDOS)
        this->m_prefabHash = prefabHash;
#else
    #if VH_IS_ON(VH_PREFAB_INFO)
        auto&& pair = PrefabManager()->RequirePrefabAndIndexByHash(prefabHash);
        prefab = &pair.first;
        m_pack.Set<PREFAB_PACK_INDEX>(pair.second);
    #else
        _SetPrefabHash(prefabHash);
    #endif
#endif
    }

    pkg.Read<Vector2i>(); // m_sector
    this->m_pos = pkg.Read<Vector3f>();
    this->m_rotation = pkg.Read<Quaternion>().EulerAngles();

    auto&& members = ZDO_MEMBERS[ID()];

    _TryReadType<float,         char16_t>(pkg, members, false);
    _TryReadType<Vector3f,      char16_t>(pkg, members, false);
    _TryReadType<Quaternion,    char16_t>(pkg, members, false);
    _TryReadType<int32_t,       char16_t>(pkg, members, false);
    _TryReadType<int64_t,       char16_t>(pkg, members, false);
    _TryReadType<std::string,   char16_t>(pkg, members, false);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t,   char16_t>(pkg, members, false);

    if (worldVersion < 17) {
        HASH_t prefabHash = GetInt(Hashes::ZDO::ZDO::PREFAB);

#if VH_IS_ON(VH_PORTABLE_ZDOS)
        this->m_prefabHash = prefabHash;
#else // !VH_PORTABLE_ZDOS
    #if VH_IS_ON(VH_PREFAB_INFO)
        auto&& pair = PrefabManager()->RequirePrefabAndIndexByHash(prefabHash);
        prefab = &pair.first;
        m_pack.Set<PREFAB_PACK_INDEX>(pair.second);
    #else // !VH_PREFAB_INFO
        _SetPrefabHash(prefabHash);
    #endif // VH_PREFAB_INFO
#endif // VH_PORTABLE_ZDOS
    }

    // TODO VH_REDUNDANT_ZDOS or not?
#if VH_IS_ON(VH_PREFAB_INFO)

    if (worldVersion < 31) {
        // Convert owners
        {
            auto&& zdoid = GetZDOID("user");
            if (zdoid) {
                SetLocal();
                Set(Hashes::ZDO::USER, zdoid.GetOwner());
            }
        }

        {
            auto&& zdoid = GetZDOID("RodOwner");
            if (zdoid) {
                SetLocal();
                Set(Hashes::ZDO::FishingFloat::ROD_OWNER, zdoid.GetOwner());
            }
        }

        // Convert terrain
#if VH_IS_ON(VH_PORTABLE_ZDOS)
        switch (this->GetPrefabHash()) {
            case Hashes::Object::cultivate:
            case Hashes::Object::raise:
            case Hashes::Object::path:
            case Hashes::Object::paved_road:
            case Hashes::Object::HeathRockPillar:
            case Hashes::Object::HeathRockPillar_frac:
            case Hashes::Object::ship_construction:
            case Hashes::Object::replant:
            case Hashes::Object::digg:
            case Hashes::Object::mud_road:
            case Hashes::Object::LevelTerrain:
            case Hashes::Object::digg_v2:
                Set(Hashes::ZDO::TerrainModifier::TIME_CREATED, timeCreated);
                break;
        }
#else // !VH_PORTABLE_ZDOS
        assert(prefab);
        if (prefab->AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER)
            || (prefab->m_hash == Hashes::Object::ship_construction))
            Set(Hashes::ZDO::TerrainModifier::TIME_CREATED, timeCreated);
#endif // VH_PORTABLE_ZDOS

        // Convert seeds
        {
            auto&& item = Get<int32_t>(Hashes::ZDO::VisEquipment::ITEM_LEFT);
            if (item) {
                Set(Hashes::ZDO::Humanoid::SEED, static_cast<int32_t>(ankerl::unordered_dense::hash<ZDOID>{}(ID())));
            }
        }
    }
#endif // VH_PREFAB_INFO
}
#endif //VH_LEGACY_WORLD_COMPATABILITY



void ZDO::Unpack(DataReader& reader, int32_t version, bool recovery) {
    auto flags = reader.Read<uint16_t>();
    
    //LOG_WARNING(LOGGER, "illegal flag mask {:#b}", flags);

    // Failsafe
    // Whether flags might be invalid
    VH_ASSERT(flags <= 0b1111111111111, "illegal flag mask {:#b}", flags);
    


    if (version) {
        auto sector = reader.Read<Vector2s>(); // redundant
        this->m_pos = reader.Read<Vector3f>();

        // Failsafe
        VH_ASSERT(
            (std::abs(this->m_pos.x) <= 100000.f
                && std::abs(this->m_pos.y) < 10000.f // not sure about dungeons or whatever else
                && std::abs(this->m_pos.z) <= 100000.f),
            "position is too large {}",
            this->m_pos
        );

        VH_ASSERT(GetZone() == sector, "position is outside sector {} != {}", GetZone(), sector);
    }

    // Once initialized info
    const auto prefabHash = reader.Read<HASH_t>();
#if VH_IS_ON(VH_PORTABLE_ZDOS)
    if (this->m_prefabHash == 0) {
        this->m_prefabHash = prefabHash;
        this->m_pack.Set<PERSISTENT_PACK_INDEX>(static_cast<bool>(flags & GlobalFlag::Marker_Persistent));
        this->m_pack.Set<DISTANT_PACK_INDEX>(static_cast<bool>(flags & GlobalFlag::Marker_Distant));
        this->m_pack.Set<TYPE_PACK_INDEX>((flags >> GlobalDenotion::Marker_Type1)
            & (GlobalFlag::Marker_Type1 | GlobalFlag::Marker_Type2));
#else
    if (m_pack.Get<PREFAB_PACK_INDEX>() == m_pack.capacity_v<PREFAB_PACK_INDEX>) {
        _SetPrefabHash(prefabHash);
#endif
    }
    else {
        // should always run if a version is provided (this assumes that the world is being loaded)
        assert(version == 0);
    }
    
    if (flags & GlobalFlag::Marker_Rotation) {
        this->m_rotation = reader.Read<Vector3f>();
    }

    //ZDOConnector::Type type = ZDOConnector::Type::None;
    if (flags & GlobalFlag::Member_Connection) {
        auto type = reader.Read<ZDOConnector::Type>();

        // Failsafe
        VH_ASSERT(std::to_underlying(type) <= 0b11111, "illegal type mask {:#b}", (int)type);

        if (version) {
            auto hash = reader.Read<HASH_t>();
            auto&& connector = ZDO_CONNECTORS[ID()]; // = ZDOConnector{ .m_type = type, .m_hash = hash };
            connector.m_type = type;
            connector.m_hash = hash;
        }
        else {
            auto target = reader.Read<ZDOID>();
            auto&& connector = ZDO_TARGETED_CONNECTORS[ID()];
            // set connection
            connector.m_type = type;
            connector.m_target = target;
        }
    }
    
    if (flags & (GlobalFlag::Member_Float | GlobalFlag::Member_Vec3 | GlobalFlag::Member_Quat | GlobalFlag::Member_Int | GlobalFlag::Member_Long | GlobalFlag::Member_String | GlobalFlag::Member_ByteArray)) {
        // Will insert a default if missing (should be missing already)
        auto&& members = ZDO_MEMBERS[ID()];
        if (flags & GlobalFlag::Member_Float) _TryReadType<float, uint8_t>(reader, members, recovery);
        if (flags & GlobalFlag::Member_Vec3) _TryReadType<Vector3f, uint8_t>(reader, members, recovery);
        if (flags & GlobalFlag::Member_Quat) _TryReadType<Quaternion, uint8_t>(reader, members, recovery);
        if (flags & GlobalFlag::Member_Int) _TryReadType<int32_t, uint8_t>(reader, members, recovery);
        if (flags & GlobalFlag::Member_Long) _TryReadType<int64_t, uint8_t>(reader, members, recovery);
        if (flags & GlobalFlag::Member_String) _TryReadType<std::string, uint8_t>(reader, members, recovery);
        if (flags & GlobalFlag::Member_ByteArray) _TryReadType<BYTES_t, uint8_t>(reader, members, recovery);
    }
}



// ZDO specific-methods

void ZDO::SetPosition(Vector3f pos) {
    if (this->m_pos != pos) {
        if (IZoneManager::WorldToZonePos(pos) == GetZone()) {
            ZDOManager()->_InvalidateZDOZone(*this);
            this->m_pos = pos;
            ZDOManager()->_AddZDOToZone(*this);
        }
        else {
            this->m_pos = pos;
        }

        if (IsLocal())
            Revise();
    }
}

ZoneID ZDO::GetZone() const {
    return IZoneManager::WorldToZonePos(this->m_pos);
}



void ZDO::Pack(DataWriter& writer, bool network) const {
    bool hasRot = std::abs(m_rotation.x) > std::numeric_limits<float>::epsilon() * 8.f
        || std::abs(m_rotation.y) > std::numeric_limits<float>::epsilon() * 8.f
        || std::abs(m_rotation.z) > std::numeric_limits<float>::epsilon() * 8.f;

    uint16_t flags{};

    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Float ? GlobalFlag::Member_Float : (GlobalFlag)0;
    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Vec3 ? GlobalFlag::Member_Vec3 : (GlobalFlag)0;
    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Quat ? GlobalFlag::Member_Quat : (GlobalFlag)0;
    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Int ? GlobalFlag::Member_Int : (GlobalFlag)0;
    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Long ? GlobalFlag::Member_Long : (GlobalFlag)0;
    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_String ? GlobalFlag::Member_String : (GlobalFlag)0;
    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_ByteArray ? GlobalFlag::Member_ByteArray : (GlobalFlag)0;
    //flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Connection ? GlobalFlag::Member_Connection : (GlobalFlag)0;
    if (IsPersistent()) flags |= GlobalFlag::Marker_Persistent;
    if (IsDistant()) flags |= GlobalFlag::Marker_Distant;
    flags |= GetType() << std::to_underlying(GlobalDenotion::Marker_Type1);
    if (hasRot) flags |= GlobalFlag::Marker_Rotation;

    const auto flagPos = writer.Position();
    //writer.Skip(sizeof(flags));
    writer.Write(flags);
        
    if (!network) {
        writer.Write(GetZone());
        writer.Write(GetPosition());
    }
    writer.Write(GetPrefabHash());
    if (hasRot) writer.Write(m_rotation);

    if (network) {
        auto&& find = ZDO_TARGETED_CONNECTORS.find(ID());
        if (find != ZDO_TARGETED_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto&& connector = find->second;
            writer.Write(connector.m_type);
            writer.Write(connector.m_target);

            flags |= GlobalFlag::Member_Connection;
        }
    }
    else {
        auto&& find = ZDO_CONNECTORS.find(ID());
        if (find != ZDO_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto&& connector = find->second;
            writer.Write(connector.m_type);
            writer.Write(connector.m_hash);

            flags |= GlobalFlag::Member_Connection;
        }
    }

    auto&& find = ZDO_MEMBERS.find(ID());
    if (find != ZDO_MEMBERS.end()) {
        auto&& types = find->second;

        if (_TryWriteType<float>(writer, types))
            flags |= GlobalFlag::Member_Float;
        if (_TryWriteType<Vector3f>(writer, types))
            flags |= GlobalFlag::Member_Vec3;
        if (_TryWriteType<Quaternion>(writer, types))
            flags |= GlobalFlag::Member_Quat;
        if (_TryWriteType<int32_t>(writer, types))
            flags |= GlobalFlag::Member_Int;
        if (_TryWriteType<int64_t>(writer, types))
            flags |= GlobalFlag::Member_Long;
        if (_TryWriteType<std::string>(writer, types))
            flags |= GlobalFlag::Member_String;
        if (_TryWriteType<BYTES_t>(writer, types))
            flags |= GlobalFlag::Member_ByteArray;
    }

    const auto endPos = writer.Position();
    writer.SetPos(flagPos);
    writer.Write(flags);
    writer.SetPos(endPos);
}
