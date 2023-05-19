#include <thread>

#include "NetManager.h"
#include "ZoneManager.h"
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
                instance->second, 
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
void IZoneManager::SendLocationIcons(Peer& peer) {
    LOG_INFO(LOGGER, "Sending location icons to {}", peer.m_name);

    peer.SubInvoke(Hashes::Routed::S2C_UpdateIcons, [this](DataWriter& writer) {
        const auto PRE = writer.Position();

        uint32_t count = 0;
        writer.Write(count); // dummy
        for (auto&& pair : m_generatedFeatures) {
            // If StartTemple
            //  or Haldor is generated, then send

            // I am knowingly doing string pointer comparisons here so shut up
            if (pair.first == FEATURE_START_TEMPLE
                || (pair.first == FEATURE_HALDOR && IsZoneGenerated(WorldToZonePos(pair.second))))
            {
                writer.Write(pair.second);
                writer.Write(pair.first);
                count++;

                // Only Haldor is extra, so could break early
                //if (count == 2) {
                    //break;
                //}
            }
        }

        const auto POST = writer.Position();
        writer.SetPos(PRE);
        writer.Write(count);
        writer.SetPos(POST);
    }); 
}

bool IZoneManager::IsZoneGenerated(ZoneID zone) {
    return ZDOManager()->m_objectsByZone.contains(zone);
}

// public
void IZoneManager::Save(DataWriter& pkg) {
    //for (auto&& pair : )
    /*
    for (auto&& zone : ZDOManager()->
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
    }*/
}

// public
void IZoneManager::Load(DataReader& reader, int32_t version) {
    // Skip generated zones
    //reader.Read<std::list<ZoneID>>();
    if (auto count = reader.Read<uint32_t>()) {
        for (decltype(count) i = 0; i < count; i++) {
            reader.Read<ZoneID>();
        }
    }    

    if (version >= 13) {
        const auto pgwVersion = reader.Read<int32_t>(); // 99
        const auto locationVersion = (version >= 21) ? reader.Read<int32_t>() : 0; // 26
        if (pgwVersion != VConstants::PGW)
            LOG_WARNING(LOGGER, "Loading unsupported pgw version");

        if (version >= 14) {
            m_globalKeys = reader.Read<decltype(m_globalKeys)>();

            if (version >= 18) {
                if (version >= 20) reader.Read<bool>();

                const auto countLocations = reader.Read<int32_t>();
                for (int i = 0; i < countLocations; i++) {
                    auto text = reader.Read<std::string_view>();
                    auto pos = reader.Read<Vector3f>();
                    bool generated = (version >= 19) ? reader.Read<bool>() : false;

                    if (text == FEATURE_START_TEMPLE)
                        m_generatedFeatures.emplace_back(FEATURE_START_TEMPLE, pos);
                    else if (text == FEATURE_HALDOR)
                        m_generatedFeatures.emplace_back(FEATURE_HALDOR, pos);
                    else if (text == FEATURE_EIKTHYR)
                        m_generatedFeatures.emplace_back(FEATURE_EIKTHYR, pos);
                    else if (text == FEATURE_ELDER)
                        m_generatedFeatures.emplace_back(FEATURE_ELDER, pos);
                    else if (text == FEATURE_BONEMASS)
                        m_generatedFeatures.emplace_back(FEATURE_BONEMASS, pos);
                    else if (text == FEATURE_MODER)
                        m_generatedFeatures.emplace_back(FEATURE_MODER, pos);
                    else if (text == FEATURE_YAGLUTH)
                        m_generatedFeatures.emplace_back(FEATURE_YAGLUTH, pos);
                    else if (text == FEATURE_QUEEN)
                        m_generatedFeatures.emplace_back(FEATURE_QUEEN, pos);
                }

                LOG_INFO(LOGGER, "Loaded {} ZoneLocation instances", countLocations);
            }
        }
    }
}

// public
IZoneManager::Instance* IZoneManager::GetNearestFeature(std::string_view name, Vector3f point) {
    float closestDist = std::numeric_limits<float>::max();    
    IZoneManager::Instance* closest = nullptr;

    for (auto&& instance : m_generatedFeatures) {
        float dist = instance.second.SqDistance(point);
        if (name == instance.first && dist < closestDist) {
            closestDist = dist;
            closest = &instance;
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
