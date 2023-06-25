#include "HeightmapManager.h"

#if VH_IS_ON(VH_ZONE_GENERATION)
#include "ZoneManager.h"
#include "HeightmapBuilder.h"

//static std::vector<float> 
// only ever used locally
//static float[] tempBiomeWeights = new float[513]; // redundant; why not use a much smaller array?

// only ever used locally
//static std::vector<Heightmap> tempHmaps; // use map instead?

//static robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>, HashUtils::Hasher>

auto HEIGHTMAP_MANAGER = std::make_unique<IHeightmapManager>();
IHeightmapManager* HeightmapManager() {
    return HEIGHTMAP_MANAGER.get();
}


// public static
void IHeightmapManager::ForceQueuedRegeneration() {
    assert(false);
    //for (auto&& pair : m_heightmaps) {
    //    auto&& heightmap = pair.second;
    //    if (heightmap->IsRegenerateQueued()) {
    //        //LOG(INFO) << "Force generating hmap " << heightmap->m_zone.x << " " << heightmap->m_zone.y;
    //        ////LOG(INFO) << "Force generating hmap " << heightmap.transform.position.ToString();
    //        heightmap->Regenerate();
    //    }
    //}
}

// public static 
float IHeightmapManager::GetOceanDepthAll(Vector3f worldPos) {
    auto&& heightmap = GetHeightmap(worldPos);
    return heightmap.GetOceanDepth(worldPos);
}

// public static
bool IHeightmapManager::AtMaxLevelDepth(Vector3f worldPos) {
    auto&& heightmap = GetHeightmap(worldPos);
    return heightmap.AtMaxWorldLevelDepth(worldPos);
}

/*
Vector3f IHeightmapManager::GetNormal(const Vector3f& pos) {

}*/

/*
// public static
bool IHeightmapManager::GetHeight(const Vector3f& worldPos, float& height) {
    auto&& heightmap = GetHeightmap(worldPos);
    if (heightmap.GetWorldHeight(worldPos, height)) {
        return true;
    }
    height = 0;
    return false;
}

float IHeightmapManager::GetHeight(const Vector3f& worldPos) {
    auto&& heightmap = GetHeightmap(IZoneManager::WorldToZonePos(worldPos));

    float height = 0;
    if (heightmap.GetWorldHeight(worldPos, height)) {
        return height;
    }

    assert(false);

    //throw std::runtime_error("Unexpected: Failed to get guaranteed heightmap at position (should not see this)");
    //LOG(ERROR) << "Failed to get guaranteed heightmap at position";
    exit(0);
}
*/

/*
// public static
bool IHeightmapManager::GetAverageHeight(const Vector3f& worldPos, float radius, float &height) {
    auto heightmaps = GetHeightmaps(worldPos, radius);

    float num = 0;
    int32_t num2 = 0;

    for (auto&& heightmap : heightmaps) {
        float num3;
        if (heightmap.GetAverageWorldHeight(worldPos, radius, num3)) {
            num += num3;
            num2++;
        }
    }

    if (num2 > 0) {
        height = num / (float)num2;
        return true;
    }
    height = 0;
    return false;
}
*/

// public static
UNORDERED_MAP_t<ZoneID, std::unique_ptr<Heightmap>>& IHeightmapManager::GetAllHeightmaps() {
    return m_heightmaps;
}

/*
Heightmap* IHeightmapManager::GetOrCreateHeightmap(const Vector2i& zoneID) {
    //for (auto&& pair : m_heightmaps) {
    //    auto&& heightmap = pair.second;
    //    if (heightmap->IsPointInside(point, 0)) {
    //        return heightmap.get();
    //    }
    //}

    throw std::runtime_error("do not use");

    ////auto&& pair = m_heightmaps.insert({ zoneID, nullptr });
    ////if (pair.second) { // if newly inserted
    ////    pair.first->second = std::make_unique<Heightmap>(zoneID);
    ////    pair.first->second->Regenerate();
    ////}
    ////
    ////assert(pair.first->second);
    ////
    ////return pair.first->second.get();
    
    //auto&& find = m_heightmaps.find(zone);
    //if (find != m_heightmaps.end()) {
    //    return find->second.get();
    //}
    //
    //return m_heightmaps.insert()
    //
    //auto h(new Heightmap(zone));
    //m_heightmaps[zone] = std::unique_ptr<Heightmap>(h);
    //return h;
    //
    //auto heightmap = GetHeightmap(point);
    //if (heightmap)
    //    return heightmap;
    //
    //return CreateHeightmap(IZoneManager::WorldToZonePos(point));
}*/

Heightmap* IHeightmapManager::PollHeightmap(ZoneID zone) {
    auto&& insert = m_heightmaps.insert({ zone, nullptr });
    //auto&& pop = m_population.insert(zone);
    //if (!insert.first->second && pop.second) // if heightmap is null, try polling it
    if (!insert.first->second)
        insert.first->second = HeightmapBuilder()->PollHeightmap(zone);

    return insert.first->second.get();
}

// public static
Heightmap& IHeightmapManager::GetHeightmap(Vector3f point) {
    return GetHeightmap(IZoneManager::WorldToZonePos(point));
}

Heightmap& IHeightmapManager::GetHeightmap(ZoneID zone) {
    auto&& insert = m_heightmaps.insert({ zone, nullptr });

    while (!insert.first->second) { // if heightmap is null, try polling it
        insert.first->second = HeightmapBuilder()->PollHeightmap(zone);
        if (!insert.first->second) // small optimize
            std::this_thread::sleep_for(1ms);
    }

    return *insert.first->second;
}

// public static
std::vector<Heightmap*> IHeightmapManager::GetHeightmaps(Vector3f point, float radius) {
    throw std::runtime_error("not implemented");
    std::vector<Heightmap*> heightmaps;
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->IsPointInside(point, radius)) {
            heightmaps.push_back(heightmap.get());
        }
    }
    return heightmaps;
}

/*
// public static
Biome IHeightmapManager::FindBiome(const Vector3f& point) {
    auto &&heightmap = GetHeightmap(point);
    if (heightmap) {
        return heightmap->GetBiome(point);
    }
    return Biome::None;
}*/

// public static
bool IHeightmapManager::IsRegenerateQueued(Vector3f point, float radius) {
    assert(false);
    return false;
    //auto heightmaps = GetHeightmaps(point, radius);
    //for (auto&& hmap : heightmaps) {
    //    if (hmap->IsRegenerateQueued())
    //        return true;
    //}
    //return false;
}

//Heightmap* IHeightmapManager::CreateHeightmap(const Vector2i& zone) {
//    assert(!m_heightmaps.contains(zone) && "fix your code dummy");
//
//    auto h(new Heightmap(zone));
//    m_heightmaps[zone] = std::unique_ptr<Heightmap>(h);
//    return h;
//}
#endif