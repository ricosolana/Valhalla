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



decltype(ZDO::ZDO_MEMBERS) ZDO::ZDO_MEMBERS;
decltype(ZDO::ZDO_CONNECTORS) ZDO::ZDO_CONNECTORS;
decltype(ZDO::ZDO_TARGETED_CONNECTORS) ZDO::ZDO_TARGETED_CONNECTORS;
//decltype(ZDO::ZDO_OWNERS) ZDO::ZDO_OWNERS;
//decltype(ZDO::ZDO_AGES) ZDO::ZDO_AGES;

ZDO::ZDO() {
    m_pack.Set<PREFAB_PACK_INDEX>(m_pack.capacity_v<PREFAB_PACK_INDEX>);
}

ZDO::ZDO(ZDOID id, Vector3f pos) : m_id(id), m_pos(pos) {
    m_pack.Set<PREFAB_PACK_INDEX>(m_pack.capacity_v<PREFAB_PACK_INDEX>);
}



void ZDO::Load31Pre(DataReader& pkg, int32_t worldVersion) {
    static constexpr auto szzie = sizeof(ZDO);

    pkg.Read<uint32_t>();       // owner rev
    pkg.Read<uint32_t>();       // data rev
    if (pkg.Read<bool>())
        m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Marker_Persistent));           // persistent
    pkg.Read<int64_t>();        // owner
    auto timeCreated = pkg.Read<int64_t>();
    pkg.Read<int32_t>();        // pgw

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        m_pack.Merge<FLAGS_PACK_INDEX>(pkg.Read<uint8_t>() << LocalDenotion::Marker_Type1);    // m_type

    if (worldVersion >= 22) {
        if (pkg.Read<bool>())
            m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Marker_Distant));       // m_distant
    }

    if (worldVersion < 13) {
        pkg.Read<char16_t>();
        pkg.Read<char16_t>();
    }

#if VH_IS_ON(VH_STANDARD_PREFABS)
    const Prefab* prefab = nullptr;
#endif

    if (worldVersion >= 17) {
#if VH_IS_ON(VH_STANDARD_PREFABS)
        auto&& pair = PrefabManager()->RequirePrefabAndIndexByHash(pkg.Read<HASH_t>());
        prefab = &pair.first;
        m_pack.Set<PREFAB_PACK_INDEX>(pair.second);
#else
        _SetPrefabHash(pkg.Read<HASH_t>());
        /*
        auto&& index = PrefabManager()->RequirePrefabIndexByHash(pkg.Read<HASH_t>());
        m_pack.Set<PREFAB_PACK_INDEX>(index);*/
#endif
    }

    pkg.Read<Vector2i>(); // m_sector
    this->m_pos = pkg.Read<Vector3f>();
    this->m_rotation = pkg.Read<Quaternion>().EulerAngles();

    auto&& members = ZDO_MEMBERS[ID()];

    _TryReadType<float,         char16_t>(pkg, members);
    _TryReadType<Vector3f,      char16_t>(pkg, members);
    _TryReadType<Quaternion,    char16_t>(pkg, members);
    _TryReadType<int32_t,       char16_t>(pkg, members);
    _TryReadType<int64_t,       char16_t>(pkg, members);
    _TryReadType<std::string,   char16_t>(pkg, members);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t,   char16_t>(pkg, members);

    if (worldVersion < 17) {
#if VH_IS_ON(VH_STANDARD_PREFABS)
        auto&& pair = PrefabManager()->RequirePrefabAndIndexByHash(GetInt(Hashes::ZDO::ZDO::PREFAB));
        prefab = &pair.first;
        m_pack.Set<PREFAB_PACK_INDEX>(pair.second);
#else
        _SetPrefabHash(GetInt(Hashes::ZDO::ZDO::PREFAB));
        /*
        auto&& index = PrefabManager()->RequirePrefabIndexByHash(GetInt(Hashes::ZDO::ZDO::PREFAB));
        m_pack.Set<PREFAB_PACK_INDEX>(index);*/
#endif
    }

#if VH_IS_ON(VH_STANDARD_PREFABS)
    assert(prefab);

    if (prefab->AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER))
         Set(Hashes::ZDO::TerrainModifier::TIME_CREATED, timeCreated);
