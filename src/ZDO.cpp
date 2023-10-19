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



//ZDO::ZDO() {}
//
//ZDO::ZDO(ZDOID id, Vector3f pos) : m_id(id), m_data() {
//    _SetPosition(pos);
//}


#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
void ZDO::Load31Pre(DataReader& pkg, int32_t worldVersion) {
    pkg.read<uint32_t>();       // owner rev
    pkg.read<uint32_t>();       // data rev
    _SetPersistent(pkg.read<bool>());        // persistent
        //m_pack.merge<FLAGS_PACK_INDEX>(1 << MACHINE_Persistent);           // persistent

    pkg.read<int64_t>();        // owner
    auto timeCreated = pkg.read<int64_t>();
    pkg.read<int32_t>();        // pgw

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.read<int32_t>();

    if (worldVersion >= 23)
        _SetType((ObjectType)pkg.read<uint8_t>());
        //m_pack.merge<FLAGS_PACK_INDEX>(pkg.read<uint8_t>() << MACHINE_Type1);    // m_type

    if (worldVersion >= 22) {
        _SetDistant(pkg.read<bool>());
        //if (pkg.read<bool>())
            //m_pack.merge<FLAGS_PACK_INDEX>(1 << MACHINE_Distant);       // m_distant
    }

    if (worldVersion < 13) {
        pkg.read<char16_t>();
        pkg.read<char16_t>();
    }

#if VH_IS_OFF(VH_MODULAR_PREFABS)
    const Prefab* prefab = nullptr;
#endif

    HASH_t prefabHash{};

    if (worldVersion >= 17) {
        prefabHash = pkg.read<HASH_t>();
#if VH_IS_OFF(VH_MODULAR_PREFABS)
        prefab = &PrefabManager()->RequirePrefabByHash(prefabHash);
#endif
        _SetPrefabHash(prefabHash);
    }

    pkg.read<Vector2i>(); // m_sector
    this->_SetPosition(pkg.read<Vector3f>());
    this->_SetRotation(pkg.read<Quaternion>());

    // will get or create an empty default
    auto&& members = ZDO_MEMBERS[GetID()];

    _TryReadType<float,         char16_t>(pkg, members);
    _TryReadType<Vector3f,      char16_t>(pkg, members);
    _TryReadType<Quaternion,    char16_t>(pkg, members);
    _TryReadType<int32_t,       char16_t>(pkg, members);
    _TryReadType<int64_t,       char16_t>(pkg, members);
    _TryReadType<std::string,   char16_t>(pkg, members);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t,   char16_t>(pkg, members);

    if (worldVersion < 17) {
        prefabHash = GetInt(Hashes::ZDO::ZDO::PREFAB);
#if VH_IS_OFF(VH_MODULAR_PREFABS)
        prefab = &PrefabManager()->RequirePrefabByHash(prefabHash);
#endif
        _SetPrefabHash(prefabHash);
    }

#if VH_IS_OFF(VH_MODULAR_PREFABS)
    assert(prefab);
#endif

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

        // Convert terrains
#if VH_IS_ON(VH_MODULAR_PREFABS)
        if (prefabHash == Hashes::Object::cultivate
            || prefabHash == Hashes::Object::raise
            || prefabHash == Hashes::Object::path
            || prefabHash == Hashes::Object::paved_road
            || prefabHash == Hashes::Object::HeathRockPillar
            || prefabHash == Hashes::Object::HeathRockPillar_frac
            || prefabHash == Hashes::Object::ship_construction
            || prefabHash == Hashes::Object::replant
            || prefabHash == Hashes::Object::digg
            || prefabHash == Hashes::Object::mud_road
            || prefabHash == Hashes::Object::LevelTerrain
            || prefabHash == Hashes::Object::digg_v2)
        {
#else
        if (prefab->AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER)
            || (GetPrefabHash() == Hashes::Object::ship_construction))
        {
#endif
            Set(Hashes::ZDO::TerrainModifier::TIME_CREATED, timeCreated);
        }

        // Convert seeds
        member_map copy = members;
        for (auto&& pair : copy) {
            if (xhash_to_hash<int32_t>(pair.first) == Hashes::ZDO::VisEquipment::ITEM_LEFT) {
                // assign an arbitrary random seed based off its GetID()
                Set("seed", 
                    static_cast<int32_t>(ankerl::unordered_dense::hash<ZDOID>{}(GetID())));
            }
        }
    }
}
#endif //VH_LEGACY_WORLD_LOADING

