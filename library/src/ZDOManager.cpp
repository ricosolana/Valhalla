
#include <cassert>
#include <cstdint>
#include <quill/LogMacros.h>
#include <stdexcept>

#include <range/v3/all.hpp>
#include <utility>

#include "Avledet.h"
#include "Hashes.h"
#include "NetManager.h"
#include "RouteManager.h"
#include "VUtils.h"
#include "ZDO.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

auto ZDO_MANAGER = std::make_unique<IZDOManager>();

IZDOManager *ZDOManager()
{
    return ZDO_MANAGER.get();
}

void IZDOManager::Init()
{
    m_nextUid = 1;

    LOG_NOTICE(AVL_LOGGER, "Initializing ZDOManager");

    RouteManager()->Register(avledet::util::hashes::Routed::DestroyZDO, [this](Peer::Ptr, DataReader reader) {
        // TODO constraint check
        reader.read([this](ZDOID zdoid) { EraseZDO(zdoid); });
    });

    //auto&& insert = ZDOManager()->m_objectsByID.begin()->second->
    //m_members.insert({0, ZDO::Ord()});
    //insert.first->second.Get
    RouteManager()->Register(avledet::util::hashes::Routed::C2S_RequestZDO,
                             [this](Peer::Ptr peer, ZDOID id) { peer->ForceSendZDO(id); });
}

void IZDOManager::Update()
{
    ZoneScoped;

    if (VUtils::run_periodic_now<struct periodic_zdo_stats>(3min)) {
        LOG_INFO(AVL_LOGGER, "Currently {} zdos (~{:0.02f}MB -> ~{:0.02f}MB)", m_objectsByID.size(),
                 //(this->GetTotalZDOAlloc() / 1000000.f),
                 ZDO::get_memory(false) / 1000000.f, ZDO::get_memory(true) / 1000000.f);
    }

    //assert(std::accumulate(m_objectsByPrefab.begin(), m_objectsByPrefab.end(), (std::size_t)0,
    //	[](std::size_t value, const decltype(m_objectsByPrefab)::value_type& v) -> std::size_t {
    //		return value + v.second.size();
    //	}
    //) == m_objectsByID.size());

    // TODO requires testing
    //	link portals if mode enabled
#if AVL_IS_ON(AVL_PORTAL_LINKING)
    if (VUtils::run_periodic<struct link_portals>(1s)) {
        auto &&portals = GetZDOs(avledet::util::hashes::Object::portal_wood);

        // TODO use the optimized Lua ported code for linking portals
        //	not the exact code but the way the algo works

        auto &&FindRandomUnconnectedPortal = [&](ZDOID skip, std::string_view tag) -> ZDO::optional {
            std::vector<ZDO::reference> list;
            for (auto &&zdo : portals) {
                if (zdo->get_id() != skip
                    && zdo->get_string(avledet::util::hashes::ZDO::TeleportWorld::TAG) == tag
                    && !zdo->get_connection_zdoid(ZDOConnector::Type::Portal)) {
                    list.push_back(zdo);
                }
            }

            if (list.empty()) {
                return ZDO::nullopt;
            }

            return ZDO::make_optional(list[VUtils::Random::State().range(0, list.size())]);
        };

        for (auto &&zdo : portals) {
            auto &&connectionZDOID = zdo->get_connection_zdoid(ZDOConnector::Type::Portal);
            auto &&string          = zdo->get_string(avledet::util::hashes::ZDO::TeleportWorld::TAG);
            if (connectionZDOID) {
                auto &&zdo2 = find_zdo(connectionZDOID);
                if (!zdo2 || zdo2->get_string(avledet::util::hashes::ZDO::TeleportWorld::TAG) != string) {
                    zdo->claim();
                    zdo->set_connection(ZDOConnector::Type::Portal, ZDOID::NONE);
                    ForceSendZDO(zdo->get_id());
                }
            }
        }

        for (auto &&zdo3 : portals) {
            if (!zdo3->get_connection_zdoid(ZDOConnector::Type::Portal)) {
                auto &&string2 = zdo3->get_string(avledet::util::hashes::ZDO::TeleportWorld::TAG);
                auto &&zdo4    = FindRandomUnconnectedPortal(zdo3->get_id(), string2);
                if (zdo4) {
                    zdo3->claim();
                    zdo4->claim();
                    zdo3->set_connection(ZDOConnector::Type::Portal, zdo4->get_id());
                    zdo4->set_connection(ZDOConnector::Type::Portal, zdo3->get_id());
                    ForceSendZDO(zdo3->get_id());
                    ForceSendZDO(zdo4->get_id());
                    //zdo3.Apply();
                }
            }
        }
    }
#endif


    auto &&peers = NetManager()->GetPeers();

    if (VUtils::run_periodic<struct zdos_release_assign>(AVL_SETTINGS.zdoAssignInterval)) {
        for (auto &&peer : peers) {
            if (!peer->IsGated()) {
                AssignOrReleaseZDOs(peer);
            }
        }
    }

    if (VUtils::run_periodic<struct periodic_send_zdos>(AVL_SETTINGS.zdoSendInterval)) {
        for (auto &&peer : peers) {
            SendZDOs(peer, false);
        }
    }

    if (!m_destroySendList.empty()) {

        // TODO make a member variable?
        //	think about emulated zdo containers (like replaying actions to specific peers)
        //	this is a functionality I might be planning on into the future
        DataWriter writer;//.write(m_destroySendList);
        writer.write(m_destroySendList);

        m_destroySendList.clear();

        RouteManager()->InvokeAll(avledet::util::hashes::Routed::DestroyZDO, std::move(writer.get_buf()));
    }
}

