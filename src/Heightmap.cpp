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
//    Regenerate();
//}

// private
// This is essentially a one-off function once constructed
// so its use is kinda redundant continually in Generate()
//void Heightmap::Initialize() {
Heightmap::Heightmap(ZoneID zoneID, std::unique_ptr<BaseHeightmap> base)
    : m_zone(zoneID), m_base(std::move(base)) {
    Regenerate();
}

/*
void Heightmap::CancelQueuedRegeneration() {
    if (IsRegenerateQueued()) {
        m_queuedRegenerateTask->Cancel();
        m_queuedRegenerateTask = nullptr;
    }
}*/

/*
// public
void Heightmap::QueueRegenerate() {
    CancelQueuedRegeneration();

    m_queuedRegenerateTask = &Valhalla()->RunTaskLater([this](Task&) {
        Regenerate();
    }, 100ms);
}

// public
bool Heightmap::IsRegenerateQueued() {
    return m_queuedRegenerateTask;
}*/

// public
// Used to be the Regenerate() method
void Heightmap::Regenerate() {
    //CancelQueuedRegeneration();

    //Generate();
    //UpdateCornerDepths();

    m_cornerBiomes = m_base->m_cornerBiomes;
    this->m_heights = m_base->m_baseHeights;

    this->m_paintMask.resize(m_base->m_vegMask.size());
    for (int i=0; i < m_base->m_vegMask.size(); i++)
        this->m_paintMask[i].a = m_base->m_vegMask[i];

    m_oceanDepth[0] = std::max(0.f, IZoneManager::WATER_LEVEL - GetHeight(0, IZoneManager::ZONE_SIZE));
    m_oceanDepth[1] = std::max(0.f, IZoneManager::WATER_LEVEL - GetHeight(IZoneManager::ZONE_SIZE, IZoneManager::ZONE_SIZE));
    m_oceanDepth[2] = std::max(0.f, IZoneManager::WATER_LEVEL - GetHeight(IZoneManager::ZONE_SIZE, 0));
    m_oceanDepth[3] = std::max(0.f, IZoneManager::WATER_LEVEL - GetHeight(0, 0));
}

/*
// private
void Heightmap::UpdateCornerDepths() {
    m_oceanDepth[0] = GetHeight(0, IZoneManager::ZONE_SIZE);
    m_oceanDepth[1] = GetHeight(IZoneManager::ZONE_SIZE, IZoneManager::ZONE_SIZE);
    m_oceanDepth[2] = GetHeight(IZoneManager::ZONE_SIZE, 0);
    m_oceanDepth[3] = GetHeight(0, 0);

    m_oceanDepth[0] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[0]);
    m_oceanDepth[1] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[1]);
    m_oceanDepth[2] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[2]);
    m_oceanDepth[3] = std::max(0.f, IZoneManager::WATER_LEVEL - m_oceanDepth[3]);
}*/

// public
std::array<float, 4>& Heightmap::GetOceanDepth() {
    return this->m_oceanDepth;
}



// public
float Heightmap::GetOceanDepth(Vector3f worldPos) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);

    float t = (float)num / IZoneManager::ZONE_SIZE;
    float t2 = (float)num2 / IZoneManager::ZONE_SIZE;
    float a = VUtils::Mathf::Lerp(this->m_oceanDepth[3], this->m_oceanDepth[2], t);
    float b = VUtils::Mathf::Lerp(this->m_oceanDepth[0], this->m_oceanDepth[1], t);
    return VUtils::Mathf::Lerp(a, b, t2);
}


/*
// private
void Heightmap::Generate() {
    //if (this->m_buildData == nullptr) {
    //    this->m_buildData = HeightmapBuilder::RequestTerrainBlocking(m_zone);
    //    m_cornerBiomes = m_buildData->m_cornerBiomes;
    //}

    this->m_heights = this->m_base->m_baseHeights;
    this->m_paintMask = this->m_base->m_baseMask;

    //this->ApplyModifiers();
}*/

