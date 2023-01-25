#pragma once

#include "Vector.h"
#include "VUtils.h"
#include "HashUtils.h"
#include "ValhallaServer.h"
#include "TerrainModifier.h"
#include "HMBuildData.h"

class IHeightmapManager;

class Rigidbody {};
class TerrainComp {};
class Material {};
//class Texture2D {};
class MeshCollider {};
class Mesh {};



class Heightmap {
    friend class IHeightmapManager;
    friend class HMBuildData;

public:
    static constexpr Color m_paintMaskDirt = Colors::RED;
    static constexpr Color m_paintMaskCultivated = Colors::GREEN;
    static constexpr Color m_paintMaskPaved = Colors::BLUE;
    static constexpr Color m_paintMaskNothing = Colors::BLACK;

    static constexpr int WIDTH = 64;

    static constexpr float m_levelMaxDelta = 8;

    static constexpr const float m_smoothMaxDelta = 1;

    //11/30/2022 11:24:44: m_isDistantLod: False
    //11/30/2022 11:24:44: m_width: 64
    //11/30/2022 11:24:44: m_scale: 1
    //11/30/2022 11:24:44: m_distantLodEditorHax: False

private:
    //friend class HeightmapManager;

    //using Heights = std::array<float, (Heightmap::WIDTH + 1)* (Heightmap::WIDTH + 1)>;

    

    //void Awake();
    //void OnDestroy();
    //void OnEnable();



    void UpdateCornerDepths();
    void Generate();
    float Distance(float x, float y, float rx, float ry);
    void ApplyModifiers();
    void ApplyModifier(TerrainModifier modifier, HMBuildData::Heights_t *levelOnly);
    Vector3 CalcVertex(int32_t x, int32_t y);
    void RebuildCollisionMesh();
    void SmoothTerrain2(const Vector3& worldPos, float radius, HMBuildData::Heights_t* levelOnlyHeights, float power);
    bool AtMaxWorldLevelDepth(const Vector3& worldPos);
    bool GetWorldBaseHeight(const Vector3& worldPos, float& height);
    bool GetWorldHeight(const Vector3& worldPos, float& height);
    bool GetAverageWorldHeight(const Vector3& worldPos, float radius, float &height);
    bool GetMinWorldHeight(const Vector3& worldPos, float radius, float &height);
    bool GetMaxWorldHeight(const Vector3& worldPos, float radius, float &height);
    void SmoothTerrain(const Vector3& worldPos, float radius, bool square, float intensity);
    float GetAvgHeight(int32_t cx, int32_t cy, int32_t w);
    float GroundHeight(const Vector3& point);
    void FindObjectsToMove(Vector3 worldPos, float area, std::vector<Rigidbody> &objects);
    void PaintCleared(Vector3 worldPos, float radius, TerrainModifier::PaintType paintType, bool heightCheck);
    void WorldToNormalizedHM(const Vector3& worldPos, float& x, float &y);
    void LevelTerrain(const Vector3& worldPos, float radius, bool square, HMBuildData::Heights_t* levelOnly);

public:
    Heightmap(const Vector2i& zone);

    void QueueRegenerate();
    bool IsRegenerateQueued();
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
    void WorldToVertex(const Vector3& worldPos, int32_t& x, int32_t &y);
    Color GetPaintMask(int32_t x, int32_t y);
    float GetHeight(int32_t x, int32_t y);
    float GetBaseHeight(int32_t x, int32_t y);
    void SetHeight(int32_t x, int32_t y, float h);
    bool IsPointInside(const Vector3& point, float radius = 0);



    TerrainComp GetAndCreateTerrainCompiler();
    
    Vector3 GetWorldPosition();

public:

    //int32_t m_width = 32;

    //float m_scale = 1;

    Material m_material;


    //bool m_isDistantLod;

    //bool m_distantLodEditorHax;

private:
    void CancelQueuedRegeneration();
    

    Task *m_queuedRegenerateTask = nullptr;

    //std::vector<float> m_heights;

    //Heights m_heights{};

    HMBuildData::Heights_t m_heights;

    std::unique_ptr<HMBuildData> m_buildData;

    //Texture2D m_paintMask;
    //std::vector<float>
    //std::vector<TerrainModifier::PaintType> m_paintMask;
    HMBuildData::Mask_t m_paintMask;

    // seems to only be used for gpu texture sets
    //Material m_materialInstance;

    MeshCollider m_collider;

    //float m_oceanDepth[4] = { 0 };

    std::array<float, 4> m_oceanDepth{};

    std::array<Biome, 4> m_cornerBiomes = {
        Biome::Meadows,
        Biome::Meadows,
        Biome::Meadows,
        Biome::Meadows
    };

    Mesh m_collisionMesh;

    Mesh m_renderMesh;

    //bool m_dirty;

    Vector2i m_zone;

public:

};