void IZDOManager::_AddZDOToZone(ZDO::reference zdo)
{
    auto &&container = _GetZDOContainer(zdo->get_zone());

    assert(!container.get().contains(zdo)
           && "\nSomething wrong with ZDO; possibilities: \n - zoning encapsulation is broken \n - ZDOID "
              "equal operator is broken");

    auto &&insert = container.get().insert(zdo);

    assert(insert.second);//ensure newly inserted

                          //LOG_INFO(AVL_LOGGER, "zdo added to zone: {} {}", zdo->get_id(), zdo->get_zone());
}

void IZDOManager::_RemoveFromSector(ZDO::reference zdo)
{
    if (auto &&container = _FindZDOContainer(zdo->get_zone())) {
        auto &&erase = container->erase(zdo);

        // ensure zdo was actually erased
        assert(erase);

        //LOG_INFO(AVL_LOGGER, "zdo removed from zone: {} {}", zdo->get_id(), zdo->get_zone());
    } else {
        //TODO otherwise, then poll the sector map
        assert(false);
    }
}

void IZDOManager::_InvalidateZDOZone(ZDO::reference zdo)
{
    for (auto &&peer : NetManager()->GetPeers()) {
        peer->ZDOSectorInvalidated(zdo);
    }
}

void IZDOManager::Save(DataWriter &writer)
{
    //pkg.write(Avledet()->ID());
    writer.write((std::int64_t) 0);
    writer.write(m_nextUid);

    {
        // Write zdos (persistent)
        auto const start = writer.get_pos();

        std::int32_t count = 0;
        writer.write(count);

        for (auto &&zdo : m_objectsByID) {
            if (zdo->is_persistent()) {
                zdo->pack(writer, false);
                count++;
            }
        }

        auto const end = writer.get_pos();
        writer.set_pos(start);
        writer.write(count);
        writer.set_pos(end);
    }
}

