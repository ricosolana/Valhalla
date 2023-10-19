#include "HeightMap.h"

#if VH_IS_ON(VH_ZONE_GENERATION)
#include "HeightmapBuilder.h"
#include "ZoneManager.h"
#include "HeightmapManager.h"
#include "VUtilsMathf.h"
#include "VUtilsMath.h"

// private
//void Heightmap::Awake() {
//    //if (!this->m_isDistantLod) {
//    //m_heightmaps[this->m_zone] = this; // must externally manage heightmap; wont work safely
//    //}
//    m_collider = base.GetComponent<MeshCollider>();
//}
//
//// private
//void Heightmap::OnDestroy() {
//    m_heightmaps.erase(this);
//    if (this->m_materialInstance) {
//        UnityEngine.Object.DestroyImmediate(this->m_materialInstance);
//    }
//}
//
//// private
//void Heightmap::OnEnable() {
//    this->Initialize();
//    regenerate();
//}

// private
// This is essentially a one-off function once constructed
// so its use is kinda redundant continually in Generate()
//void Heightmap::Initialize() {
Heightmap::Heightmap(ZoneID zoneID, std::unique_ptr<BaseHeightmap> base)
    : m_zone(zoneID), m_base(std::move(base)) {
    regenerate();
}

// public
// Used to be the Regenerate() method
void Heightmap::regenerate() {
    //CancelQueuedRegeneration();

    //Generate();
    //UpdateCornerDepths();

    m_cornerBiomes = m_base->m_cornerBiomes;
    this->m_heights = m_base->m_baseHeights;

    this->m_paintMask.resize(m_base->m_vegMask.size());
    for (int i=0; i < m_base->m_vegMask.size(); i++)
        this->m_paintMask[i].a = m_base->m_vegMask[i];

    m_oceanDepth[0] = std::max(0.f, IZoneManager::WATER_LEVEL - get_height(0, IZoneManager::ZONE_SIZE));
    m_oceanDepth[1] = std::max(0.f, IZoneManager::WATER_LEVEL - get_height(IZoneManager::ZONE_SIZE, IZoneManager::ZONE_SIZE));
    m_oceanDepth[2] = std::max(0.f, IZoneManager::WATER_LEVEL - get_height(IZoneManager::ZONE_SIZE, 0));
    m_oceanDepth[3] = std::max(0.f, IZoneManager::WATER_LEVEL - get_height(0, 0));
}

/*
// private
void Heightmap::UpdateCornerDepths() {
    m_oceanDepth[0] = get_height(0, IZoneManager::ZONE_SIZE);
    m_oceanDepth[1] = get_height(IZoneManager::ZONE_SIZE, IZoneManager::ZONE_SIZE);
    m_oceanDepth[2] = get_height(IZoneManager::ZONE_SIZE, 0);
    m_oceanDepth[3] = get_height(0, 0);

    m_oceanDepth[0] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[0]);
    m_oceanDepth[1] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[1]);
    m_oceanDepth[2] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[2]);
    m_oceanDepth[3] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[3]);
}*/

// public
std::array<float, 4>& Heightmap::get_ocean_depth() {
    return this->m_oceanDepth;
}



// public
float Heightmap::get_ocean_depth(Vector3f worldPos) {
    int32_t num;
    int32_t num2;
    this->world_to_vertex(worldPos, num, num2);

    float t = (float)num / IZoneManager::ZONE_SIZE;
    float t2 = (float)num2 / IZoneManager::ZONE_SIZE;
    float a = VUtils::Mathf::Lerp(this->m_oceanDepth[3], this->m_oceanDepth[2], t);
    float b = VUtils::Mathf::Lerp(this->m_oceanDepth[0], this->m_oceanDepth[1], t);
    return VUtils::Mathf::Lerp(a, b, t2);
}



// public
std::vector<Biome> Heightmap::get_biomes() {
    std::vector<Biome> list;
    //Biome mask = None;
    auto mask = std::to_underlying(Biome::None);
    for (auto&& item : this->m_cornerBiomes) {
        auto other = std::to_underlying(item);
        // Checking against a mask is faster/simpler than iterating array
        if (!(mask & other)) {
            list.push_back(item);
            mask |= other;
        }
    }
    return list;
}

// public
bool Heightmap::contains_biome(Biome biome) {
    return (std::to_underlying(m_cornerBiomes[0]) & std::to_underlying(biome))
        || (std::to_underlying(m_cornerBiomes[1]) & std::to_underlying(biome))
        || (std::to_underlying(m_cornerBiomes[2]) & std::to_underlying(biome))
        || (std::to_underlying(m_cornerBiomes[3]) & std::to_underlying(biome));
}

// private
float Heightmap::get_distance(float x, float y, float rx, float ry) {
    float num = x - rx;
    float num2 = y - ry;

    // (sqrt(2) - sqrt(x ^ 2 + y ^ 2)) ^ 3
    // https://www.math3d.org/sL5gEdMjk

    float num4 = std::sqrtf(2) - VUtils::Math::Magnitude(num, num2);
    return num4 * num4 * num4;
}