// public
std::vector<Biome> Heightmap::GetBiomes() {
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
bool Heightmap::HaveBiome(Biome biome) {
    return (std::to_underlying(m_cornerBiomes[0]) & std::to_underlying(biome))
        || (std::to_underlying(m_cornerBiomes[1]) & std::to_underlying(biome))
        || (std::to_underlying(m_cornerBiomes[2]) & std::to_underlying(biome))
        || (std::to_underlying(m_cornerBiomes[3]) & std::to_underlying(biome));
}

// private
float Heightmap::Distance(float x, float y, float rx, float ry) {
    float num = x - rx;
    float num2 = y - ry;

    // (sqrt(2) - sqrt(x ^ 2 + y ^ 2)) ^ 3
    // https://www.math3d.org/sL5gEdMjk

    float num4 = std::sqrtf(2) - VUtils::Math::Magnitude(num, num2);
    return num4 * num4 * num4;
}

// public
Biome Heightmap::GetBiome(Vector3f point) {
    //if all biomes are the same, return same/any
    if (this->m_cornerBiomes[0] == this->m_cornerBiomes[1] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[2] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[3]) {
        return this->m_cornerBiomes[0];
    }

    float x = point.x;
    float z = point.z;
    this->WorldToNormalizedHM(point, x, z);

    // Bitshift biome weight table
    // TODO re-normalize biome shifts to use all perfectly allocated ones (since n7 is skipped in enum)
    std::array<float, 10> tempBiomeWeights{};
    assert(m_cornerBiomes[0] != Biome::None
        && m_cornerBiomes[1] != Biome::None
        && m_cornerBiomes[2] != Biome::None
        && m_cornerBiomes[3] != Biome::None && "Got Biome::None biome");

    // subtract 1 from index because Biome::None is not counted
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[0])] += this->Distance(x, z, 0, 0);
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[1])] += this->Distance(x, z, 1, 0);
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[2])] += this->Distance(x, z, 0, 1);
    tempBiomeWeights[VUtils::GetShift(m_cornerBiomes[3])] += this->Distance(x, z, 1, 1);

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
BiomeArea Heightmap::GetBiomeArea() {
    if (this->IsBiomeEdge()) {
        return BiomeArea::Edge;
    }
    return BiomeArea::Median;
}

// public
bool Heightmap::IsBiomeEdge() {
    return this->m_cornerBiomes[0] != this->m_cornerBiomes[1] 
        || this->m_cornerBiomes[0] != this->m_cornerBiomes[2] 
        || this->m_cornerBiomes[0] != this->m_cornerBiomes[3];
}

// private
void Heightmap::ApplyModifiers() {
    assert(false);

    /*
    std::vector<TerrainModifier> allInstances = TerrainModifier::GetAllInstances();

    //auto heightsCopy(m_heights);
    //auto leveledHeights(m_heights);

    //static std::unique_ptr<Heights> leveledHeights;
    //leveledHeights.reset();

    static std::unique_ptr<Heights> leveledHeights;

    //float[] array = nullptr;
    //float[] array2 = nullptr;

    for (auto&& terrainModifier : allInstances) {
        if (terrainModifier.enabled && this->TerrainVSModifier(terrainModifier)) {
            if (terrainModifier.m_playerModifiction && !leveledHeights) {
                leveledHeights = std::make_unique<Heights>(m_heights);
            }
            //this->ApplyModifier(terrainModifier, array, array2);
            this->ApplyModifier(terrainModifier, leveledHeights.get());
        }
    }

    TerrainComp terrainComp = TerrainComp.FindTerrainCompiler(base.transform.position);
    if (terrainComp) {
        if (!leveledHeights) {
            leveledHeights = std::make_unique<Heights>(m_heights);
            //array = this->m_heights.ToArray();
            //array2 = this->m_heights.ToArray();
        }
        terrainComp.ApplyToHeightmap(this->m_paintMask, this->m_heights, array, array2, this);
    }
    this->m_paintMask.Apply();*/
}

// private
void Heightmap::ApplyModifier(TerrainModifier modifier, BaseHeightmap::Heights_t* levelOnly) {
    assert(false);

    //if (modifier.m_level) {
    //    this->LevelTerrain(modifier.transform.position + Vector3f::UP * modifier.m_levelOffset, modifier.m_levelRadius, modifier.m_square, levelOnly, modifier.m_playerModifiction);
    //}
    //if (modifier.m_smooth) {
    //    this->SmoothTerrain2(modifier.transform.position + Vector3f::UP * modifier.m_levelOffset, modifier.m_smoothRadius, levelOnly, modifier.m_smoothPower, modifier.m_playerModifiction);
    //}
    //if (modifier.m_paintCleared) {
    //    this->PaintCleared(modifier.transform.position, modifier.m_paintRadius, modifier.m_paintType, modifier.m_paintHeightCheck, false);
    //}
}

