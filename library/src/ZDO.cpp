#include <functional>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Avledet.h"
#include "NetManager.h"
#include "Prefab.h"
#include "PrefabManager.h"
#include "Types.h"
#include "VUtilsResource.h"
#include "ZDO.h"
#include "ZDOID.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

/*
    Rev define
*/

ZDO::Rev::Rev() {}

ZDO::Rev::Rev(std::uint32_t dataRev, std::uint16_t ownerRev)
{
    this->set_data_rev(dataRev);
    this->set_owner_rev(ownerRev);
}

std::uint32_t ZDO::Rev::get_data_rev() const
{
    return m_pack.get<DATA_REVISION_PACK_INDEX>();
}

std::uint16_t ZDO::Rev::get_owner_rev() const
{
    return m_pack.get<OWNER_REVISION_PACK_INDEX>();
}

void ZDO::Rev::set_data_rev(std::uint32_t dataRev)
{
    m_pack.set<DATA_REVISION_PACK_INDEX>(dataRev);
}

void ZDO::Rev::set_owner_rev(std::uint16_t ownerRev)
{
    m_pack.set<OWNER_REVISION_PACK_INDEX>(ownerRev);
}

void ZDO::Rev::rev_data()
{
    this->set_data_rev(this->get_data_rev() + 1);
}

void ZDO::Rev::rev_owner()
{
    this->set_owner_rev(this->get_owner_rev() + 1);
}

/*
    init modernizers
*/

static auto const strippable_nodes = ankerl::unordered_dense::set<avledet::util::Hash>(
        {avledet::util::get_stable_hash("generated"), avledet::util::get_stable_hash("patrolSpawnPoint"),
         avledet::util::get_stable_hash("autoDespawn"), avledet::util::get_stable_hash("targetHear"),
         avledet::util::get_stable_hash("targetSee"), avledet::util::get_stable_hash("burnt0"),
         avledet::util::get_stable_hash("burnt1"), avledet::util::get_stable_hash("burnt2"),
         avledet::util::get_stable_hash("burnt3"), avledet::util::get_stable_hash("burnt4"),
         avledet::util::get_stable_hash("burnt5"), avledet::util::get_stable_hash("burnt6"),
         avledet::util::get_stable_hash("burnt7"), avledet::util::get_stable_hash("burnt8"),
         avledet::util::get_stable_hash("burnt9"), avledet::util::get_stable_hash("burnt10"),
         avledet::util::get_stable_hash("LookDir"), avledet::util::get_stable_hash("RideSpeed")});

static auto const strippable_long_nodes = ankerl::unordered_dense::set<avledet::util::Hash>({
        avledet::util::get_stable_hash("user_u"),
        avledet::util::get_stable_hash("user_i"),
        avledet::util::get_stable_hash("RodOwner_u"),
        avledet::util::get_stable_hash("RodOwner_i"),
        avledet::util::get_stable_hash("CatchID_u"),
        avledet::util::get_stable_hash("CatchID_i"),
});

bool ZDO::_can_strip(avledet::util::Hash key)
{
    return strippable_nodes.contains(key);
}

bool ZDO::_can_strip(avledet::util::Hash key, float data)
{
    return strippable_nodes.contains(key)
           || (key == avledet::util::get_stable_hash("scaleScalar") && avledet::util::CSU::equal(data, 1.f));
}

bool ZDO::_can_strip(avledet::util::Hash key, avledet::util::CSU::Quaternion const &data)
{
    return data == avledet::util::CSU::Quaternion::IDENTITY || _can_strip(key);
}

bool ZDO::_can_strip(avledet::util::Hash key, std::int32_t data)
{
    return data == 0 || _can_strip(key);
}

bool ZDO::_can_strip(avledet::util::Hash key, std::int64_t data)
{
    return data == 0 || _can_strip(key) || strippable_long_nodes.contains(key);
}

bool ZDO::_can_strip(avledet::util::Hash key, std::string const &data)
{
    return data.empty() || _can_strip(key);
}

bool ZDO::_can_strip(avledet::util::Hash key, std::vector<char> const &data)
{
    return data.empty() || _can_strip(key);
}