void IZDOManager::Load(DataReader &reader, int version)
{
    reader.read<std::int64_t>(); // server id
    reader.read<std::uint32_t>();// next uid

    auto count = reader.read<std::uint32_t>();
    for (decltype(count) i = 0; i < count; i++) {
        auto &&insert
                = _Instantiate(version < 31 ? reader.read<ZDOID>() : ZDOID(0, ZDOManager()->m_nextUid++));

        //auto&& zdo = ZDO(*insert.first);
        //auto&& zdo = insert
        auto &&zdo = ZDO::make_reference(insert.first);

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
        if (version < 31) {
            auto zdoReader = DataReader(reader.read<std::vector<char>>());

            zdo->Load31Pre(zdoReader, version);
        } else
#endif// AVL_LEGACY_WORLD_LOADING
        {
            zdo->unpack(reader, version);
        }

        _AddZDOToZone(zdo);

        //auto&& prefab = zdo.get_prefab();

        m_objectsByPrefab[zdo->get_prefab_hash()].insert(zdo);

        //assert(std::accumulate(m_objectsByPrefab.begin(), m_objectsByPrefab.end(), (std::size_t)0,
        //	[](std::size_t value, const decltype(m_objectsByPrefab)::value_type& v) -> std::size_t {
        //		return value + v.second.size();
        //	}
        //) == m_objectsByID.size());

#if AVL_IS_ON(AVL_DUNGEON_REGENERATION)
        if (prefab.AllFlagsPresent(Prefab::Flag::DUNGEON)) {
            // Only add real sky dungeon
            if (zdo->get_position().y > 4000)
                DungeonManager()->m_dungeonInstances.push_back(zdo->get_id());
        }
#endif

        //m_objectsByID[zdo->get_id()] = std::move(zdo);
    }

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
    if (version < 31) {
        auto deadCount = reader.read<std::int32_t>();
        for (decltype(deadCount) j = 0; j < deadCount; j++) {
            reader.read<std::int64_t>();
            reader.read<std::uint32_t>();
            reader.read<std::int64_t>();
        }

        // Owners, Terrains, and Seeds have already been converted

        // convert portals
        for (auto &&zdo : GetZDOs(avledet::util::hashes::Object::portal_wood)) {
            auto &&string = zdo->get_string(avledet::util::hashes::ZDO::TeleportWorld::TAG);
            ZDOID zdoid;
            zdo->extract("target", zdoid);
            if (zdoid && !string.empty()) {
                auto &&zdo2 = find_zdo(zdoid);
                if (zdo2) {
                    auto &&string2 = zdo2->get_string(avledet::util::hashes::ZDO::TeleportWorld::TAG);
                    ZDOID zdoid2;
                    zdo2->extract("target", zdoid2);
                    if (string == string2 && zdoid == zdo2->get_id() && zdoid2 == zdo->get_id()) {
                        zdo->claim();
                        zdo2->claim();
                        zdo->set_connection(ZDOConnector::Type::Portal, zdo2->get_id());
                        zdo2->set_connection(ZDOConnector::Type::Portal, zdo->get_id());
                        //zdo.Apply();
                    }
                }
            }
        }

        // convert spawners
        for (auto &&zdo : GetZDOs(Prefab::Flag::CREATURE_SPAWNER, Prefab::Flag::NONE)) {
            zdo->claim();
            ZDOID zdoid;
            zdo->extract("spawn_id", zdoid);
            auto &&zdo2 = find_zdo(zdoid);
            zdo->set_connection(ZDOConnector::Type::Spawned, zdo2 ? zdo2->get_id() : ZDOID::NONE);
            //zdo.Apply();
        }

        // convert sync transforms
        for (auto &&zdo : GetZDOs(Prefab::Flag::SYNCED_TRANSFORM, Prefab::Flag::NONE)) {
            zdo->claim();
            ZDOID zdoid;
            zdo->extract("parentID", zdoid);
            auto &&zdo2 = find_zdo(zdoid);
            if (zdo2) {
                zdo->set_connection(ZDOConnector::Type::Spawned, zdo2->get_id());
                //zdo.Apply();
            } else {
                //zdo.m_pack.Set<ZDO::FLAGS_PACK_INDEX>(
                // zero out connector bit
                //zdo.m_pack.Get<ZDO::FLAGS_PACK_INDEX>() & static_cast<std::uint32_t>(~ZDO::LocalFlag::Member_Connection)
                //);
            }
        }
    }
#endif// AVL_LEGACY_WORLD_LOADING

    LOG_NOTICE(AVL_LOGGER, "Loaded {} zdos", m_objectsByID.size());
}

[[nodiscard]] std::pair<ZDO::smart_set::iterator, bool> IZDOManager::_Instantiate(ZDOID zdoid) noexcept
{
    //https://jguegant.github.io/blogs/tech/performing-try-emplace.html
    //auto &&insert = m_objectsByID.insert(std::make_unique<ZDO>(zdoid));
    auto contained = m_objectsByID.contains(zdoid);//TODO dbg test
    if (contained) {
        if (true) {
            volatile int x = 0;
        }
    }

    auto &&insert = m_objectsByID.insert(ZDO::make_smart(zdoid));

    // if newly inserted, but ALREADY CONTAINED???
    if (insert.second && contained) {
        assert(false);
    }

    //LOG_INFO(AVL_LOGGER, "zdo instantiated: {} {}", insert.second, zdoid);

    return insert;
}

std::pair<ZDO::smart_set::iterator, bool> IZDOManager::_Instantiate(ZDOID zdoid, Vector3f position) noexcept
{
    auto &&insert = _Instantiate(zdoid);

    // if inserted, then set pos
    if (insert.second) {
        //auto&& zdo = ZDO(*insert.first);
        auto &&zdo = ZDO::make_reference(insert.first);

        // Set zone and position of ZDO
        zdo->_set_position(position);
        _AddZDOToZone(zdo);
    }

    return insert;
}

ZDO::reference IZDOManager::_Instantiate(Vector3f position) noexcept
{
    //ZDOID zdoid = ZDOID(AVL_ID, 0);
    ZDOID zdoid = ZDOID(0, 0);
    for (;;) {
        zdoid.set_id(m_nextUid++);
        auto &&insert = _Instantiate(zdoid, position);
        if (insert.second)
            return ZDO::make_reference(insert.first);
    }
    std::unreachable();
}

//unused
//ZDO::unsafe_value IZDOManager::_TryInstantiate(ZDOID uid, Vector3f position) {
//	// See version #2
//	// ...returns a pair object whose first element is an iterator
//	//		pointing either to the newly inserted element in the
//	//		container or to the element whose key is equivalent...
//	// https://cplusplus.com/reference/unordered_map/unordered_map/insert/
//
//	auto&& insert = _Instantiate(uid, position);
//	if (insert.second)
//		return ZDO::make_unsafe_value(insert.first);
//
//	throw std::runtime_error("zdo already exists");
//}