#endif
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

    auto flags = reader.Read<uint16_t>();

    if (version) {
        reader.Read<Vector2s>(); // redundant
        this->m_pos = reader.Read<Vector3f>();
    }

    // prefab is loaded once
    //  If prefabs are disabled, then loading by index wont exactly work
    //  the way to do it would be by hash
    //      maybe should remove the prefab dynamic disable/enable as it introduces excessive problems
    //      prefab system would reduce usage on esp by at most 10% per zdo (-4 bytes)
    //  prefab system does introduce initial usage ~2270 prefabs, each at 72 bytes (total 0.16MB), this is a low end best-case scenario (considering how string takes up extra heap memory and the hashmap structure is semi-costly)
    //  on esp this might not be viable
    auto prefabHash = reader.Read<HASH_t>();
    if (m_pack.Get<PREFAB_PACK_INDEX>() == m_pack.capacity_v<PREFAB_PACK_INDEX>) {
        //m_pack.Set<PREFAB_PACK_INDEX>(PrefabManager()->RequirePrefabIndexByHash(prefabHash));

        _SetPrefabHash(prefabHash);

        if (flags & GlobalFlag::Marker_Persistent) {
            m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Marker_Persistent));
        }

        if (flags & GlobalFlag::Marker_Distant) {
            m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Marker_Distant));
        }

        if (flags & GlobalFlag::Marker_Type1) {
            m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Marker_Type1));
        }

        if (flags & GlobalFlag::Marker_Type2) {
            m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Marker_Type2));
        }
    }
    else {
        // should always run if a version is provided (this assumes that the world is being loaded)
        assert(version == 0);
    }
    
    if (flags & GlobalFlag::Marker_Rotation) {
        this->m_rotation = reader.Read<Vector3f>();
    }

    ZDOConnector::Type type = ZDOConnector::Type::None;
    if (flags & GlobalFlag::Member_Connection) {
        type = reader.Read<ZDOConnector::Type>();
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
            type &= ~ZDOConnector::Type::Target;
        }
        m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));
    }
    else {
        // Remove connector flag
        m_pack.Set<FLAGS_PACK_INDEX>(
            m_pack.Get<FLAGS_PACK_INDEX>() & (~std::to_underlying(LocalFlag::Member_Connection)));
    }

    if (flags & (GlobalFlag::Member_Float | GlobalFlag::Member_Vec3 | GlobalFlag::Member_Quat | GlobalFlag::Member_Int | GlobalFlag::Member_Long | GlobalFlag::Member_String | GlobalFlag::Member_ByteArray)) {
        // Will insert a default if missing (should be missing already)
        auto&& members = ZDO_MEMBERS[ID()];
        if (flags & GlobalFlag::Member_Float) _TryReadType<float, uint8_t>(reader, members);
        if (flags & GlobalFlag::Member_Vec3) _TryReadType<Vector3f, uint8_t>(reader, members);
        if (flags & GlobalFlag::Member_Quat) _TryReadType<Quaternion, uint8_t>(reader, members);
        if (flags & GlobalFlag::Member_Int) _TryReadType<int32_t, uint8_t>(reader, members);
        if (flags & GlobalFlag::Member_Long) _TryReadType<int64_t, uint8_t>(reader, members);
        if (flags & GlobalFlag::Member_String) _TryReadType<std::string, uint8_t>(reader, members);
        if (flags & GlobalFlag::Member_ByteArray) _TryReadType<BYTES_t, uint8_t>(reader, members);
    }

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
    bool hasRot = std::abs(m_rotation.x) > std::numeric_limits<float>::epsilon() * 8.f
        || std::abs(m_rotation.y) > std::numeric_limits<float>::epsilon() * 8.f
        || std::abs(m_rotation.z) > std::numeric_limits<float>::epsilon() * 8.f;

    uint16_t flags{};
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Float ? GlobalFlag::Member_Float : (GlobalFlag)0;
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Vec3 ? GlobalFlag::Member_Vec3 : (GlobalFlag)0;
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Quat ? GlobalFlag::Member_Quat : (GlobalFlag)0;
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Int ? GlobalFlag::Member_Int : (GlobalFlag)0;
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Long ? GlobalFlag::Member_Long : (GlobalFlag)0;
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_String ? GlobalFlag::Member_String : (GlobalFlag)0;
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_ByteArray ? GlobalFlag::Member_ByteArray : (GlobalFlag)0;
    flags |= m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Connection ? GlobalFlag::Member_Connection : (GlobalFlag)0;
    flags |= IsPersistent() ? GlobalFlag::Marker_Persistent : (GlobalFlag)0;
    flags |= IsDistant() ? GlobalFlag::Marker_Distant : (GlobalFlag)0;
    flags |= GetType() << std::to_underlying(GlobalDenotion::Marker_Type1);
    flags |= hasRot ? GlobalFlag::Marker_Rotation : GlobalFlag(0);

    writer.Write(flags);
    if (!network) {
        writer.Write(GetZone());
        writer.Write(Position());
    }
    writer.Write(GetPrefabHash());
    if (hasRot) writer.Write(m_rotation);

    // TODO add connector
    if (m_pack.Get<FLAGS_PACK_INDEX>() & LocalFlag::Member_Connection) {
        if (network) {
            auto&& find = ZDO_TARGETED_CONNECTORS.find(ID());
            if (find != ZDO_TARGETED_CONNECTORS.end()) {
                auto&& connector = find->second;
                writer.Write(connector.m_type);
                writer.Write(connector.m_target);
            }
            else {
                assert(false);
            }
        }
        else {
            auto&& find = ZDO_CONNECTORS.find(ID());
            if (find != ZDO_CONNECTORS.end()) {
                auto&& connector = find->second;
                writer.Write(connector.m_type);
                writer.Write(connector.m_hash);
            }
            else {
                assert(false);
            }
        }
    }
    else {
        assert(!ZDO_CONNECTORS.contains(ID()) && !ZDO_TARGETED_CONNECTORS.contains(ID()));
    }


    //if (m_encoded.HasAnyFlags(LocalFlag::Member_Float | LocalFlag::Member_Vec3 | LocalFlag::Member_Quat | LocalFlag::Member_Int | LocalFlag::Member_Long | LocalFlag::Member_String | LocalFlag::Member_ByteArray)) {
    if (m_pack.Get<FLAGS_PACK_INDEX>() & (LocalFlag::Member_Float | LocalFlag::Member_Vec3 | LocalFlag::Member_Quat | LocalFlag::Member_Int | LocalFlag::Member_Long | LocalFlag::Member_String | LocalFlag::Member_ByteArray)) {
        auto&& find = ZDO_MEMBERS.find(ID());
        if (find != ZDO_MEMBERS.end()) {
            auto&& types = find->second;

            _TryWriteType<float>(writer, types);
            _TryWriteType<Vector3f>(writer, types);
            _TryWriteType<Quaternion>(writer, types);
            _TryWriteType<int32_t>(writer, types);
            _TryWriteType<int64_t>(writer, types);
            _TryWriteType<std::string>(writer, types);
            _TryWriteType<BYTES_t>(writer, types);
        }
        else {
            assert(false);
        }
    }
}