// (Keep as a member function, to access m_id as needed in future)
bool ZDO::_try_convert(avledet::util::Hash key, avledet::util::CSU::Vector3f data)
{
    if (this->_can_strip(key)) {
        return true;
    }
    if (key == avledet::util::get_stable_hash("SpawnPoint")) {
        //ZDOExtraData.Set(zdoid, ZDOVars.s_spawnPoint, data);
        _set(m_id, avledet::util::get_stable_hash("spawnpoint"), data);
        return true;
    }

    // if x == y == z
    if (avledet::util::CSU::equal(data.x, data.y) && avledet::util::CSU::equal(data.y, data.z)) {
        if (key == avledet::util::get_stable_hash("scale")) {
            // 1 is unit scale
            if (avledet::util::CSU::equal(data.x, 1.f)) {
                return true;
            }
            this->_set(m_id, avledet::util::get_stable_hash("scale"), data.x);
            //ZDOExtraData.Set(zid, ZDOVars.s_scaleScalarHash, data.x);
            return true;
        }
        // 0 is default ZDO when no mapping exists
        else if (avledet::util::CSU::equal(data.x, 0.f)) {
            return true;
        }
    }
    return false;
}

std::uint32_t ZDO::_read_num_items(avledet::util::Reader &reader, int version)
{
    if (version < 33) {
        return (std::uint32_t) reader.read<std::uint8_t>();
    }
    auto num = (std::uint32_t) reader.read<std::uint8_t>();
    if ((num & 128) != 0) {
        num = ((num & 127) << 8) | (std::uint32_t) reader.read<std::uint8_t>();
    }
    return num;
};

void ZDO::_write_num_items(DataWriter &writer, int numItems)
{
    if (numItems < 128) {
        writer.write((std::uint8_t) numItems);
        return;
    }
    writer.write((std::uint8_t)((numItems >> 8) | 128));
    writer.write((std::uint8_t) numItems);
}

/*
    ZDO define
*/

void ZDO::_revise()
{
    this->m_rev.rev_data();
}

void ZDO::_set_prefab_hash(avledet::util::Hash hash)
{
    this->m_prefab_index = PrefabManager()->get_prefab_index(hash);
}

// Set the owner of the ZDO without revising
void ZDO::_set_owner(avledet::util::UserID owner)
{
    if (owner) {
        ZDO_OWNERS[m_id] = owner;
    } else {
        ZDO_OWNERS.erase(m_id);
    }
}

void ZDO::_set_position(Vector3f const &pos)
{
    this->m_pos = pos;
}

void ZDO::_set_rotation(Vector3f const &rot)
{
    this->m_rotation = rot;
}

void ZDO::_set_rotation(Quaternion const &rot)
{
    this->_set_rotation(rot.euler_angles());
}