ZDO::optional IZDOManager::find_zdo(ZDOID id)
{
    if (id) {
        auto &&find = m_objectsByID.find(id);
        if (find != m_objectsByID.end())
            return ZDO::make_optional(find);
    }
    return ZDO::nullopt;
}

/*
std::pair<IZDOManager::ZDO_iterator, bool> IZDOManager::_GetOrInstantiate(ZDOID id, Vector3f def) {
	auto&& insert = _Instantiate(def, id);
	if (!insert.second)
		return insert;
	
	auto&& insert = m_objectsByID.insert({ id, nullptr });
	
	if (!insert.second) // if new insert failed, return it
		return insert;

	auto&& pair = insert.first;

	auto&& zdo = pair->second;

	zdo = std::make_unique<ZDO>(id, def);
	return insert;
}*/


ZDO::reference IZDOManager::Instantiate(Prefab const &prefab, Vector3f pos)
{
    auto &&zdo = _Instantiate(pos);
    //zdo.get().m_encoded.SetPrefabIndex(PrefabManager()->RequirePrefabIndexByHash(prefab.m_hash));
    //zdo.get().m_pack.Set<ZDO::PREFAB_PACK_INDEX>(PrefabManager()->RequirePrefabIndexByHash(prefab.m_hash));
    zdo->_set_prefab_hash(prefab.m_hash);
    if (prefab.AllFlagsPresent(Prefab::Flag::SYNC_INITIAL_SCALE)) {
        zdo->set_local_scale(prefab.m_localScale, false);
    }

    return zdo;
}

//unused
//ZDO::unsafe_value IZDOManager::Instantiate(avledet::util::Hash hash, Vector3f pos, const Prefab** outPrefab) {
//	//auto&& zdo = Instantiate()
//	auto&& prefab = PrefabManager()->RequirePrefabByHash(hash);
//	if (outPrefab) *outPrefab = &prefab;
//
//	return Instantiate(prefab, pos);
//}

/*
ZDO::reference IZDOManager::Instantiate(const ZDO& zdo) {
	assert(false);

	auto&& copy = _Instantiate(zdo.get_position());
	
	//copy.get().m_encoded = zdo.m_encoded;
	//copy.get().m_pack = zdo.m_pack;
	copy.get().m_rotation = zdo.m_rotation;

	return copy;
}*/


void IZDOManager::AssignOrReleaseZDOs(Peer::Ptr peer)
{
    ZoneScoped;

    auto &&zone = IZoneManager::WorldToZonePos(peer->m_pos);

    ZDO::reference_list m_tempNearObjects;
    GetZDOs_Zone(zone, m_tempNearObjects);         // get zdos: zone, nearby
    GetZDOs_NeighborZones(zone, m_tempNearObjects);// get zdos: zone, nearby

    for (auto &&zdo : m_tempNearObjects) {
        if (zdo->is_persistent()) {
            if (zdo->is_owner(peer->GetUserID())) {
                // If peer no longer in area of zdo, unclaim zdo
                if (!ZoneManager()->ZonesOverlap(zdo->get_zone(), zone)) {
                    zdo->disown();
                }
            } else {
                // If ZDO no longer has owner, or the owner went far away,
                //  Then assign this new peer as owner
                if (!(zdo->has_owner() && ZoneManager()->IsPeerNearby(zdo->get_zone(), zdo->get_owner()))
                    && ZoneManager()->ZonesOverlap(zdo->get_zone(), zone)) {

                    zdo->set_owner(peer->GetUserID());
                }
            }
        }
    }

    if (AVL_SETTINGS.TEST_zdoAssignAlgorithm == AssignAlgorithm::DYNAMIC_RADIUS) {

        float minSqDist = std::numeric_limits<float>::max();
        Vector3f closestPos;

        // get the distance to the closest peer
        for (auto &&otherPeer : NetManager()->GetPeers()) {
            if (otherPeer == peer)
                continue;

            if (!ZoneManager()->IsPeerNearby(IZoneManager::WorldToZonePos(otherPeer->m_pos),
                                             peer->GetUserID()))
                continue;

            float sqDist = otherPeer->m_pos.sq_distance_to(peer->m_pos);
            if (sqDist < minSqDist) {
                minSqDist  = sqDist;
                closestPos = otherPeer->m_pos;

                if (minSqDist <= 12 * 12) {
                    break;
                }
            }
        }

        if (minSqDist != std::numeric_limits<float>::max() && minSqDist > 12 * 12) {
            // Get zdos immediate to this peer
            auto zdos = GetZDOs(peer->m_pos, std::sqrt(minSqDist) * 0.5f - 2.f);

            // Basically reassign zdos from another owner to me instead
            for (auto &&zdo : zdos) {
                if (zdo->is_persistent()
                    && zdo->get_position().sq_distance_to(closestPos)
                               > 12 * 12// Ensure the ZDO is far from the other player
                ) {
                    zdo->set_owner(peer->GetUserID());
                }
            }
        }
    }
}

