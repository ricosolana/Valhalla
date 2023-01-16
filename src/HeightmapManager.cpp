#include "HeightmapManager.h"

//static std::vector<float> 
// only ever used locally
//static float[] tempBiomeWeights = new float[513]; // redundant; why not use a much smaller array?

// only ever used locally
//static std::vector<Heightmap> tempHmaps; // use map instead?

//static robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>, HashUtils::Hasher>




// public static
void HeightmapManager::ForceQueuedRegeneration() {
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
float HeightmapManager::GetOceanDepthAll(const Vector3& worldPos) {
    auto&& heightmap = FindHeightmap(worldPos);
    if (heightmap) {
        return heightmap->GetOceanDepth(worldPos);
    }
    return 0;
}

// public static
bool HeightmapManager::AtMaxLevelDepth(const Vector3& worldPos) {
    auto heightmap = FindHeightmap(worldPos);
    return heightmap && heightmap->AtMaxWorldLevelDepth(worldPos);
}

// public static
bool HeightmapManager::GetHeight(const Vector3& worldPos, float& height) {
    auto heightmap = FindHeightmap(worldPos);
    if (heightmap && heightmap->GetWorldHeight(worldPos, height)) {
        return true;
    }
    height = 0;
    return false;
}

// public static
bool HeightmapManager::GetAverageHeight(const Vector3& worldPos, float& radius, float height) {
    std::vector<Heightmap*> list;
    FindHeightmap(worldPos, radius, list);

    float num = 0;
    int32_t num2 = 0;

    for (auto&& hmap : list) {
        float num3;
        if (hmap->GetAverageWorldHeight(worldPos, radius, num3)) {
            num += num3;
            num2++;
        }
    }



    //using (std::vector<Heightmap>.Enumerator enumerator = list.GetEnumerator()) {
    //    while (enumerator.MoveNext()) {
    //        float num3;
    //        if (enumerator.Current.GetAverageWorldHeight(worldPos, radius, num3)) {
    //            num += num3;
    //            num2++;
    //        }
    //    }
    //}

    if (num2 > 0) {
        height = num / (float)num2;
        return true;
    }
    height = 0;
    return false;
}


// public static
robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>>& HeightmapManager::GetAllHeightmaps() {
    return m_heightmaps;
}

// public static
Heightmap* HeightmapManager::FindHeightmap(const Vector3& point) {
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->IsPointInside(point, 0)) {
            return heightmap.get();
        }
    }
    return nullptr;
}

// public static
void HeightmapManager::FindHeightmap(const Vector3& point, float radius, std::vector<Heightmap*>& heightmaps) {
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->IsPointInside(point, radius)) {
            heightmaps.push_back(heightmap.get());
        }
    }
}

// public static
Heightmap::Biome HeightmapManager::FindBiome(const Vector3& point) {
    auto heightmap = FindHeightmap(point);
    if (heightmap) {
        return heightmap->GetBiome(point);
    }
    return Heightmap::Biome::None;
}

// public static
bool HeightmapManager::IsRegenerateQueued(const Vector3& point, float radius) {
    //tempHmaps.clear();
    std::vector<Heightmap*> heightmaps;
    FindHeightmap(point, radius, heightmaps);
    for (auto&& hmap : heightmaps) {
        if (hmap->IsRegenerateQueued())
            return true;
    }



    //using (std::vector<Heightmap>.Enumerator enumerator = Heightmap.tempHmaps.GetEnumerator()) {
    //    while (enumerator.MoveNext()) {
    //        if (enumerator.Current.HaveQueuedRebuild()) {
    //            return true;
    //        }
    //    }
    //}
    return false;
}
