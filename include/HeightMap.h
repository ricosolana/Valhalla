#pragma once

#include "Vector.h"
#include "VUtils.h"
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
    //friend class HeightmapManager;

    //using Heights = std::array<float, (Heightmap::WIDTH + 1)* (Heightmap::WIDTH + 1)>;

    

    //void Awake();
    //void OnDestroy();
    //void OnEnable();



    //void UpdateCornerDepths();
    //void Generate();

    float Distance(float x, float y, float rx, float ry);
    void ApplyModifiers();
    void ApplyModifier(TerrainModifier modifier, BaseHeightmap::Heights_t *levelOnly);
    Vector3 CalcVertex(int32_t x, int32_t y);
    void RebuildCollisionMesh();
    void SmoothTerrain2(const Vector3& worldPos, float radius, BaseHeightmap::Heights_t* levelOnlyHeights, float power);
    bool AtMaxWorldLevelDepth(const Vector3& worldPos);
    bool GetWorldBaseHeight(const Vector3& worldPos, float& height);
    
    bool GetAverageWorldHeight(const Vector3& worldPos, float radius, float &height);
    bool GetMinWorldHeight(const Vector3& worldPos, float radius, float &height);
    bool GetMaxWorldHeight(const Vector3& worldPos, float radius, float &height);
    void SmoothTerrain(const Vector3& worldPos, float radius, bool square, float intensity);
    float GetAvgHeight(int32_t cx, int32_t cy, int32_t w);
    float GroundHeight(const Vector3& point);
    void FindObjectsToMove(Vector3 worldPos, float area, std::vector<Rigidbody> &objects);
    void PaintCleared(Vector3 worldPos, float radius, TerrainModifier::PaintType paintType, bool heightCheck);
    void WorldToNormalizedHM(const Vector3& worldPos, float& x, float &y);
    void LevelTerrain(const Vector3& worldPos, float radius, bool square, BaseHeightmap::Heights_t* levelOnly);

public:
    Heightmap(const ZoneID& zone, std::unique_ptr<BaseHeightmap> base);

    //Heightmap(const Heightmap& other) = delete; // delete copy

    //void QueueRegenerate();
    //bool IsRegenerateQueued();
    void Regenerate();
    std::array<float, 4> &GetOceanDepth();

    

    float GetOceanDepth(const Vector3& worldPos);
    std::vector<Biome> GetBiomes();
    bool HaveBiome(Biome biome);
    Biome GetBiome(const Vector3& point);
    BiomeArea GetBiomeArea();
    bool IsBiomeEdge();

    // client command only
    //bool CheckTerrainModIsContained(TerrainModifier modifier);

    bool TerrainVSModifier(TerrainModifier modifier);

    
    // Should use an array independent from paintmask
    // only the alpha is used
    float GetVegetationMask(const Vector3& worldPos);
    bool IsCleared(const Vector3& worldPos);
    bool IsCultivated(const Vector3& worldPos);

    // Get the relative vertex of a world position to this heightmap
    //  Heightmap is treated as the center
    void WorldToVertex(const Vector3& worldPos, int32_t& x, int32_t &y);

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
    bool GetWorldHeight(const Vector3& worldPos, float& height);

    // Get the underlying height in builder heights array
    //  x, y must be within [0, 63]
    //  otherwise 0 is returned
    float GetBaseHeight(int32_t x, int32_t y);
    void SetHeight(int32_t x, int32_t y, float h);
    bool IsPointInside(const Vector3& point, float radius = 0);


    bool GetWorldNormal(const Vector3& worldPos, Vector3& normal);


    TerrainComp GetAndCreateTerrainCompiler();
    
    //Vector3 GetWorldPosition();

public:

    Material m_material;

private:
    //void CancelQueuedRegeneration();
    

    //Task *m_queuedRegenerateTask = nullptr;

    const std::unique_ptr<BaseHeightmap> m_base;
    BaseHeightmap::Heights_t m_heights;

    BaseHeightmap::Mask_t m_paintMask;

    MeshCollider m_collider;

    std::array<float, 4> m_oceanDepth{};

    std::array<Biome, 4> m_cornerBiomes = {
        Biome::Meadows,
        Biome::Meadows,
        Biome::Meadows,
        Biome::Meadows
    };

    Mesh m_collisionMesh;

    Mesh m_renderMesh;

    const ZoneID m_zone;
};