ZDO::smart_set::iterator IZDOManager::_EraseZDO(ZDO::smart_set::iterator itr)
{
    assert(itr != m_objectsByID.end());

    auto &&zdo   = ZDO::make_reference(itr);
    auto &&zdoid = zdo->get_id();

    // update local next only if im the user who created the zdo
    if (zdoid.get_user_id() == AVL_ID) {
        this->m_nextUid = std::max(this->m_nextUid, zdoid.get_id() + 1);
    }

    //VLOG(2) << "Destroying zdo (" << zdo->get_prefab().m_name << ")";

    _RemoveFromSector(zdo);
    {
        auto &&pfind = m_objectsByPrefab.find(zdo->get_prefab_hash());
        if (pfind != m_objectsByPrefab.end())
            pfind->second.erase(zdoid);
    }

    // cleans up some zdos
    for (auto &&peer : NetManager()->GetPeers()) {
        peer->m_zdos.erase(zdoid);
    }

    m_erasedZDOs.insert(zdoid);

    //erase members
    ZDO::m_floats.erase(zdoid);
    ZDO::m_vec3.erase(zdoid);
    ZDO::m_quats.erase(zdoid);
    ZDO::m_ints.erase(zdoid);
    ZDO::m_longs.erase(zdoid);
    ZDO::m_strings.erase(zdoid);
    ZDO::m_byteArrays.erase(zdoid);

    ZDO::ZDO_OWNERS.erase(zdoid);
    ZDO::PAIRED_CONNECTORS.erase(zdoid);

    //LOG_INFO(AVL_LOGGER, "zdo erased: {}", zdoid);

    return m_objectsByID.erase(itr);
}

void IZDOManager::GetZDOs_ActiveZones(ZoneID zone, ZDO::reference_list &out, ZDO::reference_list &outDistant)
{
    // Add ZDOs from immediate sector
    GetZDOs_Zone(zone, out);

    // Add ZDOs from nearby zones
    GetZDOs_NeighborZones(zone, out);

    // Add ZDOs from distant zones
    GetZDOs_DistantZones(zone, outDistant);
}

void IZDOManager::GetZDOs_NeighborZones(ZoneID zone, ZDO::reference_list &sectorObjects)
{
    for (auto z = zone.y - IZoneManager::NEAR_ZRADIUS; z <= zone.y + IZoneManager::NEAR_ZRADIUS; z++) {
        for (auto x = zone.x - IZoneManager::NEAR_ZRADIUS; x <= zone.x + IZoneManager::NEAR_ZRADIUS; x++) {
            auto current = ZoneID(x, z);
            // Skip the center zone
            if (current == zone)
                continue;

            GetZDOs_Zone(current, sectorObjects);
        }
    }
}

void IZDOManager::GetZDOs_DistantZones(ZoneID zone, ZDO::reference_list &out)
{
    for (std::int16_t r = IZoneManager::NEAR_ZRADIUS + 1;
         r <= IZoneManager::NEAR_ZRADIUS + IZoneManager::DISTANT_ZRADIUS; r++) {
        for (std::int16_t x = zone.x - r; x <= zone.x + r; x++) {
            GetZDOs_Distant(ZoneID(x, zone.y - r), out);
            GetZDOs_Distant(ZoneID(x, zone.y + r), out);
        }
        for (std::int16_t y = zone.y - r + 1; y <= zone.y + r - 1; y++) {
            GetZDOs_Distant(ZoneID(zone.x - r, y), out);
            GetZDOs_Distant(ZoneID(zone.x + r, y), out);
        }
    }
}

