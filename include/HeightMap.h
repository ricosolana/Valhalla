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
    float get_distance(float x, float y, float rx, float ry);
    Vector3f calc_vertex(int32_t x, int32_t y);

    void world_to_heightmap(Vector3f worldPos, float& x, float &y);

public:
    Heightmap(ZoneID zone, std::unique_ptr<BaseHeightmap> base);

    ZoneID get_zone() const {
        return m_zone;
    }

    void regenerate();
    std::array<float, 4> &get_ocean_depth();
    float get_ocean_depth(Vector3f worldPos);
    std::vector<Biome> get_biomes();
    bool contains_biome(Biome biome);
    Biome get_biome(Vector3f point);
    BiomeArea get_biome_area();
    bool is_biome_edge();
    
    // Should use an array independent from paintmask
    // only the alpha is used
    float get_vegetation_mask(Vector3f worldPos);

    // Get the relative vertex of a world position to this heightmap
    //  Heightmap is treated as the center
    void world_to_vertex(Vector3f worldPos, int32_t& x, int32_t &y);

    // Get the underlying height in heights array
    //  x, y must be within [0, 63]
    //  otherwise 0 is returned
    float get_height(int32_t x, int32_t y);

    // Get the underlying height in heights array at world position
    //  Returns false if the position outside of this heightmap
    bool get_world_height(const Vector3f& worldPos, float& height);

    // Get the underlying height in builder heights array
    //  x, y must be within [0, 63]
    //  otherwise 0 is returned
    float get_base_height(int32_t x, int32_t y);

    bool get_world_normal(Vector3f worldPos, Vector3f& normal);
};
#endif