// public
//bool Heightmap::CheckTerrainModIsContained(TerrainModifier modifier) {
//    throw std::runtime_error("not implemented");
//
//    //Vector3f position = modifier.transform.position;
//    //float num = modifier.GetRadius() + 0.1f;
//    //Vector3f position2 = base.transform.position;
//    //float num2 = (float)WIDTH * 0.5f;
//    //return position.x + num <= position2.x + num2 
//    //    && position.x - num >= position2.x - num2 
//    //    && position.z + num <= position2.z + num2 
//    //    && position.z - num >= position2.z - num2;
//}

// public
bool Heightmap::TerrainVSModifier(TerrainModifier modifier) {
    throw std::runtime_error("not implemented");

    //Vector3f position = modifier.transform.position;
    //float num = modifier.GetRadius() + 4;
    //Vector3f position2 = base.transform.position;
    //float num2 = (float)WIDTH * 0.5f;
    //return position.x + num >= position2.x - num2 && position.x - num <= position2.x + num2 && position.z + num >= position2.z - num2 && position.z - num <= position2.z + num2;
}

// private
Vector3f Heightmap::CalcVertex(int32_t x, int32_t y) {
    Vector3f a = Vector3f((float)IZoneManager::ZONE_SIZE * -0.5f, 0.f, (float)IZoneManager::ZONE_SIZE * -0.5f);

    // Poll heightmap height at x,z
    float y2 = this->m_heights[y * E_WIDTH + x];
    return a + Vector3f((float)x, y2, (float)y);
}

// private
void Heightmap::RebuildCollisionMesh() {
    assert(false);

    /*
    if (this->m_collisionMesh == nullptr) {
        this->m_collisionMesh = new Mesh();
    }

    int32_t num = WIDTH + 1;
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();

    std::vector<Vector3f> m_tempVertises;

    for (int32_t i = 0; i < num; i++) {
        for (int32_t j = 0; j < num; j++) {
            Vector3f vector = this->CalcVertex(j, i);
            m_tempVertises.push_back(vector);

            num3 = std::min(vector.y, num3);
            num2 = std::max(vector.y, num2);
        }
    }

    this->m_collisionMesh.SetVertices(m_tempVertises);
    uint32_t num4 = (num - 1) * (num - 1) * 6;
    if (this->m_collisionMesh.GetIndexCount(0) != num4) {

        std::vector<int32_t> m_tempIndices;

        for (int32_t k = 0; k < num - 1; k++) {
            for (int32_t l = 0; l < num - 1; l++) {
                int32_t item = k * num + l;
                int32_t item2 = k * num + l + 1;
                int32_t item3 = (k + 1) * num + l + 1;
                int32_t item4 = (k + 1) * num + l;
                m_tempIndices.push_back(item);
                m_tempIndices.push_back(item4);
                m_tempIndices.push_back(item2);
                m_tempIndices.push_back(item2);
                m_tempIndices.push_back(item4);
                m_tempIndices.push_back(item3);
            }
        }
        this->m_collisionMesh.SetIndices(m_tempIndices.ToArray(), MeshTopology.Triangles, 0);
    }
    if (this->m_collider) {
        this->m_collider.sharedMesh = this->m_collisionMesh;
    }*/
}

// private
void Heightmap::SmoothTerrain2(Vector3f worldPos, float radius, 
    BaseHeightmap::Heights_t* levelOnlyHeights, float power) {
    
    assert(false);

    /*
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);

    float b = worldPos.y - base.transform.position.y;
    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2f a = Vector2f(num, num2);
    int32_t num5 = WIDTH + 1;

    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            float num6 = a.Distance(Vector2f(j, i));
            if (num6 <= num3) {
                float num7 = num6 / num3;
                if (j >= 0 && i >= 0 && j < num5 && i < num5) {
                    if (power == 3) {
                        num7 = num7 * num7 * num7;
                    }
                    else {
                        num7 = pow(num7, power);
                    }
                    float height = this->GetHeight(j, i);
                    float t = 1 - num7;
                    float num8 = VUtils::Math::Lerp(height, b, t);
                    if (levelOnlyHeights) {
                        float num9 = (*levelOnlyHeights)[i * num5 + j];
                        num8 = VUtils::Math::Clamp(num8, num9 - 1, num9 + 1);
                    }
                    this->SetHeight(j, i, num8);
                }
            }
        }
    }*/

}