bool ZDO::_set_connection(ZDOConnector::Type type, ZDOID zdoid)
{
    auto &&insert = PAIRED_CONNECTORS.try_emplace(m_id, type, zdoid);

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

void ZDO::set_connection(ZDOConnector::Type type, ZDOID zdoid)
{
    if (this->_set_connection(type, zdoid)) {
        this->_revise();
    }
}

ZDOID ZDO::get_connection_zdoid(ZDOConnector::Type type) const
{
    auto &&find = PAIRED_CONNECTORS.find(m_id);
    if (find != PAIRED_CONNECTORS.end()) {
        if (find->second.m_type == type)
            return find->second.m_target;
    }
    return ZDOID::NONE;
}

ZDOID ZDO::get_id() const
{
    return this->m_id;
}

Vector3f ZDO::get_position() const
{
    return this->m_pos;
}

//ZDO::Rev &ZDO::_get_revision()
//{
//    return this->m_rev;
//}

Quaternion ZDO::get_rotation() const
{
    return Quaternion::euler(this->m_rotation);
}

void ZDO::set_rotation(Quaternion rot)
{
    auto &&euler = rot.euler_angles();
    if (euler != this->m_rotation) {
        this->m_rotation = euler;
        this->_revise();
    }
}

Prefab const &ZDO::get_prefab() const
{
    return PrefabManager()->get_indexed_prefab(m_prefab_index);
}

avledet::util::Hash ZDO::get_prefab_hash() const
{
    return this->get_prefab().m_hash;
}

void ZDO::set_local_scale(Vector3f scale, bool allowIdentity)
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

avledet::util::UserID ZDO::get_owner() const
{
    // TODO optimize by checking owner bit
    auto &&find = ZDO_OWNERS.find(this->m_id);
    if (find != ZDO_OWNERS.end()) {
        return find->second;
    }
    return 0;
}

bool ZDO::is_owner(avledet::util::UserID owner) const
{
    return owner == this->get_owner();
}

bool ZDO::owned_by_me() const
{
    return this->is_owner(AVL_ID);
}

bool ZDO::has_owner() const
{
    return this->get_owner() != 0;
}

bool ZDO::claim()
{
    return this->set_owner(AVL_ID);
}

void ZDO::set_claimed(bool local)
{
    if (local) {
        this->claim();
    } else {
        this->disown();
    }
}

void ZDO::disown()
{
    this->set_owner(0);
}

bool ZDO::set_owner(avledet::util::UserID owner)
{
    // only if the owner has changed, then revise it
    if (this->get_owner() != owner) {
        this->_set_owner(owner);

        this->m_rev.rev_owner();
        return true;
    }
    return false;
}

std::uint16_t ZDO::get_owner_rev() const
{
    return this->m_rev.get_owner_rev();
}

std::uint32_t ZDO::get_data_rev() const
{
    return this->m_rev.get_data_rev();
}

bool ZDO::is_persistent() const
{
    return this->get_prefab().is_persistent();
}

bool ZDO::is_distant() const
{
    return this->get_prefab().is_distant();
}

avledet::util::ObjectType ZDO::get_type() const
{
    return this->get_prefab().GetObjectType();
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
    this->_set_position(pkg.read<Vector3f>());
    this->_SetRotation(pkg.read<Quaternion>());

    // will get or create an empty default
    auto&& members = ZDO_MEMBERS[get_id()];

    _TryReadType<float,         char16_t>(pkg, members);
    _TryReadType<Vector3f,      char16_t>(pkg, members);
    _TryReadType<Quaternion,    char16_t>(pkg, members);
    _TryReadType<std::int32_t,       char16_t>(pkg, members);
    _TryReadType<std::int64_t,       char16_t>(pkg, members);
    _TryReadType<std::string,   char16_t>(pkg, members);
    
    if (worldVersion >= 27)
        _TryReadType<avledet::util::Bytes,   char16_t>(pkg, members);

    if (worldVersion < 17) {
        prefabHash = get_int(avledet::util::hashes::ZDO::ZDO::PREFAB);
        prefab = &PrefabManager()->RequirePrefabByHash(prefabHash);
        _SetPrefabHash(prefabHash);
    }

    assert(prefab);

    if (worldVersion < 31) {
        // Convert owners
        {
            auto&& zdoid = get_zdoid("user");
            if (zdoid) {
                claim();
                set(avledet::util::hashes::ZDO::USER, zdoid.GetOwner());
            }
        }

        {
            auto&& zdoid = get_zdoid("RodOwner");
            if (zdoid) {
                claim();
                set(avledet::util::hashes::ZDO::FishingFloat::ROD_OWNER, zdoid.GetOwner());
            }
        }

        if (prefab->AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER)
            || (get_prefab_hash() == avledet::util::hashes::Object::ship_construction))
        {
            set(avledet::util::hashes::ZDO::TerrainModifier::TIME_CREATED, timeCreated);
        }

        // Convert seeds
        member_map copy = members;
        for (auto&& pair : copy) {
            if (xhash_to_hash<std::int32_t>(pair.first) == avledet::util::hashes::ZDO::VisEquipment::ITEM_LEFT) {
                // assign an arbitrary random seed based off its get_id()
                set("seed", 
                    static_cast<std::int32_t>(ankerl::unordered_dense::hash<ZDOID>{}(get_id())));
            }
        }
    }*/
}
#endif//AVL_LEGACY_WORLD_LOADING