// public
Biome Heightmap::get_biome(Vector3f point) {
    //if all biomes are the same, return same/any
    if (this->m_cornerBiomes[0] == this->m_cornerBiomes[1] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[2] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[3]) {
        return this->m_cornerBiomes[0];
    }

    float x = point.x;
    float z = point.z;
    this->world_to_heightmap(point, x, z);

    // Bitshift biome weight table
    // TODO re-normalize biome shifts to use all perfectly allocated ones (since n7 is skipped in enum)
    std::array<float, 10> tempBiomeWeights{};
    assert(m_cornerBiomes[0] != Biome::None
        && m_cornerBiomes[1] != Biome::None
        && m_cornerBiomes[2] != Biome::None
        && m_cornerBiomes[3] != Biome::None && "Got Biome::None biome");

    // subtract 1 from index because Biome::None is not counted
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[0])] += this->get_distance(x, z, 0, 0);
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[1])] += this->get_distance(x, z, 1, 0);
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[2])] += this->get_distance(x, z, 0, 1);
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[3])] += this->get_distance(x, z, 1, 1);

    Biome biome = Biome::None;
    float weight = std::numeric_limits<float>::min();
    for (unsigned int j = 0; j < tempBiomeWeights.size(); j++) {
        if (tempBiomeWeights[j] > weight) {
            biome = Biome(1 << j);
            weight = tempBiomeWeights[j];
        }
    }

    return biome;
}

// public
BiomeArea Heightmap::get_biome_area() {
    if (this->is_biome_edge()) {
        return BiomeArea::Edge;
    }
    return BiomeArea::Median;
}

// public
bool Heightmap::is_biome_edge() {
    return this->m_cornerBiomes[0] != this->m_cornerBiomes[1] 
        || this->m_cornerBiomes[0] != this->m_cornerBiomes[2] 
        || this->m_cornerBiomes[0] != this->m_cornerBiomes[3];
}

// private
Vector3f Heightmap::calc_vertex(int32_t x, int32_t y) {
    Vector3f a = Vector3f((float)IZoneManager::ZONE_SIZE * -0.5f, 0.f, (float)IZoneManager::ZONE_SIZE * -0.5f);

    // Poll heightmap height at x,z
    float y2 = this->m_heights[y * E_WIDTH + x];
    return a + Vector3f((float)x, y2, (float)y);
}



bool Heightmap::get_world_normal(Vector3f worldPos, Vector3f& normal) {
    //float y;
    Vector3f a = worldPos;
    if (!get_world_height(worldPos, a.y))
        return false;

    //float y1;
    Vector3f b = worldPos + Vector3f(1, 0, 0);
    if (!get_world_height(b, b.y)) {
        b = worldPos + Vector3f(-1, 0, 0);
        get_world_height(b, b.y);
    }

    Vector3f c = worldPos + Vector3f(0, 0, 1);
    if (!get_world_height(c, c.y)) {
        c = worldPos + Vector3f(0, 0, -1);
        get_world_height(c, c.y);
    }

    //return cross prod

    // make vectors locally relative
    b -= a;
    c -= a;

    normal = b.Cross(c).Normal();

    // if it points below the horizon
    if (normal.y < 0)
        normal *= -1; // flip it back up

    return true;
}

// private
bool Heightmap::get_world_height(const Vector3f& worldPos, float& height) {
    int32_t x;
    int32_t y;
    this->world_to_vertex(worldPos, x, y);

    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        height = 0;
        return false;
    }

    height = this->m_heights[y * E_WIDTH + x];
    return true;
}



// public
float Heightmap::get_vegetation_mask(Vector3f worldPos) {
    int32_t x;
    int32_t y;
    this->world_to_vertex(worldPos - Vector3f(.5f, 0.f, .5f), x, y);

    // USE A DIFFERENT MASK OF ONLY ALPHA-TEX FLOATS
    return this->m_paintMask[y* IZoneManager::ZONE_SIZE + x].a;
}


// public
void Heightmap::world_to_vertex(Vector3f worldPos, int32_t& x, int32_t &y) {
    Vector3f vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    x = floor(vector.x + 0.5f) + (IZoneManager::ZONE_SIZE / 2);
    y = floor(vector.z + 0.5f) + (IZoneManager::ZONE_SIZE / 2);
}

// private
void Heightmap::world_to_heightmap(Vector3f worldPos, float& x, float &y) {
    Vector3f vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    x = vector.x / IZoneManager::ZONE_SIZE + 0.5f;
    y = vector.z / IZoneManager::ZONE_SIZE + 0.5f;
}

// public
float Heightmap::get_height(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        return 0;
    }
    return this->m_heights[y * E_WIDTH + x];
}

// public
float Heightmap::get_base_height(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        return 0;
    }
    return this->m_base->m_baseHeights[y * E_WIDTH + x];
}

// public
//Vector3f Heightmap::GetWorldPosition() {
//    return IZoneManager::ZoneToWorldPos(this->m_zone) + 
//        Vector3f(IZoneManager::ZONE_SIZE / 2.f, 0.f, IZoneManager::ZONE_SIZE / 2.f);
//}

#endif