// private
bool Heightmap::AtMaxWorldLevelDepth(Vector3f worldPos) {
    float num;
    this->GetWorldHeight(worldPos, num);
    float num2;
    this->GetWorldBaseHeight(worldPos, num2);
    return std::max(-(num - num2), 0.f) >= 7.95f;
}

// private
bool Heightmap::GetWorldBaseHeight(Vector3f worldPos, float& height) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);

    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        height = 0;
        return false;
    }

    height = this->m_base->m_baseHeights[y * E_WIDTH + x];
    return true;
}


bool Heightmap::GetWorldNormal(Vector3f worldPos, Vector3f& normal) {
    //float y;
    Vector3f a = worldPos;
    if (!GetWorldHeight(worldPos, a.y))
        return false;

    //float y1;
    Vector3f b = worldPos + Vector3f(1, 0, 0);
    if (!GetWorldHeight(b, b.y)) {
        b = worldPos + Vector3f(-1, 0, 0);
        GetWorldHeight(b, b.y);
    }

    Vector3f c = worldPos + Vector3f(0, 0, 1);
    if (!GetWorldHeight(c, c.y)) {
        c = worldPos + Vector3f(0, 0, -1);
        GetWorldHeight(c, c.y);
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
bool Heightmap::GetWorldHeight(const Vector3f& worldPos, float& height) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);

    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        height = 0;
        return false;
    }

    height = this->m_heights[y * E_WIDTH + x];
    return true;
}

// private
bool Heightmap::GetAverageWorldHeight(Vector3f worldPos, float radius, float &height) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);

    float sumHeight = 0;
    int32_t sumArea = 0;
    for (int32_t i = y - radius; i <= y + radius; i++) {
        for (int32_t j = x - radius; j <= x + radius; j++) {
            if (VUtils::Math::SqDistance(x, y, j, i) <= radius * radius) {
                if (!(j >= 0 && i >= 0 && j < E_WIDTH && i < E_WIDTH))
                    continue;

                sumHeight += this->GetHeight(j, i);
                sumArea++;
            }
        }
    }

    if (sumArea == 0) {
        height = 0;
        return false;
    }

    height = sumHeight / (float)sumArea;
    return true;
}

// private
bool Heightmap::GetMinWorldHeight(Vector3f worldPos, float radius, float &height) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);

    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2f a = Vector2f(x, y);
    int32_t num5 = IZoneManager::ZONE_SIZE + 1;
    height = 99999;
    for (int32_t i = y - num4; i <= y + num4; i++) {
        for (int32_t j = x - num4; j <= x + num4; j++) {
            if (a.Distance(Vector2f(j, i)) <= num3 
                && j >= 0 && i >= 0 && j < num5&& i < num5) {
                float height2 = this->GetHeight(j, i);
                if (height2 < height) {
                    height = height2;
                }
            }
        }
    }

    return height != 99999;
}

// private
bool Heightmap::GetMaxWorldHeight(Vector3f worldPos, float radius, float &height) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);

    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2f a = Vector2f(x, y);

    height = -99999;
    for (int32_t i = y - num4; i <= y + num4; i++) {
        for (int32_t j = x - num4; j <= x + num4; j++) {
            if (a.Distance(Vector2f(j, i)) <= num3 
                && j >= 0 && i >= 0 && j < E_WIDTH && i < E_WIDTH) {
                float height2 = this->GetHeight(j, i);
                if (height2 > height) {
                    height = height2;
                }
            }
        }
    }

    return height != -99999;
}