std::list<std::pair<ZDO::reference, float>> IZDOManager::CreateSyncList(Peer::Ptr peer)
{
    auto zone = IZoneManager::WorldToZonePos(peer->m_pos);

    // Gather all updated ZDO's
    ZDO::reference_list zoneZDOs;
    ZDO::reference_list distantZDOs;
    GetZDOs_ActiveZones(zone, zoneZDOs, distantZDOs);

    std::list<std::pair<ZDO::reference, float>> result;

    // Prepare client-side outdated ZDO's
    auto const time(Avledet()->Time());
    for (auto &&zdo : zoneZDOs) {
        decltype(Peer::m_zdos)::iterator outItr;
        if (peer->IsOutdatedZDO(zdo, outItr)) {
            float weight = 150;
            if (outItr != peer->m_zdos.end())
                weight = std::min(time - outItr->second.second, 100.f) * 1.5f;

            result.push_back({zdo, zdo->get_position().sq_distance_to(peer->m_pos) - weight * weight});
        }
    }

    // Prioritize ZDO's
    result.sort([&](std::pair<ZDO::reference, float> const &first,
                    std::pair<ZDO::reference, float> const &second) {
        // Sort in rough order of:
        //	flag -> type/priority -> distance ASC -> age ASC

        // https://www.reddit.com/r/valheim/comments/mga1iw/understanding_the_new_networking_mechanisms_from/

        auto &&a = first.first;
        auto &&b = second.first;

        bool flag = a->get_type() == avledet::util::ObjectType::PRIORITIZED && a->has_owner()
                    && !a->is_owner(peer->GetUserID());
        bool flag2 = b->get_type() == avledet::util::ObjectType::PRIORITIZED && b->has_owner()
                     && !b->is_owner(peer->GetUserID());

        if (flag == flag2) {
            if ((flag && flag2) || a->get_type() == b->get_type()) {
                return first.second < second.second;
            } else
                // > (shows large trees first)
                // <  (shows smallest, then small trees first)
                // >= (shows small trees and large trees simultaneously)

                // the problem with seemingly slowly perceived network speed is not with the actual network,
                // but with the ZDOManager being bottlenecked by the expensive HeightmapBuilder
                //	(if Heightmap is not ready, vegetation cannot be generated -> ZDOs cannot be sent)
                //	this only applies to newly generated areas
                return a->get_type() >= b->get_type();
        } else {
            return flag;
        }
    });

    // Add a minimum amount of ZDOs
    if (result.size() < 10) {
        for (auto &&zdo2 : distantZDOs) {
            if (peer->IsOutdatedZDO(zdo2)) {
                result.push_back({zdo2, 0});
            }
        }
    }

    // Add forcible send ZDOs
    for (auto &&itr = peer->m_forceSend.begin(); itr != peer->m_forceSend.end();) {
        auto &&zdoid = *itr;
        auto zdo     = find_zdo(zdoid);
        if (zdo && peer->IsOutdatedZDO(zdo)) {
            result.push_front({zdo, 0});
            ++itr;
        } else {
            itr = peer->m_forceSend.erase(itr);
        }
    }

    return result;
}

void IZDOManager::GetZDOs_Zone(ZoneID zone, ZDO::reference_list &objects)
{
    if (auto &&container = _FindZDOContainer(zone)) {
        objects.insert(objects.end(), container->begin(), container->end());
    }
}

void IZDOManager::GetZDOs_Distant(ZoneID zone, ZDO::reference_list &objects)
{
    if (auto &&container = _FindZDOContainer(zone)) {
        for (auto &&zdo : *container) {
            if (zdo->is_distant()) {
                objects.push_back(zdo);
            }
        }
    }
}

ZDO::reference_list IZDOManager::GetZDOs(avledet::util::Hash prefab)
{
    ZDO::reference_list out;
    auto &&find = m_objectsByPrefab.find(prefab);
    if (find != m_objectsByPrefab.end()) {
        auto &&container = find->second;
        out.insert(out.end(), container.begin(), container.end());
    }
    return out;
}

ZDO::reference_list IZDOManager::GetZDOs(ZDO::Filter const &pred)
{
    ZDO::reference_list out;
    for (auto &&v : m_objectsByID) {
        auto &&zdo = ZDO::make_reference(v);
        if (!pred || pred(zdo)) {
            out.push_back(zdo);
        }
    }
    return out;
}

ZDO::reference_list IZDOManager::SomeZDOs(Vector3f const &pos, float radius, std::size_t max,
                                          ZDO::Filter const &pred)
{
    ZDO::reference_list out;

    float const sqRadius = radius * radius;

    auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
    auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));

    for (auto z = minZone.y; z <= maxZone.y; z++) {
        for (auto x = minZone.x; x <= maxZone.x; x++) {
            if (auto &&container = _FindZDOContainer(ZoneID(x, z))) {
                for (auto &&zdo : *container) {
                    if (zdo->get_position().sq_distance_to(pos) <= sqRadius && (!pred || pred(zdo))) {
                        if (max--)
                            out.push_back(zdo);
                        else
                            return out;
                    }
                }
            }
        }
    }

    return out;
}

ZDO::reference_list IZDOManager::SomeZDOs(ZoneID const &zone, std::size_t max, ZDO::Filter const &pred)
{
    ZDO::reference_list out;

    if (auto &&container = _FindZDOContainer(zone)) {
        for (auto &&zdo : *container) {
            if (!pred || pred(zdo)) {
                if (max--)
                    out.push_back(zdo);
                else
                    return out;
            }
        }
    }

    return out;
}

