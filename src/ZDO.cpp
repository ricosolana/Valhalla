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

//bool ZDO::Apply() const {
//    if (auto&& opt = ZDOManager()->_GetZDOMatch(this->GetID())) {
//        *opt = *this;
//    }
//}


#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
void ZDO::Load31Pre(DataReader& pkg, int32_t worldVersion) {
    pkg.Read<uint32_t>();       // owner rev
    pkg.Read<uint32_t>();       // data rev
    pkg.Read<bool>();           // persistent

    pkg.Read<int64_t>();        // owner
    auto timeCreated = pkg.Read<int64_t>();
    pkg.Read<int32_t>();        // pgw

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.Read<int32_t>();

    if (worldVersion >= 23)
        pkg.Read<uint8_t>();    // type

    if (worldVersion >= 22) {
        pkg.Read<bool>();       // distant
    }

    if (worldVersion < 13) {
        pkg.Read<char16_t>();
        pkg.Read<char16_t>();
    }

    const Prefab* prefab = nullptr;

    HASH_t prefabHash{};

    if (worldVersion >= 17) {
        prefabHash = pkg.Read<HASH_t>();
        prefab = &PrefabManager()->RequirePrefabByHash(prefabHash);
        _SetPrefabHash(prefabHash);
    }

    pkg.Read<Vector2i>(); // m_sector
    this->_SetPosition(pkg.Read<Vector3f>());
    this->_SetRotation(pkg.Read<Quaternion>());

    // will get or create an empty default
    auto&& members = ZDO_MEMBERS[GetID()];

    _TryReadType<float>(pkg, members, worldVersion);
    _TryReadType<Vector3f>(pkg, members, worldVersion);
    _TryReadType<Quaternion>(pkg, members, worldVersion);
    _TryReadType<int32_t>(pkg, members, worldVersion);
    _TryReadType<int64_t>(pkg, members, worldVersion);
    _TryReadType<std::string>(pkg, members, worldVersion);
    
    if (worldVersion >= 27)
        _TryReadType<BYTES_t>(pkg, members, worldVersion);

    if (worldVersion < 17) {
        prefabHash = GetInt(Hashes::ZDO::ZDO::PREFAB);
        prefab = &PrefabManager()->RequirePrefabByHash(prefabHash);
        _SetPrefabHash(prefabHash);
    }

    assert(prefab);

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

        if (prefab->AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER)
            || (GetPrefabHash() == Hashes::Object::ship_construction))
        {
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
    auto flags = reader.Read<uint16_t>();
    
    if (version) {
        // Set the self incremental id (ZDOID is no longer saved to disk)
        //this->m_id.SetUID(ZDOManager()->m_nextUid++);

        auto sector = reader.Read<Vector2s>(); // redundant
        this->_SetPosition(reader.Read<Vector3f>());
        if (sector != GetZone())
            throw std::runtime_error("sector mismatch");
    }

    // This runs once per created ZDO
    auto prefabHash = reader.Read<HASH_t>();
    if (GetPrefabHash() == 0) { // Init once
        _SetPrefabHash(prefabHash);
    }
    else {
        // should always run if a version is provided (this assumes that the world is being loaded)
#ifndef RUN_TESTS
        assert(version == 0);
#endif
    }
    
    if (flags & (1 << NETWORK_Rotation)) {
        this->_SetRotation(reader.Read<Vector3f>());
    }

    //ZDOConnector::Type type = ZDOConnector::Type::None;
    if (flags & (1 << NETWORK_Connection)) {
        auto type = reader.Read<ZDOConnector::Type>();
        if (version) {
            auto hash = reader.Read<HASH_t>();
            auto&& connector = ZDO_CONNECTORS[GetID()]; // = ZDOConnector{ .m_type = type, .m_hash = hash };
            connector.m_type = type;
            connector.m_hash = hash;
        }
        else {
            auto target = reader.Read<ZDOID>();
            auto&& connector = ZDO_TARGETED_CONNECTORS[GetID()];
            // set connection
            connector.m_type = type;
            connector.m_target = target;
            //type &= ~ZDOConnector::Type::Target;
        }
        //m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));
    }
    else {
        // Remove connector flag
        //m_pack.Unset<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));
        //m_pack.Set<FLAGS_PACK_INDEX>(
            //m_pack.Get<FLAGS_PACK_INDEX>() & (~std::to_underlying(LocalFlag::Member_Connection)));
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
        auto&& members = ZDO_MEMBERS[GetID()];
        if (flags & (1 << NETWORK_Float)) 
            _TryReadType<float>(reader, members, version);
        if (flags & (1 << NETWORK_Vec3)) 
            _TryReadType<Vector3f>(reader, members, version);
        if (flags & (1 << NETWORK_Quat)) 
            _TryReadType<Quaternion>(reader, members, version);
        if (flags & (1 << NETWORK_Int)) 
            _TryReadType<int32_t>(reader, members, version);
        if (flags & (1 << NETWORK_Long)) 
            _TryReadType<int64_t>(reader, members, version);
        if (flags & (1 << NETWORK_String)) 
            _TryReadType<std::string>(reader, members, version);
        if (flags & (1 << NETWORK_ByteArray)) 
            _TryReadType<BYTES_t>(reader, members, version);
    }
}



// ZDO specific-methods

void ZDO::SetPosition(Vector3f pos) {
    if (this->GetPosition() != pos) {
        if (IZoneManager::WorldToZonePos(pos) != GetZone()) {
            ZDOManager()->_InvalidateZDOZone(this);

            ZDOManager()->_RemoveFromSector(this);
            this->_SetPosition(pos);
            ZDOManager()->_AddZDOToZone(this);
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
    bool hasRot = this->m_rotation != Vector3f::Zero();

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
        writer.Write(this->m_rotation);
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

        if (_TryWriteType<float>(writer, types, network))
            flags |= 1 << NETWORK_Float;
        if (_TryWriteType<Vector3f>(writer, types, network))
            flags |= 1 << NETWORK_Vec3;
        if (_TryWriteType<Quaternion>(writer, types, network))
            flags |= 1 << NETWORK_Quat;
        if (_TryWriteType<int32_t>(writer, types, network))
            flags |= 1 << NETWORK_Int;
        if (_TryWriteType<int64_t>(writer, types, network))
            flags |= 1 << NETWORK_Long;
        if (_TryWriteType<std::string>(writer, types, network))
            flags |= 1 << NETWORK_String;
        if (_TryWriteType<BYTES_t>(writer, types, network))
            flags |= 1 << NETWORK_ByteArray;
    }

    const auto endPos = writer.Position();
    writer.SetPos(flagPos);
    writer.Write(flags);
    writer.SetPos(endPos);
}
