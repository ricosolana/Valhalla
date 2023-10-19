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

auto HEIGHTMAP_MANAGER(std::make_unique<IHeightmapManager>());
IHeightmapManager* HeightmapManager() {
    return HEIGHTMAP_MANAGER.get();
}



Heightmap* IHeightmapManager::poll(ZoneID zone) {
    auto&& insert = m_heightmaps.insert({ zone, nullptr });
    //auto&& pop = m_population.insert(zone);
    //if (!insert.first->second && pop.second) // if heightmap is null, try polling it
    if (!insert.first->second)
        insert.first->second = HeightmapBuilder()->poll(zone);

    return insert.first->second.get();
}

// public static
Heightmap& IHeightmapManager::get_heightmap(Vector3f point) {
    return get_heightmap(IZoneManager::WorldToZonePos(point));
}

Heightmap& IHeightmapManager::get_heightmap(ZoneID zone) {
    auto&& insert = m_heightmaps.insert({ zone, nullptr });

    while (!insert.first->second) { // if heightmap is null, try polling it
        insert.first->second = HeightmapBuilder()->poll(zone);
        if (!insert.first->second) // small optimize
            std::this_thread::sleep_for(1ms);
    }

    return *insert.first->second;
}
#endif