// private
void Heightmap::SmoothTerrain(Vector3f worldPos, float radius, bool square, float intensity) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);

    std::vector<std::pair<Vector2i, float>> list;

    for (int32_t i = y - radius; i <= y + radius; i++) {
        for (int32_t j = x - radius; j <= x + radius; j++) {
            if ((square || VUtils::Math::SqDistance(x, y, j, i) <= radius * radius)
                && (j != 0 && i != 0 && j != IZoneManager::ZONE_SIZE && i != IZoneManager::ZONE_SIZE)) {
                list.push_back(std::make_pair(Vector2i(j, i), this->GetAvgHeight(j, i, 1)));
            }
        }
    }

    for (auto&& pair : list) {
        float h = VUtils::Mathf::Lerp(
            this->GetHeight(pair.first.x, pair.first.y), pair.second, intensity);
        this->SetHeight(pair.first.x, pair.first.y, h);
    }
}

// private
float Heightmap::GetAvgHeight(int32_t cx, int32_t cy, int32_t w) {
    float sumHeight = 0;
    int32_t sumArea = 0;

    for (int32_t i = cy - w; i <= cy + w; i++) {
        for (int32_t j = cx - w; j <= cx + w; j++) {
            if (j >= 0 && i >= 0 && j < E_WIDTH && i < E_WIDTH) {
                sumHeight += this->GetHeight(j, i);
                sumArea++;
            }
        }
    }

    if (sumArea == 0) {
        return 0;
    }

    return sumHeight / (float)sumArea;
}

// private
float Heightmap::GroundHeight(Vector3f point) {
    assert(false);

    //Ray ray = new Ray(point + Vector3f::UP * 100.f, Vector3f::DOWN);
    //RaycastHit raycastHit;
    //if (this->m_collider.Raycast(ray, raycastHit, 300)) {
    //    return raycastHit.point.y;
    //}
    return -10000;
}

// private
void Heightmap::FindObjectsToMove(Vector3f worldPos, float area, std::vector<Rigidbody> &objects) {
    assert(false);

    //if (this->m_collider == nullptr) {
    //    return;
    //}
    //for (auto&& collider : Physics.OverlapBox(worldPos, Vector3f(area / 2.f, 500, area / 2.f))) {
    //    if (!(collider == this->m_collider) && collider.attachedRigidbody) {
    //        Rigidbody attachedRigidbody = collider.attachedRigidbody;
    //        ZNetView component = attachedRigidbody.GetComponent<ZNetView>();
    //        if (!component || component.IsOwner()) {
    //            objects.push_back(attachedRigidbody);
    //        }
    //    }
    //}
}

// private
void Heightmap::PaintCleared(Vector3f worldPos, float radius, 
    TerrainModifier::PaintType paintType, bool heightCheck) {

    assert(false);

    /*
    worldPos.x -= 0.5f;
    worldPos.z -= 0.5f;
    float num = worldPos.y - base.transform.position.y;
    int32_t num2;
    int32_t num3;
    this->WorldToVertex(worldPos, num2, num3);
    float num4 = radius;
    int32_t num5 = ceil(num4);
    Vector2f a = Vector2f(num2, num3);

    for (int32_t i = num3 - num5; i <= num3 + num5; i++) {
        for (int32_t j = num2 - num5; j <= num2 + num5; j++) {
            float num6 = a.Distance(Vector2f(j, i));
            if (j >= 0 && i >= 0 && j < this->m_paintMask.width && i < this->m_paintMask.height 
                && (!heightCheck || this->GetHeight(j, i) <= num)) {

                float num7 = 1 - VUtils::Math::Clamp01(num6 / num4);
                num7 = pow(num7, 0.1f);
                Color color = this->m_paintMask.GetPixel(j, i);
                float a2 = color.a;
                switch (paintType) {
                case TerrainModifier::PaintType::Dirt:
                    color = color.Lerp(m_paintMaskDirt, num7);
                    break;
                case TerrainModifier::PaintType::Cultivate:
                    color = color.Lerp(m_paintMaskCultivated, num7);
                    break;
                case TerrainModifier::PaintType::Paved:
                    color = color.Lerp(m_paintMaskPaved, num7);
                    break;
                case TerrainModifier::PaintType::Reset:
                    color = color.Lerp(m_paintMaskNothing, num7);
                    break;
                }
                color.a = a2;
                this->m_paintMask.SetPixel(j, i, color);
            }
        }
    }*/

}

// public
float Heightmap::GetVegetationMask(Vector3f worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos - Vector3f(.5f, 0.f, .5f), x, y);

    // USE A DIFFERENT MASK OF ONLY ALPHA-TEX FLOATS
    return this->m_paintMask[y* IZoneManager::ZONE_SIZE + x].a;
}

