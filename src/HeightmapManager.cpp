#include "HeightmapManager.h"
#include "ZoneManager.h"

//static std::vector<float> 
// only ever used locally
//static float[] tempBiomeWeights = new float[513]; // redundant; why not use a much smaller array?

// only ever used locally
//static std::vector<Heightmap> tempHmaps; // use map instead?

//static robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>, HashUtils::Hasher>

auto HEIGHTMAP_MANAGER(std::make_unique<IHeightmapManager>());
IHeightmapManager* HeightmapManager() {
    return HEIGHTMAP_MANAGER.get();
}


// public static
void IHeightmapManager::ForceQueuedRegeneration() {
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->IsRegenerateQueued()) {
            LOG(INFO) << "Force generating hmap " << heightmap->m_zone.x << " " << heightmap->m_zone.y;
            //LOG(INFO) << "Force generating hmap " << heightmap.transform.position.ToString();
            heightmap->Regenerate();
        }
    }
}

// public static 
float IHeightmapManager::GetOceanDepthAll(const Vector3& worldPos) {
    auto&& heightmap = GetHeightmap(worldPos);
    if (heightmap) {
        return heightmap->GetOceanDepth(worldPos);
    }
    return 0;
}

// public static
bool IHeightmapManager::AtMaxLevelDepth(const Vector3& worldPos) {
    auto heightmap = GetHeightmap(worldPos);
    return heightmap && heightmap->AtMaxWorldLevelDepth(worldPos);
}

// public static
bool IHeightmapManager::GetHeight(const Vector3& worldPos, float& height) {
    auto heightmap = GetHeightmap(worldPos);
    if (heightmap && heightmap->GetWorldHeight(worldPos, height)) {
        return true;
    }
    height = 0;
    return false;
}

float IHeightmapManager::GetHeight(const Vector3& worldPos) {
    auto heightmap = GetOrCreateHeightmap(IZoneManager::WorldToZonePos(worldPos));

    float height = 0;
    if (heightmap->GetWorldHeight(worldPos, height)) {
        return height;
    }

    throw std::runtime_error("something went wrong with Heightmap");
}

// public static
bool IHeightmapManager::GetAverageHeight(const Vector3& worldPos, float& radius, float height) {
    std::vector<Heightmap*> list = GetHeightmaps(worldPos, radius);

    float num = 0;
    int32_t num2 = 0;

    for (auto&& hmap : list) {
        float num3;
        if (hmap->GetAverageWorldHeight(worldPos, radius, num3)) {
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


// public static
robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>>& IHeightmapManager::GetAllHeightmaps() {
    return m_heightmaps;
}

Heightmap* IHeightmapManager::GetOrCreateHeightmap(const Vector2i& zoneID) {
    //for (auto&& pair : m_heightmaps) {
    //    auto&& heightmap = pair.second;
    //    if (heightmap->IsPointInside(point, 0)) {
    //        return heightmap.get();
    //    }
    //}

    auto&& pair = m_heightmaps.insert({ zoneID, std::make_unique<Heightmap>(zoneID) });
    if (pair.second) // if newly inserted
        pair.first->second->Regenerate();

    return pair.first->second.get();
    
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
}

// public static
Heightmap* IHeightmapManager::GetHeightmap(const Vector3& point) {
    //for (auto&& pair : m_heightmaps) {
    //    auto&& heightmap = pair.second;
    //    if (heightmap->IsPointInside(point, 0)) {
    //        return heightmap.get();
    //    }
    //}

    auto zone = IZoneManager::WorldToZonePos(point);
    auto&& find = m_heightmaps.find(zone);
    if (find != m_heightmaps.end())
        return find->second.get();

    return nullptr;
}

// public static
std::vector<Heightmap*> IHeightmapManager::GetHeightmaps(const Vector3& point, float radius) {
    std::vector<Heightmap*> heightmaps;
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->IsPointInside(point, radius)) {
            heightmaps.push_back(heightmap.get());
        }
    }
    return heightmaps;
}

// public static
Biome IHeightmapManager::FindBiome(const Vector3& point) {
    auto heightmap = GetHeightmap(point);
    if (heightmap) {
        return heightmap->GetBiome(point);
    }
    return Biome::None;
}

// public static
bool IHeightmapManager::IsRegenerateQueued(const Vector3& point, float radius) {
    auto heightmaps = GetHeightmaps(point, radius);
    for (auto&& hmap : heightmaps) {
        if (hmap->IsRegenerateQueued())
            return true;
    }
    return false;
}

//Heightmap* IHeightmapManager::CreateHeightmap(const Vector2i& zone) {
//    assert(!m_heightmaps.contains(zone) && "fix your code dummy");
//
//    auto h(new Heightmap(zone));
//    m_heightmaps[zone] = std::unique_ptr<Heightmap>(h);
//    return h;
//}