ZDO::optional IZDOManager::NearestZDO(Vector3f const &pos, float radius, ZDO::Filter const &pred)
{
    float minSqDist = radius * radius;

    ZDO::optional out;
    //float minSqDist = std::numeric_limits<float>::max();

    auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
    auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));

    for (auto z = minZone.y; z <= maxZone.y; z++) {
        for (auto x = minZone.x; x <= maxZone.x; x++) {
            if (auto &&container = _FindZDOContainer(ZoneID(x, z))) {
                for (auto &&zdo : *container) {
                    float sqDist = zdo->get_position().sq_distance_to(pos);
                    if (sqDist < minSqDist// Filter to closest ZDO
                        && (!pred || pred(zdo))) {
                        out       = zdo;
                        minSqDist = sqDist;
                    }
                }
            }
        }
    }

    return out;
}

// Global send
void IZDOManager::ForceSendZDO(ZDOID const &id)
{
    for (auto &&peer : NetManager()->GetPeers()) {
        peer->ForceSendZDO(id);
    }
}

bool IZDOManager::SendZDOs(Peer::Ptr peer, bool flush)
{
    ZoneScoped;

    auto sendQueueSize = (std::uint32_t) peer->m_socket->get_send_queue_size();

    // flushing forces a packet send
    auto const threshold = AVL_SETTINGS.zdoMaxCongestion;
    if (!flush && sendQueueSize > threshold)
        return false;

    // if very little space remaining, skip
    auto availableSpace = threshold - sendQueueSize;
    if (availableSpace < AVL_SETTINGS.zdoMinCongestion)
        return false;

    auto syncList = CreateSyncList(peer);

    // continue only if there are updated/invalid ZDOs to send
    if (syncList.empty() && peer->m_invalidSector.empty())
        return false;

    // TODO a better optimization would be to use write a special
    //	preserializer that prepends packet data to the peer-buffer
    //	to avoid a few buffer allocs
    //	this only matters if performance is upmost concern, which it is because c :>

    peer->SubInvoke(
            avledet::util::hashes::Rpc::ZDOData, [&peer, &syncList, availableSpace](DataWriter &writer) {
                writer.write(peer->m_invalidSector);

                auto const time = Avledet()->Time();

                for (auto &&itr = syncList.begin();
                     itr != syncList.end() && writer.size() <= availableSpace /* (1) */; itr++) {

                    // if size exceeded, break now
                    //  HEY DUMMY! look above (1), I already did this
                    //if (writer.size() > availableSpace) {
                    //    break;
                    //}

                    // copy is intentional
                    //  TODO copy is solely for LUA
                    //      - I've already decided that copies are tacky as fuck
                    //          the better alternative would be to use shared ptr zdos
                    //          the problem is shared_ptr has a lot of overhead
                    //          so use boost intrusive ptr, where bits will have to be used for refcount
                    auto zdo = itr->first;

                    peer->m_forceSend.erase(zdo->get_id());

                    if (!AVL_SCRIPT_EVENT(IScriptManager::Events::SendingZDO, peer, zdo)) {
                        continue;
                    }

                    writer.write(zdo->get_id());
                    writer.write(zdo->get_owner_rev());
                    writer.write(zdo->get_data_rev());
                    writer.write(zdo->get_owner());
                    writer.write(zdo->get_position());

                    writer.write([zdo](DataWriter &writer) { zdo->pack(writer, true); });

                    peer->m_zdos[zdo->get_id()] = {zdo->_get_revision(), time};
                }
                writer.write(ZDOID::NONE);// null terminator
            });

    if (!peer->m_invalidSector.empty() || !syncList.empty()) {
        peer->m_invalidSector.clear();

        return true;
    }

    return false;
}