// public
bool Heightmap::IsCleared(Vector3f worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos - Vector3f(.5f, 0.f, .5f), x, y);
    
    // mode is clamp
    x = std::clamp(x, 0, IZoneManager::ZONE_SIZE - 1);
    y = std::clamp(y, 0, IZoneManager::ZONE_SIZE - 1);

    auto&& pixel = this->m_paintMask[y * IZoneManager::ZONE_SIZE + x];
    return pixel.r > 0.5f || pixel.g > 0.5f || pixel.b > 0.5f;
}

// public
bool Heightmap::IsCultivated(Vector3f worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);

    return this->m_paintMask[y * IZoneManager::ZONE_SIZE + x].g > 0.5f;
}

// public
void Heightmap::WorldToVertex(Vector3f worldPos, int32_t& x, int32_t &y) {
    Vector3f vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    x = floor(vector.x + 0.5f) + (IZoneManager::ZONE_SIZE / 2);
    y = floor(vector.z + 0.5f) + (IZoneManager::ZONE_SIZE / 2);
}

// private
void Heightmap::WorldToNormalizedHM(Vector3f worldPos, float& x, float &y) {
    Vector3f vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    x = vector.x / IZoneManager::ZONE_SIZE + 0.5f;
    y = vector.z / IZoneManager::ZONE_SIZE + 0.5f;
}

// private
void Heightmap::LevelTerrain(Vector3f worldPos, float radius, bool square, 
    BaseHeightmap::Heights_t* levelOnly) {

    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    Vector3f vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    float num3 = radius;
    int32_t num4 = ceil(num3);
    int32_t num5 = E_WIDTH;
    Vector2f a = Vector2f(num, num2);
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if ((square || a.Distance(Vector2f(j, i)) <= num3) && j >= 0 && i >= 0 && j < num5&& i < num5) {
                float num6 = vector.y;
                if (levelOnly) {
                    float num7 = m_heights[i * num5 + j];
                    num6 = VUtils::Math::Clamp(num6, num7 - 8, num7 + 8);
                    (*levelOnly)[i * num5 + j] = num6;
                }
                this->SetHeight(j, i, num6);
            }
        }
    }
}

// public
Color Heightmap::GetPaintMask(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || x >= IZoneManager::ZONE_SIZE || y >= IZoneManager::ZONE_SIZE) {
        return Colors::BLACK;
    }
    return this->m_paintMask[y * IZoneManager::ZONE_SIZE + x];
}

// public
float Heightmap::GetHeight(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        return 0;
    }
    return this->m_heights[y * E_WIDTH + x];
}

// public
float Heightmap::GetBaseHeight(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        return 0;
    }
    return this->m_base->m_baseHeights[y * E_WIDTH + x];
}

// public
void Heightmap::SetHeight(int32_t x, int32_t y, float h) {
    if (x < 0 || y < 0 || x >= E_WIDTH || y >= E_WIDTH) {
        return;
    }
    this->m_heights[y * E_WIDTH + x] = h;
}

// public
bool Heightmap::IsPointInside(Vector3f point, float radius) {
    //throw std::runtime_error("not implemented");

    float num = (float)IZoneManager::ZONE_SIZE * 0.5f;
    Vector3f position = IZoneManager::ZoneToWorldPos(this->m_zone);
    return point.x + radius >= position.x - num 
        && point.x - radius <= position.x + num 
        && point.z + radius >= position.z - num 
        && point.z - radius <= position.z + num;
}

// public
TerrainComp Heightmap::GetAndCreateTerrainCompiler() {
    throw std::runtime_error("not implemented");
    //TerrainComp terrainComp = TerrainComp.FindTerrainCompiler(base.transform.position);
    //if (terrainComp) {
    //    return terrainComp;
    //}
    //return UnityEngine.Object.Instantiate<GameObject>(this->m_terrainCompilerPrefab, base.transform.position, Quaternion::IDENTITY).GetComponent<TerrainComp>();
}

// public
//Vector3f Heightmap::GetWorldPosition() {
//    return IZoneManager::ZoneToWorldPos(this->m_zone) + 
//        Vector3f(IZoneManager::ZONE_SIZE / 2.f, 0.f, IZoneManager::ZONE_SIZE / 2.f);
//}

#endif