void ZDO::Unpack(DataReader& reader, int32_t version) {
    auto flags = reader.read<uint16_t>();

    if (version) {
        // Set the self incremental id (ZDOID is no longer saved to disk)
        //this->m_id.SetUID(ZDOManager()->m_nextUid++);

        auto sector = reader.read<Vector2s>(); // redundant
        this->_SetPosition(reader.read<Vector3f>());
        if (sector != GetZone())
            throw std::runtime_error("sector mismatch");
    }

    // This runs once per created ZDO
    auto prefabHash = reader.read<HASH_t>();
    if (GetPrefabHash() == 0) { // Init once
        _SetPrefabHash(prefabHash);

        if (flags & (1 << NETWORK_Persistent)) {
            //m_pack.merge<FLAGS_PACK_INDEX>(1 << MACHINE_Persistent);
            _SetPersistent(true);
        }

        if (flags & (1 << NETWORK_Distant)) {
            _SetDistant(true);
            //m_pack.merge<FLAGS_PACK_INDEX>(1 << MACHINE_Distant);
        }

        ObjectType type = ObjectType(((flags & (1 << NETWORK_Type1)) | (flags & (1 << NETWORK_Type2))) >> NETWORK_Type1);

        _SetType(type);

        //if (flags & (1 << NETWORK_Type1)) {
        //    m_pack.merge<FLAGS_PACK_INDEX>(1 << MACHINE_Type1);
        //}
        //
        //if (flags & (1 << NETWORK_Type2)) {
        //    m_pack.merge<FLAGS_PACK_INDEX>(1 << MACHINE_Type2);
        //}
    }
    else {
        // should always run if a version is provided (this assumes that the world is being loaded)
#ifndef RUN_TESTS
        assert(version == 0);
#endif
    }
    
    if (flags & (1 << NETWORK_Rotation)) {
        this->_SetRotation(reader.read<Vector3f>());
    }

    //ZDOConnector::Type type = ZDOConnector::Type::None;
    if (flags & (1 << NETWORK_Connection)) {
        auto type = reader.read<ZDOConnector::Type>();
        if (version) {
            auto hash = reader.read<HASH_t>();
            auto&& connector = ZDO_CONNECTORS[GetID()]; // = ZDOConnector{ .m_type = type, .m_hash = hash };
            connector.m_type = type;
            connector.m_hash = hash;
        }
        else {
            auto target = reader.read<ZDOID>();
            auto&& connector = ZDO_TARGETED_CONNECTORS[GetID()];
            // set connection
            connector.m_type = type;
            connector.m_target = target;
            //type &= ~ZDOConnector::Type::Target;
        }
        //m_pack.merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));
    }
    else {
        // Remove connector flag
        //m_pack.unset<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));
        //m_pack.set<FLAGS_PACK_INDEX>(
            //m_pack.get<FLAGS_PACK_INDEX>() & (~std::to_underlying(LocalFlag::Member_Connection)));
    }
    
    if (flags & (
        (1 << NETWORK_Float) 
        | (1 << NETWORK_Vec3) 
        | (1 << NETWORK_Quat) 
        | (1 << NETWORK_Int) 
        | (1 << NETWORK_Long) 
        | (1 << NETWORK_String
        | (1 << NETWORK_ByteArray)))) 
    {
        // Will insert a default if missing (should be missing already)
        auto&& members = ZDO_MEMBERS[GetID()];
        if (flags & (1 << NETWORK_Float)) 
            _TryReadType<float, uint8_t>(reader, members);
        if (flags & (1 << NETWORK_Vec3)) 
            _TryReadType<Vector3f, uint8_t>(reader, members);
        if (flags & (1 << NETWORK_Quat)) 
            _TryReadType<Quaternion, uint8_t>(reader, members);
        if (flags & (1 << NETWORK_Int)) 
            _TryReadType<int32_t, uint8_t>(reader, members);
        if (flags & (1 << NETWORK_Long)) 
            _TryReadType<int64_t, uint8_t>(reader, members);
        if (flags & (1 << NETWORK_String)) 
            _TryReadType<std::string, uint8_t>(reader, members);
        if (flags & (1 << NETWORK_ByteArray)) 
            _TryReadType<BYTES_t, uint8_t>(reader, members);
    }
}



