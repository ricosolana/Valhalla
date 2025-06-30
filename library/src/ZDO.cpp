#include <functional>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "NetManager.h"
#include "Prefab.h"
#include "PrefabManager.h"
#include "Types.h"
#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "ZDO.h"
#include "ZDOID.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

bool ZDO::_SetConnection(ZDOConnector::Type type, ZDOID zdoid)
{
    auto &&insert = ZDO_TARGETED_CONNECTORS.try_emplace(m_id, type, zdoid);

    auto &&connector = insert.first->second;

    // if it was not newly inserted
    //  check the old values
    if (!insert.second) {
        auto &&type2  = connector.m_type;
        auto &&zdoid2 = connector.m_target;

        if (type == type2 && zdoid2 == zdoid) {
            return false;
        }
    }

    connector.m_type   = type;
    connector.m_target = zdoid;

    //m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));

    return true;
}

void ZDO::SetConnection(ZDOConnector::Type type, ZDOID zdoid)
{
    if (_SetConnection(type, zdoid)) {
        Revise();
    }
}

ZDOID ZDO::GetConnectionZDOID(ZDOConnector::Type type) const
{
    auto &&find = ZDO_TARGETED_CONNECTORS.find(m_id);
    if (find != ZDO_TARGETED_CONNECTORS.end()) {
        if (find->second.m_type == type)
            return find->second.m_target;
    }
    return ZDOID::NONE;
}

//ZDOID ZDO::GetConnectionZDOID() const
//{
//    auto &&find = ZDO_TARGETED_CONNECTORS.find(m_id);
//    if (find != ZDO_TARGETED_CONNECTORS.end()) {
//        return find->second.m_target;
//    }
//    return ZDOID::NONE;
//}

ZDOID ZDO::GetID() const
{
    return this->m_id;
}

Vector3f ZDO::GetPosition() const
{
    return this->m_pos;
}

ZDO::Rev &ZDO::GetRevision()
{
    return this->m_rev;
}

Quaternion ZDO::GetRotation() const
{
    return Quaternion::euler(this->m_rotation);
}

void ZDO::SetRotation(Quaternion rot)
{
    auto &&euler = rot.euler_angles();
    if (euler != this->m_rotation) {
        this->m_rotation = euler;
        this->Revise();
    }
}

Prefab const &ZDO::GetPrefab() const
{
    return PrefabManager()->get_indexed_prefab(m_prefab_index);
    //return PrefabManager()->get_prefab(this->m_prefabHash);
}

avledet::util::Hash ZDO::GetPrefabHash() const
{
    return this->GetPrefab().m_hash;
}

void ZDO::SetLocalScale(Vector3f scale, bool allowIdentity)
{
    // if scaling along all axis VS scaling axis differently
    // this is just to save some memory
    if (std::abs(scale.x - scale.y) < std::numeric_limits<float>::epsilon() * 8
        && std::abs(scale.y - scale.z) < std::numeric_limits<float>::epsilon() * 8) {

        if (allowIdentity || std::abs(scale.x - 1) > std::numeric_limits<float>::epsilon() * 8) {
            this->set(avledet::util::hashes::ZDO::ZNetView::SCALE_SCALAR, scale);
        }
    } else {
        // otherwise use scale
        this->set(avledet::util::hashes::ZDO::ZNetView::SCALE, scale);
    }
}

avledet::util::UserID ZDO::Owner() const
{
    // TODO optimize by checking owner bit
    auto &&find = ZDO_OWNERS.find(GetID());
    if (find != ZDO_OWNERS.end()) {
        return find->second;
    }
    return 0;
}

bool ZDO::IsOwner(avledet::util::UserID owner) const
{
    return owner == this->Owner();
}

bool ZDO::IsLocal() const
{
    return this->IsOwner(AVL_ID);
}

bool ZDO::HasOwner() const
{
    return this->Owner() != 0;
}

bool ZDO::SetLocal()
{
    return this->SetOwner(AVL_ID);
}

void ZDO::Disown()
{
    this->SetOwner(0);
}