void IZDOManager::OnNewPeer(Peer::Ptr peer)
{
    peer->Register(avledet::util::hashes::Rpc::ZDOData, [this](Peer::Ptr peer, DataReader reader) {
        ZoneScoped;

        // Only allow if normal mode
        if (peer->IsGated())
            return;

        //assert(false); //TODO
        reader.read([this](ZDOID zdoid) {
            if (auto zdo = find_zdo(zdoid))
                _InvalidateZDOZone(zdo);
        });

        auto time = Avledet()->Time();

        while (auto zdoid = reader.read<ZDOID>()) {
            auto owner_rev = reader.read<std::uint16_t>();          // owner revision
            auto data_rev  = reader.read<std::uint32_t>();          // data revision
            auto owner     = reader.read<std::int64_t>();           // owner
            auto pos       = reader.read<Vector3f>();               // position

            auto des = DataReader(reader.read<std::vector<char>>());// dont move this

            /*
			ZDO::Rev rev = { 
				.m_dataRev = dataRev, 
				.m_ownerRev = ownerRev, 
				.m_syncTime = time 
			};*/

            auto &&[zdo_itr, created] = this->_Instantiate(zdoid);
            auto &&zdo                = ZDO::make_reference(zdo_itr);

            assert(zdoid == zdo->get_id());

            if (!created) {
                // If the incoming data revision is at most older or equal to this revision, we do NOT need to deserialize
                //	(because the data will be the same, or at the worst case, it will be outdated)
                if (data_rev <= zdo->get_data_rev()) {

                    // If the owner has changed, keep a copy
                    if (owner_rev > zdo->get_owner_rev()) {
                        zdo->_set_owner(owner);
                        zdo->_get_revision().set_owner_rev(owner_rev);
                        peer->m_zdos[zdoid] = {ZDO::Rev(data_rev, owner_rev), time};
                    }
                    continue;
                }
            } else {
                assert((_FindZDOContainer(zdo->get_zone()))
                               ? !_FindZDOContainer(zdo->get_zone())->contains(zdoid)
                               : true);

                if (m_erasedZDOs.contains(zdoid)) {
                    m_destroySendList.push_back(zdoid);

                    m_objectsByID.erase(zdo_itr);
                    continue;
                }
            }

            // Also used as restore point if this ZDO breaks during deserialization
            //ZDO copy(zdo);

            //try {
            zdo->_set_owner(owner);
            zdo->_get_revision().set_data_rev(data_rev);
            zdo->_get_revision().set_owner_rev(owner_rev);

            // Unpack the ZDOs primary data
            zdo->unpack(des, 0);

            AVL_SCRIPT_EVENT(IScriptManager::Events::ZDOUnpacked, peer, zdo);

            // Only disperse through world if ZDO is new
            if (created) {
                //if (!AVL_SCRIPT_EVENT(IScriptManager::Events::ZDOCreated, peer, zdo)) {
                //	_EraseZDO(pair.first);
                //	continue;
                //}

                //zdo->_set_position(pos);//unrevised because is fresh zdo
                //_AddZDOToZone(zdo);

                m_objectsByPrefab[zdo->get_prefab_hash()].insert(zdo);
            } else {
                //if (!AVL_SCRIPT_EVENT(IScriptManager::Events::ZDOModified, peer, zdo, copy, pos)) {
                //	zdo = std::move(copy);
                //	continue;
                //}
            }

            zdo->set_position(pos);

            assert(_FindZDOContainer(zdo->get_zone()));

            assert(_FindZDOContainer(zdo->get_zone())->contains(zdo));

            peer->m_zdos[zdoid] = {zdo->_get_revision(), time};
            /*
			}
			catch (const std::runtime_error& e) {
				// erase the zdo from map
				if (created) // if the zdo was just created, throw it away
					EraseZDO(pair.first);
				else zdo = copy; // else, restore the ZDO to the prior revision

				// This will kick the malicious peer
				std::rethrow_exception(std::make_exception_ptr(e));
			}*/
        }
    });
}

void IZDOManager::OnPeerQuit(Peer::Ptr peer)
{
    for (auto &&itr = m_objectsByID.begin(); itr != m_objectsByID.end();) {
        auto &&zdo = ZDO::make_reference(itr);

        if (!zdo->is_persistent()
            && (!zdo->has_owner() || zdo->is_owner(peer->GetUserID())
                || !NetManager()->FindPeerByUserID(zdo->get_owner()))) {
            itr = _DestroyZDO(itr);
        } else
            ++itr;
    }
}

std::size_t IZDOManager::GetSumZDOMembers()
{
    std::size_t res = 0;
    for (auto &&zdo : m_objectsByID) {
        //res += zdo.second->m_members.size();
    }
    return res;
}

float IZDOManager::GetMeanZDOMembers()
{
    return (float) GetSumZDOMembers() / m_objectsByID.size();
}

float IZDOManager::GetStDevZDOMembers()
{
    float const mean = GetMeanZDOMembers();
    float const n    = m_objectsByID.size();

    float res = 0;
    for (auto &&zdo : m_objectsByID) {
        //res += std::pow((float)zdo.second->m_members.size() - mean, 2.f);
    }

    return std::sqrt(res / n);
}

std::size_t IZDOManager::GetTotalZDOAlloc()
{
    //std::size_t bytes = m_objectsByID.size() * sizeof(ZDO);
    //for (auto&& zdo : m_objectsByID) bytes += zdo->get_memory();
    //return bytes;
    return ZDO::get_memory(false);
}

std::size_t IZDOManager::GetCountEmptyZDOs()
{
    // so gather each ZDO member, and write how many of them are empty
    std::size_t count = 0;
    for (auto &&zdo : m_objectsByID) {
        auto alloc = zdo->get_memory(false);
        if (alloc == 0)
            count++;
    }
    return count;
}