// ZDO specific-methods

void ZDO::SetPosition(Vector3f pos) {
    if (this->GetPosition() != pos) {
        if (IZoneManager::WorldToZonePos(pos) != GetZone()) {
            ZDOManager()->_InvalidateZDOZone(*this);

            ZDOManager()->_RemoveFromSector(*this);
            this->_SetPosition(pos);
            ZDOManager()->_AddZDOToZone(*this);
        }
        else {
            this->_SetPosition(pos);
        }

        if (this->IsLocal())
            this->Revise();
    }
}

ZoneID ZDO::GetZone() const {
    return IZoneManager::WorldToZonePos(this->GetPosition());
}



void ZDO::Pack(DataWriter& writer, bool network) const {
    bool hasRot = this->m_data.get().m_rotation != Vector3f::Zero();

    uint16_t flags{};

    if (IsPersistent()) flags |= 1 << NETWORK_Persistent;
    if (IsDistant()) flags |= 1 << NETWORK_Distant;
    flags |= std::to_underlying(GetType()) << NETWORK_Type1;
    if (hasRot) flags |= 1 << NETWORK_Rotation;

    const auto flagPos = writer.Position();
    writer.Write(flags);
    if (!network) {
        writer.Write(GetZone());
        writer.Write(GetPosition());
    }
    writer.Write(GetPrefabHash());
    if (hasRot) {
        writer.Write(this->m_data.get().m_rotation);
    }

    if (network) {
        auto&& find = ZDO_TARGETED_CONNECTORS.find(GetID());
        if (find != ZDO_TARGETED_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto&& connector = find->second;
            writer.Write(connector.m_type);
            writer.Write(connector.m_target);

            flags |= 1 << NETWORK_Connection;
        }
    }
    else {
        auto&& find = ZDO_CONNECTORS.find(GetID());
        if (find != ZDO_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto&& connector = find->second;
            writer.Write(connector.m_type);
            writer.Write(connector.m_hash);

            flags |= 1 << NETWORK_Connection;
        }
    }

    auto&& find = ZDO_MEMBERS.find(GetID());
    if (find != ZDO_MEMBERS.end()) {
        auto&& types = find->second;

        if (_TryWriteType<float>(writer, types))
            flags |= 1 << NETWORK_Float;
        if (_TryWriteType<Vector3f>(writer, types))
            flags |= 1 << NETWORK_Vec3;
        if (_TryWriteType<Quaternion>(writer, types))
            flags |= 1 << NETWORK_Quat;
        if (_TryWriteType<int32_t>(writer, types))
            flags |= 1 << NETWORK_Int;
        if (_TryWriteType<int64_t>(writer, types))
            flags |= 1 << NETWORK_Long;
        if (_TryWriteType<std::string>(writer, types))
            flags |= 1 << NETWORK_String;
        if (_TryWriteType<BYTES_t>(writer, types))
            flags |= 1 << NETWORK_ByteArray;
    }

    const auto endPos = writer.Position();
    writer.SetPos(flagPos);
    writer.Write(flags);
    writer.SetPos(endPos);
}