bool ZDO::SetOwner(avledet::util::UserID owner)
{
    // only if the owner has changed, then revise it
    if (this->Owner() != owner) {
        this->_SetOwner(owner);

        this->m_rev.ReviseOwner();
        return true;
    }
    return false;
}

std::uint16_t ZDO::GetOwnerRevision() const
{
    return this->m_rev.GetOwnerRevision();
}

std::uint32_t ZDO::GetDataRevision() const
{
    return this->m_rev.GetDataRevision();
}

bool ZDO::IsPersistent() const
{
    return GetPrefab().IsPersistent();
}

bool ZDO::IsDistant() const
{
    return GetPrefab().IsDistant();
}

avledet::util::ObjectType ZDO::GetType() const
{
    return GetPrefab().GetObjectType();
}

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
void ZDO::Load31Pre(DataReader &pkg, std::int32_t worldVersion)
{
    (void) pkg;
    (void) worldVersion;
    assert(false);//TODO
    /*
    pkg.read<std::uint32_t>();       // owner rev
    pkg.read<std::uint32_t>();       // data rev
    pkg.read<bool>();           // persistent

    pkg.read<std::int64_t>();        // owner
    auto timeCreated = pkg.read<std::int64_t>();
    pkg.read<std::int32_t>();        // pgw

    if (worldVersion >= 16 && worldVersion < 24)
        pkg.read<std::int32_t>();

    if (worldVersion >= 23)
        pkg.read<std::uint8_t>();    // type

    if (worldVersion >= 22) {
        pkg.read<bool>();       // distant
    }

    if (worldVersion < 13) {
        pkg.read<char16_t>();
        pkg.read<char16_t>();
    }

    const Prefab* prefab = nullptr;

    avledet::util::Hash prefabHash{};

    if (worldVersion >= 17) {
        prefabHash = pkg.read<avledet::util::Hash>();
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
    _TryReadType<std::int32_t,       char16_t>(pkg, members);
    _TryReadType<std::int64_t,       char16_t>(pkg, members);
    _TryReadType<std::string,   char16_t>(pkg, members);
    
    if (worldVersion >= 27)
        _TryReadType<avledet::util::Bytes,   char16_t>(pkg, members);

    if (worldVersion < 17) {
        prefabHash = GetInt(avledet::util::hashes::ZDO::ZDO::PREFAB);
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
                Set(avledet::util::hashes::ZDO::USER, zdoid.GetOwner());
            }
        }

        {
            auto&& zdoid = GetZDOID("RodOwner");
            if (zdoid) {
                SetLocal();
                Set(avledet::util::hashes::ZDO::FishingFloat::ROD_OWNER, zdoid.GetOwner());
            }
        }

        if (prefab->AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER)
            || (GetPrefabHash() == avledet::util::hashes::Object::ship_construction))
        {
            Set(avledet::util::hashes::ZDO::TerrainModifier::TIME_CREATED, timeCreated);
        }

        // Convert seeds
        member_map copy = members;
        for (auto&& pair : copy) {
            if (xhash_to_hash<std::int32_t>(pair.first) == avledet::util::hashes::ZDO::VisEquipment::ITEM_LEFT) {
                // assign an arbitrary random seed based off its GetID()
                Set("seed", 
                    static_cast<std::int32_t>(ankerl::unordered_dense::hash<ZDOID>{}(GetID())));
            }
        }
    }*/
}
#endif//AVL_LEGACY_WORLD_LOADING

