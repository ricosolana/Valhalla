#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_ZONE_GENERATION)
#include "Vector.h"

#include "HashUtils.h"
#include "ValhallaServer.h"
#include "TerrainModifier.h"
//#include "HMBuildData.h"
#include "ZoneManager.h"

class IHeightmapManager;

class Rigidbody {};
class TerrainComp {};
class Material {};
//class Texture2D {};
class MeshCollider {};
class Mesh {};

class BaseHeightmap {
public:
    using Heights_t = std::vector<float>;
    using Mask_t = std::vector<Color>;

public:
    std::array<Biome, 4> m_cornerBiomes;
    Heights_t m_baseHeights;
    //Mask_t m_baseMask;
    std::vector<float> m_vegMask;
};

class Heightmap {
    friend class IHeightmapManager;

public:
    static constexpr Color m_paintMaskDirt = Colors::RED;
    static constexpr Color m_paintMaskCultivated = Colors::GREEN;
    static constexpr Color m_paintMaskPaved = Colors::BLUE;
    static constexpr Color m_paintMaskNothing = Colors::BLACK;

    //static constexpr int WIDTH = 64;

    static constexpr float m_levelMaxDelta = 8;

    static constexpr const float m_smoothMaxDelta = 1;

    //11/30/2022 11:24:44: m_isDistantLod: False
    //11/30/2022 11:24:44: m_width: 64
    //11/30/2022 11:24:44: m_scale: 1
    //11/30/2022 11:24:44: m_distantLodEditorHax: False

    static constexpr int E_WIDTH = IZoneManager::ZONE_SIZE + 1;

private:
    const std::unique_ptr<BaseHeightmap> m_base;
    BaseHeightmap::Heights_t m_heights;

    BaseHeightmap::Mask_t m_paintMask;

    std::array<float, 4> m_oceanDepth{};

    std::array<Biome, 4> m_cornerBiomes = {
        Biome::Meadows,
        Biome::Meadows,
        Biome::Meadows,
        Biome::Meadows
    };

    const ZoneID m_zone;

private:
    float Distance(float x, float y, float rx, float ry);
    void ApplyModifiers();
    void ApplyModifier(TerrainModifier modifier, BaseHeightmap::Heights_t *levelOnly);
    avledet::util::CSU::Vector3f CalcVertex(int32_t x, int32_t y);
    void RebuildCollisionMesh();
    void SmoothTerrain2(avledet::util::CSU::Vector3f worldPos, float radius, BaseHeightmap::Heights_t* levelOnlyHeights, float power);
    bool AtMaxWorldLevelDepth(avledet::util::CSU::Vector3f worldPos);
    bool GetWorldBaseHeight(avledet::util::CSU::Vector3f worldPos, float& height);
    
    bool GetAverageWorldHeight(avledet::util::CSU::Vector3f worldPos, float radius, float &height);
    bool GetMinWorldHeight(avledet::util::CSU::Vector3f worldPos, float radius, float &height);
    bool GetMaxWorldHeight(avledet::util::CSU::Vector3f worldPos, float radius, float &height);
    void SmoothTerrain(avledet::util::CSU::Vector3f worldPos, float radius, bool square, float intensity);
    float GetAvgHeight(int32_t cx, int32_t cy, int32_t w);
    float GroundHeight(avledet::util::CSU::Vector3f point);
    void FindObjectsToMove(avledet::util::CSU::Vector3f worldPos, float area, std::vector<Rigidbody> &objects);
    void PaintCleared(avledet::util::CSU::Vector3f worldPos, float radius, TerrainModifier::PaintType paintType, bool heightCheck);
    void WorldToNormalizedHM(avledet::util::CSU::Vector3f worldPos, float& x, float &y);
    void LevelTerrain(avledet::util::CSU::Vector3f worldPos, float radius, bool square, BaseHeightmap::Heights_t* levelOnly);

public:
    Heightmap(ZoneID zone, std::unique_ptr<BaseHeightmap> base);

    ZoneID GetZone() {
        return m_zone;
    }

    //Heightmap(const Heightmap& other) = delete; // delete copy

    //void QueueRegenerate();
    //bool IsRegenerateQueued();
    void Regenerate();
    std::array<float, 4> &GetOceanDepth();

    

    float GetOceanDepth(avledet::util::CSU::Vector3f worldPos);
    std::vector<Biome> GetBiomes();
    bool HaveBiome(Biome biome);
    Biome GetBiome(avledet::util::CSU::Vector3f point);
    BiomeArea GetBiomeArea();
    bool IsBiomeEdge();

    // client command only
    //bool CheckTerrainModIsContained(TerrainModifier modifier);

    bool TerrainVSModifier(TerrainModifier modifier);

    
    // Should use an array independent from paintmask
    // only the alpha is used
    float GetVegetationMask(avledet::util::CSU::Vector3f worldPos);
    bool IsCleared(avledet::util::CSU::Vector3f worldPos);
    bool IsCultivated(avledet::util::CSU::Vector3f worldPos);

    // Get the relative vertex of a world position to this heightmap
    //  Heightmap is treated as the center
    void WorldToVertex(avledet::util::CSU::Vector3f worldPos, std::int32_t& x, std::int32_t &y);

    // Get the underlying color mask in paint array
    //  x, y must be within [0, 63]
    //  otherwise 0 is returned
    Color GetPaintMask(int32_t x, int32_t y);

    // Get the underlying height in heights array
    //  x, y must be within [0, 63]
    //  otherwise 0 is returned
    float GetHeight(int32_t x, int32_t y);

    // Get the underlying height in heights array at world position
    //  Returns false if the position outside of this heightmap
    bool GetWorldHeight(const avledet::util::CSU::Vector3f& worldPos, float& height);

    // Get the underlying height in builder heights array
    //  x, y must be within [0, 63]
    //  otherwise 0 is returned
    float GetBaseHeight(int32_t x, int32_t y);
    void SetHeight(int32_t x, int32_t y, float h);
    bool IsPointInside(avledet::util::CSU::Vector3f point, float radius = 0);


    bool GetWorldNormal(avledet::util::CSU::Vector3f worldPos, avledet::util::CSU::Vector3f& normal);

    // TOOD either implement or remove
    TerrainComp GetAndCreateTerrainCompiler();
};
#endif