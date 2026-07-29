#pragma once

#include "HeightMap.h"
#include "Manager.h"
#include "TerrainModifier.h"
#include "Types.h"
#include "VUtils.h"

#if AVL_IS_ON(AVL_ZONE_GENERATION)

class HeightmapManager : public avledet::util::IManager<HeightmapManager>
{
    //avledet::util::Set<ZoneID> m_population;
    avledet::util::Map<ZoneID, std::unique_ptr<Heightmap>> m_heightmaps;

  public:
    //void ForceGenerateAll();
    void ForceQueuedRegeneration();

    float GetOceanDepthAll(Vector3f worldPos);

    bool AtMaxLevelDepth(Vector3f worldPos);

    //Vector3f GetNormal(const Vector3f& pos);

    // Get the heightmap height at position
    //	Will only succeed given the heightmap exists
    // TODO make this return the height, or throw if out of bounds of entire square world
    //bool GetHeight(const Vector3f& worldPos, float& height);
    //bool GetAverageHeight(const Vector3f& worldPos, float radius, float &height);


    // Get the heightmap height at position,
    //	The heightmap will be created if it does not exist
    //float GetHeight(const Vector3f& worldPos);

    //static std::vector<Heightmap> GetAllHeightmaps();
    avledet::util::Map<ZoneID, std::unique_ptr<Heightmap>> &GetAllHeightmaps();

    //Heightmap* GetOrCreateHeightmap(const Vector2i& zoneID);

    Heightmap *PollHeightmap(ZoneID zone);

    Heightmap &GetHeightmap(Vector3f point);
    Heightmap &GetHeightmap(ZoneID zone);
    std::vector<Heightmap *> GetHeightmaps(Vector3f point, float radius);
    //avledet::util::Biome FindBiome(const Vector3f& point);

    bool IsRegenerateQueued(Vector3f point, float radius);

    //Heightmap* CreateHeightmap(const Vector2i& zone);
};

#endif// AVL_IS_ON(AVL_ZONE_GENERATION)