void ZDO::unpack(DataReader &reader, std::int32_t version)
{
    auto flags = reader.read<std::uint16_t>();

    if (version) {
        auto zone = reader.read<avledet::util::CSU::Vector2s>();// sector

        this->_set_position(reader.read<avledet::util::CSU::Vector3f>());
        if (zone != get_zone())                                 // a mismatch indicates a 99% of corruption
            throw std::runtime_error("sector mismatch");
    }

    auto prefab_hash = reader.read<std::int32_t>();
    if (m_prefab_index == Prefab::NONE) {// Init once
        this->_set_prefab_hash(prefab_hash);
    } else {
        // prefab index is set once for world loads,
        //assert(version == 0);
    }

    //if ((num & 4096) > 0)
    if (flags & (1 << NETWORK_Rotation)) {
        this->_set_rotation(reader.read<avledet::util::CSU::Vector3f>());
    }

    //ZDOConnector::Type type = ZDOConnector::Type::None;
    if (flags & (1 << NETWORK_Connection)) {
        auto type = reader.read<ZDOConnector::Type>();
        if (version) {// disk
            auto hash = reader.read<avledet::util::Hash>();
            //manager->s_connectionsHashData[m_uid] = std::make_pair(reader.read<ConnectionType>(), reader.read<avledet::util::Hash>());

            auto &&connector = TYPED_CONNECTORS[get_id()];// = ZDOConnector{ .m_type = type, .m_hash = hash };
            connector.m_type = type;
            connector.m_hash = hash;
        } else {                                          // network
            auto target      = reader.read<ZDOID>();
            auto &&connector = PAIRED_CONNECTORS[get_id()];
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
            this->_load_vars(reader, version, m_floats);
        if (flags & (1 << NETWORK_Vec3))
            this->_load_vars(reader, version, m_vec3);
        if (flags & (1 << NETWORK_Quat))
            this->_load_vars(reader, version, m_quats);
        if (flags & (1 << NETWORK_Int))
            this->_load_vars(reader, version, m_ints);
        if (flags & (1 << NETWORK_Long))
            this->_load_vars(reader, version, m_longs);
        if (flags & (1 << NETWORK_String))
            this->_load_vars(reader, version, m_strings);
        if (flags & (1 << NETWORK_ByteArray))
            this->_load_vars(reader, version, m_byteArrays);
    }
}

// ZDO specific-methods

void ZDO::set_position(Vector3f pos)
{
    if (this->get_position() != pos) {
        if (IZoneManager::WorldToZonePos(pos) != get_zone()) {
            ZDOManager()->_InvalidateZDOZone(this->smart_from_this());

            ZDOManager()->_RemoveFromSector(this->smart_from_this());
            this->_set_position(pos);//unrevised
            ZDOManager()->_AddZDOToZone(this->smart_from_this());
        } else {
            this->_set_position(pos);
        }

        assert(IZoneManager::WorldToZonePos(pos) == this->get_zone());

        if (this->owned_by_me())
            this->_revise();
    }
}

ZoneID ZDO::get_zone() const
{
    return IZoneManager::WorldToZonePos(this->get_position());
}

void ZDO::pack(DataWriter &writer, bool network) const
{
    bool hasRot = this->m_rotation != Vector3f::ZERO;

    std::uint16_t flags {};

    if (this->is_persistent())
        flags |= 1 << NETWORK_Persistent;
    if (this->is_distant())
        flags |= 1 << NETWORK_Distant;
    flags |= std::to_underlying(this->get_type()) << NETWORK_Type1;
    if (hasRot)
        flags |= 1 << NETWORK_Rotation;

    auto const flagPos = writer.get_pos();
    writer.write(flags);// dummy spacer
    if (!network) {
        writer.write(this->get_zone());
        writer.write(this->get_position());
    }
    writer.write(this->get_prefab_hash());
    if (hasRot) {
        writer.write(this->m_rotation);
    }

    if (network) {
        auto &&find = PAIRED_CONNECTORS.find(this->m_id);
        if (find != PAIRED_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto &&connector = find->second;
            writer.write(connector.m_type);
            writer.write(connector.m_target);

            flags |= 1 << NETWORK_Connection;
        }
    } else {
        auto &&find = TYPED_CONNECTORS.find(this->m_id);
        if (find != TYPED_CONNECTORS.end() && find->second.m_type != ZDOConnector::Type::None) {
            auto &&connector = find->second;
            writer.write(connector.m_type);
            writer.write(connector.m_hash);

            flags |= 1 << NETWORK_Connection;
        }
    }

    if (this->_try_write_type<float>(writer))
        flags |= 1 << NETWORK_Float;
    if (this->_try_write_type<Vector3f>(writer))
        flags |= 1 << NETWORK_Vec3;
    if (this->_try_write_type<Quaternion>(writer))
        flags |= 1 << NETWORK_Quat;
    if (this->_try_write_type<std::int32_t>(writer))
        flags |= 1 << NETWORK_Int;
    if (this->_try_write_type<std::int64_t>(writer))
        flags |= 1 << NETWORK_Long;
    if (this->_try_write_type<std::string>(writer))
        flags |= 1 << NETWORK_String;
    if (this->_try_write_type<avledet::util::Bytes>(writer))
        flags |= 1 << NETWORK_ByteArray;

    auto const endPos = writer.get_pos();
    writer.set_pos(flagPos);
    writer.write(flags);
    writer.set_pos(endPos);
}