void ZDO::Unpack(DataReader &reader, std::int32_t version)
{
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
        auto zone = reader.read<avledet::util::CSU::Vector2s>();// sector

        //this.m_position = pkg.ReadVector3();
        //m_position = reader.read<util::CSU::Vector3f>();
        _SetPosition(reader.read<avledet::util::CSU::Vector3f>());
        if (zone != GetZone())// a mismatch indicates a 99% of corruption
            throw std::runtime_error("sector mismatch");
    }

    //this.m_prefab = pkg.ReadInt();
    auto prefab_hash = reader.read<std::int32_t>();
    //_SetPrefabHash(reader.read<std::int32_t>());
    //if (GetPrefabHash() == 0) {// Init once
    if (m_prefab_index == Prefab::NONE) {// Init once
        _SetPrefabHash(prefab_hash);
    } else {
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
    if (flags & (1 << NETWORK_Rotation)) {
        //this.m_rotation = pkg.ReadVector3();
        //m_rotation = reader.read<avledet::util::CSU::Vector3f>();
        _SetRotation(reader.read<avledet::util::CSU::Vector3f>());
    }

    //ZDOConnector::Type type = ZDOConnector::Type::None;
    if (flags & (1 << NETWORK_Connection)) {
        auto type = reader.read<ZDOConnector::Type>();
        if (version) {// disk
            auto hash = reader.read<avledet::util::Hash>();
            //manager->s_connectionsHashData[m_uid] = std::make_pair(reader.read<ConnectionType>(), reader.read<avledet::util::Hash>());

            auto &&connector = ZDO_CONNECTORS[GetID()];// = ZDOConnector{ .m_type = type, .m_hash = hash };
            connector.m_type = type;
            connector.m_hash = hash;
        } else {                                       // network
            auto target      = reader.read<ZDOID>();
            auto &&connector = ZDO_TARGETED_CONNECTORS[GetID()];
            // set connection
            connector.m_type   = type;
            connector.m_target = target;
            //type &= ~ZDOConnector::Type::Target;
        }
        //m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));
    } else {
        // Remove connector flag
        //m_pack.Unset<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));
        //m_pack.Set<FLAGS_PACK_INDEX>(
        //m_pack.Get<FLAGS_PACK_INDEX>() & (~std::to_underlying(LocalFlag::Member_Connection)));
    }

    if (flags
        & ((1 << NETWORK_Float) | (1 << NETWORK_Vec3) | (1 << NETWORK_Quat) | (1 << NETWORK_Int)
           | (1 << NETWORK_Long) | (1 << NETWORK_String | (1 << NETWORK_ByteArray)))) {
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

void ZDO::SetPosition(Vector3f pos)
{
    if (this->GetPosition() != pos) {
        if (IZoneManager::WorldToZonePos(pos) != GetZone()) {
            ZDOManager()->_InvalidateZDOZone(this->intrusive_from_this());

            ZDOManager()->_RemoveFromSector(this->intrusive_from_this());
            this->_SetPosition(pos);//unrevised
            ZDOManager()->_AddZDOToZone(this->intrusive_from_this());
        } else {
            this->_SetPosition(pos);
        }

        assert(IZoneManager::WorldToZonePos(pos) == GetZone());

        if (this->IsLocal())
            this->Revise();
    }
}

ZoneID ZDO::GetZone() const
{
    return IZoneManager::WorldToZonePos(this->GetPosition());
}

void ZDO::Pack(DataWriter &writer, bool network) const
{
    bool hasRot = this->m_rotation != Vector3f::ZERO;

    std::uint16_t flags {};

    if (IsPersistent())
        flags |= 1 << NETWORK_Persistent;
    if (IsDistant())
        flags |= 1 << NETWORK_Distant;
    flags |= std::to_underlying(GetType()) << NETWORK_Type1;
    if (hasRot)
        flags |= 1 << NETWORK_Rotation;

    auto const flagPos = writer.get_pos();
    writer.write(flags);// dummy spacer
    if (!network) {
        writer.write(GetZone());
        writer.write(GetPosition());
    }
    writer.write(GetPrefabHash());
    if (hasRot) {
        writer.write(this->m_rotation);
    }

    if (network) {
        auto &&find = ZDO_TARGETED_CONNECTORS.find(GetID());
        if (find != ZDO_TARGETED_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto &&connector = find->second;
            writer.write(connector.m_type);
            writer.write(connector.m_target);

            flags |= 1 << NETWORK_Connection;
        }
    } else {
        auto &&find = ZDO_CONNECTORS.find(GetID());
        if (find != ZDO_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto &&connector = find->second;
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
    if (_TryWriteType<avledet::util::Bytes>(writer))
        flags |= 1 << NETWORK_ByteArray;

    auto const endPos = writer.get_pos();
    writer.set_pos(flagPos);
    writer.write(flags);
    writer.set_pos(endPos);
}
