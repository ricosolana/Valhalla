#include <functional>
#include <utility>
#include <vector>

#include "ZDO.h"
#include "ZDOManager.h"
#include "ValhallaServer.h"
#include "ZDOID.h"
#include "PrefabManager.h"
#include "Prefab.h"
#include "ZoneManager.h"
#include "NetManager.h"
#include "VUtilsResource.h"



#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
void ZDO::Load31Pre(DataReader& pkg, int32_t worldVersion) {
    assert(false); //TODO
    /*
    pkg.read<uint32_t>();       // owner rev
    pkg.read<uint32_t>();       // data rev
    pkg.read<bool>();           // persistent

    pkg.read<int64_t>();        // owner
    auto timeCreated = pkg.read<int64_t>();
    pkg.read<int32_t>();        // pgw

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.read<int32_t>();

    if (worldVersion >= 23)
        pkg.read<uint8_t>();    // type

    if (worldVersion >= 22) {
        pkg.read<bool>();       // distant
    }

    if (worldVersion < 13) {
        pkg.read<char16_t>();
        pkg.read<char16_t>();
    }

    const Prefab* prefab = nullptr;

    HASH_t prefabHash{};

    if (worldVersion >= 17) {
        prefabHash = pkg.read<HASH_t>();
        prefab = &PrefabManager()->RequirePrefabByHash(prefabHash);
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
    }*/
}
#endif //VH_LEGACY_WORLD_LOADING

void ZDO::Unpack(DataReader& reader, int32_t version) {
    //.m_uid.SetID();
    ushort flags = reader.read<std::uint16_t>();

    //this.Persistent = (num & 256) > 0;
    //m_data_flags = m_data_flags | ((num & 256) ? DataFlags::Persistent : DataFlags::None);
    
    //this.Distant = (num & 512) > 0;
    //m_data_flags = m_data_flags | ((num & 512) ? DataFlags::Distant : DataFlags::None);
    
    //this.Type = (ZDO.ObjectType)((num >> 10) & 3);		
    //m_data_flags = m_data_flags | (static_cast<DataFlags>(num >> 10) & DataFlags::Type);

    if (version) {
        //this.m_sector = pkg.ReadVector2s();
        auto zone = reader.read<avledet::util::CSU::Vector2s>(); // sector

        //this.m_position = pkg.ReadVector3();
        //m_position = reader.read<util::CSU::Vector3f>();
        _SetPosition(reader.read<avledet::util::CSU::Vector3f>());
        if (zone != GetZone()) // a mismatch indicates a 99% of corruption
            throw std::runtime_error("sector mismatch");
    }
    
    //this.m_prefab = pkg.ReadInt();
    auto prefab_hash = reader.read<std::int32_t>();
    //_SetPrefabHash(reader.read<std::int32_t>());
    if (GetPrefabHash() == 0) { // Init once
        _SetPrefabHash(prefab_hash);
    }
    else {
        // should always run if a version is provided (this assumes that the world is being loaded)
#ifndef RUN_TESTS
        assert(version == 0);
#endif
    }

    //this.OwnerRevision = 0;
    //this.DataRevision = 0U;
    //this.Owned = false;
    //this.Owner = false;

    //this.Valid = true;
    //m_data_flags = m_data_flags | DataFlags::Valid;

    //this.SaveClone = false;

    //if ((num & 4096) > 0)
    if (flags & (1 << NETWORK_Rotation))
    {
        //this.m_rotation = pkg.ReadVector3();
        //m_rotation = reader.read<avledet::util::CSU::Vector3f>();
        _SetRotation(reader.read<avledet::util::CSU::Vector3f>());
    }

    //ZDOConnector::Type type = ZDOConnector::Type::None;
    if (flags & (1 << NETWORK_Connection)) {
        auto type = reader.read<ZDOConnector::Type>();
        if (version) { // disk
            auto hash = reader.read<HASH_t>();
            //manager->s_connectionsHashData[m_uid] = std::make_pair(reader.read<ConnectionType>(), reader.read<avledet::util::Hash>());

            auto&& connector = ZDO_CONNECTORS[GetID()]; // = ZDOConnector{ .m_type = type, .m_hash = hash };
            connector.m_type = type;
            connector.m_hash = hash;
        }
        else { // network
            auto target = reader.read<ZDOID>();
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
        if (flags & (1 << NETWORK_Float))
            load_vars(reader, version, m_floats);
        if (flags & (1 << NETWORK_Vec3))
            load_vars(reader, version, m_vec3);
        if (flags & (1 << NETWORK_Quat))
            load_vars(reader, version, m_quats);
        if (flags & (1 << NETWORK_Int))
            load_vars(reader, version, m_ints);
        if (flags & (1 << NETWORK_Long))
            load_vars(reader, version, m_longs);
        if (flags & (1 << NETWORK_String))
            load_vars(reader, version, m_strings);
        if (flags & (1 << NETWORK_ByteArray))
            load_vars(reader, version, m_byteArrays);
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

    const auto flagPos = writer.get_pos();
    writer.write(flags); // dummy spacer
    if (!network) {
        writer.write(GetZone());
        writer.write(GetPosition());
    }
    writer.write(GetPrefabHash());
    if (hasRot) {
        writer.write(this->m_rotation);
    }

    if (network) {
        auto&& find = ZDO_TARGETED_CONNECTORS.find(GetID());
        if (find != ZDO_TARGETED_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto&& connector = find->second;
            writer.write(connector.m_type);
            writer.write(connector.m_target);

            flags |= 1 << NETWORK_Connection;
        }
    }
    else {
        auto&& find = ZDO_CONNECTORS.find(GetID());
        if (find != ZDO_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto&& connector = find->second;
            writer.write(connector.m_type);
            writer.write(connector.m_hash);

            flags |= 1 << NETWORK_Connection;
        }
    }

    if (_TryWriteType<float>(writer))
        flags |= 1 << NETWORK_Float;
    if (_TryWriteType<Vector3f>(writer))
        flags |= 1 << NETWORK_Vec3;
    if (_TryWriteType<Quaternion>(writer))
        flags |= 1 << NETWORK_Quat;
    if (_TryWriteType<std::int32_t>(writer))
        flags |= 1 << NETWORK_Int;
    if (_TryWriteType<std::int64_t>(writer))
        flags |= 1 << NETWORK_Long;
    if (_TryWriteType<std::string>(writer))
        flags |= 1 << NETWORK_String;
    if (_TryWriteType<std::vector<char>>(writer))
        flags |= 1 << NETWORK_ByteArray;

    const auto endPos = writer.get_pos();
    writer.set_pos(flagPos);
    writer.write(flags);
    writer.set_pos(endPos);
}
