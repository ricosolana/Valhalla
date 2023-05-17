#include <thread>

#include "NetManager.h"
#include "ZoneManager.h"
#include "PrefabManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"
#include "Hashes.h"
#include "ZDOManager.h"

auto ZONE_MANAGER(std::make_unique<IZoneManager>()); // TODO stop constructing in global
IZoneManager* ZoneManager() {
    return ZONE_MANAGER.get();
}



// private
void IZoneManager::PostPrefabInit() {
    LOG_INFO(LOGGER, "Initializing ZoneManager");
    
    RouteManager()->Register(Hashes::Routed::C2S_SetGlobalKey, [this](Peer* peer, std::string_view name) {
        // TODO constraint check
        if (m_globalKeys.insert(name).second)
            SendGlobalKeys(); // Notify clients
    });

    RouteManager()->Register(Hashes::Routed::C2S_RemoveGlobalKey, [this](Peer* peer, std::string_view name) {
        // TODO constraint check
        if (m_globalKeys.erase(name))
            SendGlobalKeys(); // Notify clients
    });

    RouteManager()->Register(Hashes::Routed::C2S_RequestIcon, [this](Peer* peer, std::string_view locationName, Vector3f point, std::string_view pinName, int pinType, bool showMap) {
        if (auto&& instance = GetNearestFeature(locationName, point)) {
            LOG_INFO(LOGGER, "Found location: '{}'", locationName);
            RouteManager()->Invoke(peer->m_uuid, 
                Hashes::Routed::S2C_ResponseIcon, 
                pinName, 
                pinType, 
                instance->m_pos, 
                showMap
            );
        }
        else {
            LOG_INFO(LOGGER, "Failed to find location: '{}'", locationName);
        }
    });

    RouteManager()->Register(Hashes::Routed::S2C_ResponsePing, [](Peer* peer, float time) {
        peer->Route(Hashes::Routed::Pong, time);
    });
}

// private
void IZoneManager::OnNewPeer(Peer& peer) {
    SendGlobalKeys(peer);
    SendLocationIcons(peer);
}

bool IZoneManager::ZonesOverlap(ZoneID zone, Vector3f refPoint) {
    return ZonesOverlap(zone,
        WorldToZonePos(refPoint));
}

bool IZoneManager::ZonesOverlap(ZoneID zone, ZoneID refCenterZone) {
    int num = NEAR_ACTIVE_AREA - 1;
    return zone.x >= refCenterZone.x - num
        && zone.x <= refCenterZone.x + num
        && zone.y <= refCenterZone.y + num
        && zone.y >= refCenterZone.y - num;
}

bool IZoneManager::IsPeerNearby(ZoneID zone, OWNER_t uid) {
    auto&& peer = NetManager()->GetPeerByUUID(uid);
    //assert((peer && uid) || (!peer && uid)); // makes sure no peer is ever found with 0 uid
    if (peer) return ZonesOverlap(zone, peer->m_pos);
    return false;
}

// private
void IZoneManager::SendGlobalKeys() {
    RouteManager()->InvokeAll(Hashes::Routed::S2C_UpdateKeys, m_globalKeys);
}

void IZoneManager::SendGlobalKeys(Peer& peer) {
    //RouteManager()->Invoke(peer, Hashes::Routed::S2C_UpdateKeys, m_globalKeys);
    peer.Route(Hashes::Routed::S2C_UpdateKeys, m_globalKeys);
}

// private
void IZoneManager::SendLocationIcons() {
    BYTES_t bytes;
    DataWriter writer(bytes);

    auto&& icons = GetFeatureIcons();

    writer.Write<int32_t>(icons.size());
    for (auto&& instance : icons) {
        writer.Write(instance.get().m_pos);
        writer.Write(std::string_view(instance.get().m_feature.get().m_name));
    }

    RouteManager()->InvokeAll(Hashes::Routed::S2C_UpdateIcons, bytes);
}

// private
void IZoneManager::SendLocationIcons(Peer& peer) {
    LOG_INFO(LOGGER, "Sending location icons to {}", peer.m_name);

    BYTES_t bytes;
    DataWriter writer(bytes);

    auto&& icons = GetFeatureIcons();

    writer.Write<int32_t>(icons.size());
    for (auto&& instance : icons) {
        writer.Write(instance.get().m_pos);
        writer.Write(std::string_view(instance.get().m_feature.get().m_name));
    }

    //RouteManager()->Invoke(peer, Hashes::Routed::S2C_UpdateIcons, bytes);

    peer.Route(Hashes::Routed::S2C_UpdateIcons, bytes);
}

// public
void IZoneManager::Save(DataWriter& pkg) {
    pkg.Write(m_generatedZones);
    pkg.Write(VConstants::PGW);
    pkg.Write(VConstants::LOCATION);
    pkg.Write(m_globalKeys);
    pkg.Write(true);
    pkg.Write<int32_t>(m_generatedFeatures.size());
    for (auto&& pair : m_generatedFeatures) {
        auto&& inst = pair.second;
        auto&& location = inst->m_feature.get();

        pkg.Write(std::string_view(location.m_name));
        pkg.Write(inst->m_pos);
        pkg.Write(m_generatedZones.contains(WorldToZonePos(inst->m_pos)));
    }
}

// public
void IZoneManager::Load(DataReader& reader, int32_t version) {
    m_generatedZones = reader.Read<decltype(m_generatedZones)>();

    if (version >= 13) {
        const auto pgwVersion = reader.Read<int32_t>(); // 99
        const auto locationVersion = (version >= 21) ? reader.Read<int32_t>() : 0; // 26
        if (pgwVersion != VConstants::PGW)
            LOG_WARNING(LOGGER, "Loading unsupported pgw version");

        if (version >= 14) {
            m_globalKeys = reader.Read<decltype(m_globalKeys)>();

#ifndef ELPP_DISABLE_VERBOSE_LOGS
            //VLOG(1) << "global keys: " << (this->m_globalKeys.empty() ? "none" : "");
            for (auto&& key : this->m_globalKeys) {
                //VLOG(1) << " - " << key;
            }
#endif

            if (version >= 18) {
                if (version >= 20) reader.Read<bool>();

                const auto countLocations = reader.Read<int32_t>();
                for (int i = 0; i < countLocations; i++) {
                    auto text = reader.Read<std::string_view>();
                    auto pos = reader.Read<Vector3f>();
                    bool generated = (version >= 19) ? reader.Read<bool>() : false;

                    auto&& location = GetFeature(text);
                    if (location) {
                        m_generatedFeatures[WorldToZonePos(pos)] = 
                            std::make_unique<Feature::Instance>(*location, pos);
                    }
                    else {
                        LOG_ERROR(LOGGER, "Failed to find location {}", text);
                    }
                }

                LOG_INFO(LOGGER, "Loaded {} ZoneLocation instances", countLocations);
                if (pgwVersion != VConstants::PGW) {
                  m_generatedFeatures.clear();
                }
            }
        }
    }
}

// private
const IZoneManager::Feature* IZoneManager::GetFeature(HASH_t hash) {
    auto&& find = m_featuresByHash.find(hash);
    if (find != m_featuresByHash.end())
        return &find->second.get();

    return nullptr;
}

// private
const IZoneManager::Feature* IZoneManager::GetFeature(std::string_view name) {
    return GetFeature(VUtils::String::GetStableHashCode(name));
}

// public
// call from within ZNet.init or earlier...
void IZoneManager::PostGeoInit() {
    // Will be empty if world failed to load
    if (!m_generatedFeatures.empty())
        return;

    // Crucially important Location
    // So check that it exists period
    auto&& spawnLoc = m_featuresByHash.find(VUtils::String::GetStableHashCodeCT("StartTemple"));
    if (spawnLoc == m_featuresByHash.end())
        throw std::runtime_error("World spawnpoint missing (StartTemple)");

    if (!VH_SETTINGS.worldFeatures) {
        LOG_WARNING(LOGGER, "Location generation is disabled");
        PrepareFeatures(spawnLoc->second);
    }
    else {
#ifdef VH_OPTION_ENABLE_ZONE_GENERATION
        auto now(steady_clock::now());

        // Already presorted by priority
        for (auto&& loc : m_features) {
            PrepareFeatures(*loc.get());
        }

        LOG_INFO(LOGGER, "Location generation took {}s", duration_cast<seconds>(steady_clock::now() - now).count());
#endif
    }
}

bool IZoneManager::HaveLocationInRange(const Feature& loc, Vector3f p) {
    for (auto&& pair : m_generatedFeatures) {
        auto&& locationInstance = pair.second;
        auto&& location = locationInstance->m_feature.get();

        if ((location == loc 
            || (!loc.m_group.empty() && loc.m_group == location.m_group)) 
            && locationInstance->m_pos.Distance(p) < loc.m_minDistanceFromSimilar) // TODO use sqdist
        {
            return true;
        }
    }
    return false;
}

// public
// TODO make this batch update every time a new location is added or whatever
std::list<std::reference_wrapper<IZoneManager::Feature::Instance>> IZoneManager::GetFeatureIcons() {
    std::list<std::reference_wrapper<IZoneManager::Feature::Instance>> result;

    for (auto&& pair : m_generatedFeatures) {
        auto&& instance = pair.second;
        auto&& location = instance->m_feature.get();

        auto zone = WorldToZonePos(instance->m_pos);
        if (location.m_iconAlways
            || (location.m_iconPlaced && m_generatedZones.contains(zone)))
        {
            result.push_back(*instance.get());
        }
    }

    return result;
}

// public
IZoneManager::Feature::Instance* IZoneManager::GetNearestFeature(std::string_view name, Vector3f point) {
    float closestDist = std::numeric_limits<float>::max();
    
    IZoneManager::Feature::Instance* closest = nullptr;

    for (auto&& pair : m_generatedFeatures) {
        auto&& instance = pair.second;
        auto&& location = instance->m_feature.get();

        float dist = instance->m_pos.SqDistance(point);
        if (location.m_name == name && dist < closestDist) {
            closestDist = dist;
            closest = instance.get();
        }
    }

    return closest;
}

// public
// this is world position to zone position
// formerly GetZone
Vector2i IZoneManager::WorldToZonePos(Vector3f point) {
    int32_t x = floor((point.x + (float)ZONE_SIZE / 2.f) / (float)ZONE_SIZE);
    int32_t y = floor((point.z + (float)ZONE_SIZE / 2.f) / (float)ZONE_SIZE);
    return Vector2i(x, y);
}

// public
// zone position to ~world position
// GetZonePos
Vector3f IZoneManager::ZoneToWorldPos(ZoneID id) {
    return Vector3f(id.x * ZONE_SIZE, 0, id.y * ZONE_SIZE);
}
