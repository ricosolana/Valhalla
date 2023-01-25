#include "HeightMap.h"
#include "HeightmapBuilder.h"
#include "ZoneManager.h"
#include "HeightmapManager.h"

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
Heightmap::Heightmap(const Vector2i& zoneID) : m_zone(zoneID) {
    int32_t num = WIDTH + 1;
    int32_t num2 = num * num;
    //if (this->m_heights.size() != num2) {
        //this->m_heights.clear();
        //for (int32_t i = 0; i < num2; i++) {
            //this->m_heights.push_back(0);
        //}

    // Zero value-initialize
    // https://stackoverflow.com/a/18295840
    //std::fill(m_heights.begin(), m_heights.end(), 0);
    m_heights.resize((Heightmap::WIDTH + 1) * (Heightmap::WIDTH + 1));

    m_paintMask.resize(WIDTH * WIDTH);

    //this->m_paintMask = new Texture2D(WIDTH, WIDTH);
    //this->m_paintMask.wrapMode = TextureWrapMode.Clamp;
    //this->m_materialInstance = new Material(this->m_material);
    //this->m_materialInstance.SetTexture("_ClearedMaskTex", this->m_paintMask);
}

void Heightmap::CancelQueuedRegeneration() {
    if (IsRegenerateQueued()) {
        m_queuedRegenerateTask->Cancel();
        m_queuedRegenerateTask = nullptr;
    }
}

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
    //return base.IsInvoking("Regenerate");
}

// public
void Heightmap::Regenerate() {
    CancelQueuedRegeneration();

    Generate();
    //RebuildCollisionMesh();
    UpdateCornerDepths();
}

// private
void Heightmap::UpdateCornerDepths() {
    m_oceanDepth[0] = GetHeight(0, WIDTH);
    m_oceanDepth[1] = GetHeight(WIDTH, WIDTH);
    m_oceanDepth[2] = GetHeight(WIDTH, 0);
    m_oceanDepth[3] = GetHeight(0, 0);

    float num = IZoneManager::WATER_LEVEL;

    m_oceanDepth[0] = std::max(0.f, num - m_oceanDepth[0]);
    m_oceanDepth[1] = std::max(0.f, num - m_oceanDepth[1]);
    m_oceanDepth[2] = std::max(0.f, num - m_oceanDepth[2]);
    m_oceanDepth[3] = std::max(0.f, num - m_oceanDepth[3]);
    //m_materialInstance.SetFloatArray("_depth", m_oceanDepth); // for shader only, server wont use
}

// public
std::array<float, 4>& Heightmap::GetOceanDepth() {
    return this->m_oceanDepth;
}



// public
float Heightmap::GetOceanDepth(const Vector3& worldPos) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);

    float t = (float)num / (float)WIDTH;
    float t2 = (float)num2 / (float)WIDTH;
    float a = VUtils::Math::Lerp(this->m_oceanDepth[3], this->m_oceanDepth[2], t);
    float b = VUtils::Math::Lerp(this->m_oceanDepth[0], this->m_oceanDepth[1], t);
    return VUtils::Math::Lerp(a, b, t2);
}



// private
void Heightmap::Generate() {
    //this->Initialize();
    int32_t num = WIDTH + 1;
    int32_t num2 = num * num;
    //Vector3 position; // = m_zone;

    if (this->m_buildData == nullptr) {
        this->m_buildData = HeightmapBuilder::RequestTerrainBlocking(m_zone);
        std::copy(m_buildData->m_cornerBiomes.begin(), 
            m_buildData->m_cornerBiomes.end() + m_buildData->m_cornerBiomes.size(), 
            m_cornerBiomes.begin());
        //this->m_cornerBiomes = this->m_buildData->m_cornerBiomes;
    }

    this->m_heights = this->m_buildData->m_baseHeights;

    //for (int32_t i = 0; i < num2; i++) {
    //
    //    this->m_heights[i] = this->m_buildData->m_baseHeights[i];
    //}

    this->m_paintMask = this->m_buildData->m_baseMask;

    //this->m_paintMask.SetPixels(this->m_buildData->m_baseMask);
    this->ApplyModifiers();
}

// private
float Heightmap::Distance(float x, float y, float rx, float ry) {
    float num = x - rx;
    float num2 = y - ry;
    float num3 = sqrt(num * num + num2 * num2);
    float num4 = 1.4142135f - num3;
    return num4 * num4 * num4;
}

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

// public
Biome Heightmap::GetBiome(const Vector3& point) {
    //if all biomes are the same, return same/any
    if (this->m_cornerBiomes[0] == this->m_cornerBiomes[1] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[2] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[3]) {
        return this->m_cornerBiomes[0];
    }

    float x = point.x;
    float z = point.z;
    this->WorldToNormalizedHM(point, x, z);

    // Basically what this does is create a weight table based on



    // basically get the bitmask shift of the cornerBiome value
    std::array<float, 9> tempBiomeWeights{};

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

    //return (Biome) (0b1 << result);




    //std::vector<float> tempBiomeWeights;
    //tempBiomeWeights.resize(513);
    //
    //tempBiomeWeights[this->m_cornerBiomes[0]] += this->Distance(x, z, 0, 0);
    //tempBiomeWeights[this->m_cornerBiomes[1]] += this->Distance(x, z, 1, 0);
    //tempBiomeWeights[this->m_cornerBiomes[2]] += this->Distance(x, z, 0, 1);
    //tempBiomeWeights[this->m_cornerBiomes[3]] += this->Distance(x, z, 1, 1);
    //
    //int32_t result = 0;
    //float num = -99999;
    //for (int32_t j = 1; j < tempBiomeWeights.size(); j++) {
    //    if (tempBiomeWeights[j] > num) {
    //        result = j;
    //        num = tempBiomeWeights[j];
    //    }
    //}
    //return (Biome)result;
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
    return this->m_cornerBiomes[0] != this->m_cornerBiomes[1] || this->m_cornerBiomes[0] != this->m_cornerBiomes[2] || this->m_cornerBiomes[0] != this->m_cornerBiomes[3];
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
void Heightmap::ApplyModifier(TerrainModifier modifier, HMBuildData::Heights_t* levelOnly) {
    assert(false);

    //if (modifier.m_level) {
    //    this->LevelTerrain(modifier.transform.position + Vector3::UP * modifier.m_levelOffset, modifier.m_levelRadius, modifier.m_square, levelOnly, modifier.m_playerModifiction);
    //}
    //if (modifier.m_smooth) {
    //    this->SmoothTerrain2(modifier.transform.position + Vector3::UP * modifier.m_levelOffset, modifier.m_smoothRadius, levelOnly, modifier.m_smoothPower, modifier.m_playerModifiction);
    //}
    //if (modifier.m_paintCleared) {
    //    this->PaintCleared(modifier.transform.position, modifier.m_paintRadius, modifier.m_paintType, modifier.m_paintHeightCheck, false);
    //}
}

// public
//bool Heightmap::CheckTerrainModIsContained(TerrainModifier modifier) {
//    throw std::runtime_error("not implemented");
//
//    //Vector3 position = modifier.transform.position;
//    //float num = modifier.GetRadius() + 0.1f;
//    //Vector3 position2 = base.transform.position;
//    //float num2 = (float)WIDTH * 0.5f;
//    //return position.x + num <= position2.x + num2 
//    //    && position.x - num >= position2.x - num2 
//    //    && position.z + num <= position2.z + num2 
//    //    && position.z - num >= position2.z - num2;
//}

// public
bool Heightmap::TerrainVSModifier(TerrainModifier modifier) {
    throw std::runtime_error("not implemented");

    //Vector3 position = modifier.transform.position;
    //float num = modifier.GetRadius() + 4;
    //Vector3 position2 = base.transform.position;
    //float num2 = (float)WIDTH * 0.5f;
    //return position.x + num >= position2.x - num2 && position.x - num <= position2.x + num2 && position.z + num >= position2.z - num2 && position.z - num <= position2.z + num2;
}

// private
Vector3 Heightmap::CalcVertex(int32_t x, int32_t y) {
    int32_t num = WIDTH + 1;
    Vector3 a = Vector3((float)WIDTH * -0.5f, 0, (float)WIDTH * -0.5f);

    // Poll heightmap height at x,z
    float y2 = this->m_heights[y * num + x];
    return a + Vector3(x, y2, y);
}

// private
void Heightmap::RebuildCollisionMesh() {
    //assert(false);
    /*
    if (this->m_collisionMesh == nullptr) {
        this->m_collisionMesh = new Mesh();
    }

    int32_t num = WIDTH + 1;
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();

    std::vector<Vector3> m_tempVertises;

    for (int32_t i = 0; i < num; i++) {
        for (int32_t j = 0; j < num; j++) {
            Vector3 vector = this->CalcVertex(j, i);
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
void Heightmap::SmoothTerrain2(const Vector3& worldPos, float radius, 
    HMBuildData::Heights_t* levelOnlyHeights, float power) {
    
    assert(false);

    /*
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);

    float b = worldPos.y - base.transform.position.y;
    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2 a = Vector2(num, num2);
    int32_t num5 = WIDTH + 1;

    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            float num6 = a.Distance(Vector2(j, i));
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
bool Heightmap::AtMaxWorldLevelDepth(const Vector3& worldPos) {
    float num;
    this->GetWorldHeight(worldPos, num);
    float num2;
    this->GetWorldBaseHeight(worldPos, num2);
    return std::max(-(num - num2), 0.f) >= 7.95f;
}

// private
bool Heightmap::GetWorldBaseHeight(const Vector3& worldPos, float& height) {
    assert(false);

    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    int32_t num3 = WIDTH + 1;
    if (num < 0 || num2 < 0 || num >= num3 || num2 >= num3) {
        height = 0;
        return false;
    }
    //height = this->m_buildData->m_baseHeights[num2 * num3 + num] 
    //    + base.transform.position.y;
    return true;
}

// private
bool Heightmap::GetWorldHeight(const Vector3& worldPos, float& height) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    int32_t num3 = WIDTH + 1;
    if (num < 0 || num2 < 0 || num >= num3 || num2 >= num3) {
        height = 0;
        return false;
    }

    height = this->m_heights[num2 * num3 + num];

    return true;
}

// private
bool Heightmap::GetAverageWorldHeight(const Vector3& worldPos, float radius, float &height) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2 a = Vector2(num, num2);
    int32_t num5 = WIDTH + 1;
    float num6 = 0;
    int32_t num7 = 0;
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if (a.Distance(Vector2(j, i)) <= num3 
                && j >= 0 && i >= 0 && j < num5 && i < num5) {
                num6 += this->GetHeight(j, i);
                num7++;
            }
        }
    }
    if (num7 == 0) {
        height = 0;
        return false;
    }
    height = num6 / (float)num7;
    return true;
}

// private
bool Heightmap::GetMinWorldHeight(const Vector3& worldPos, float radius, float &height) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2 a = Vector2(num, num2);
    int32_t num5 = WIDTH + 1;
    height = 99999;
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if (a.Distance(Vector2(j, i)) <= num3 
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
bool Heightmap::GetMaxWorldHeight(const Vector3& worldPos, float radius, float &height) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2 a = Vector2(num, num2);
    int32_t num5 = WIDTH + 1;
    height = -99999;
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if (a.Distance(Vector2(j, i)) <= num3 
                && j >= 0 && i >= 0 && j < num5&& i < num5) {
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
void Heightmap::SmoothTerrain(const Vector3& worldPos, float radius, bool square, float intensity) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);

    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2 a = Vector2(num, num2);
    std::vector<std::pair<Vector2i, float>> list;

    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if ((square || a.Distance(Vector2(j, i)) <= num3) 
                && j != 0 && i != 0 && j != WIDTH && i != WIDTH) {
                list.push_back(std::make_pair(Vector2i(j, i), this->GetAvgHeight(j, i, 1)));
            }
        }
    }

    for (auto&& keyValuePair : list) {
        float h = VUtils::Math::Lerp(
            this->GetHeight(keyValuePair.first.x, keyValuePair.first.y), keyValuePair.second, intensity);
        this->SetHeight(keyValuePair.first.x, keyValuePair.first.y, h);
    }
}

// private
float Heightmap::GetAvgHeight(int32_t cx, int32_t cy, int32_t w) {
    int32_t num = WIDTH + 1;
    float num2 = 0;
    int32_t num3 = 0;

    for (int32_t i = cy - w; i <= cy + w; i++) {
        for (int32_t j = cx - w; j <= cx + w; j++) {
            if (j >= 0 && i >= 0 && j < num && i < num) {
                num2 += this->GetHeight(j, i);
                num3++;
            }
        }
    }
    if (num3 == 0) {
        return 0;
    }
    return num2 / (float)num3;
}

// private
float Heightmap::GroundHeight(const Vector3& point) {
    assert(false);

    //Ray ray = new Ray(point + Vector3::UP * 100.f, Vector3::DOWN);
    //RaycastHit raycastHit;
    //if (this->m_collider.Raycast(ray, raycastHit, 300)) {
    //    return raycastHit.point.y;
    //}
    return -10000;
}

// private
void Heightmap::FindObjectsToMove(Vector3 worldPos, float area, std::vector<Rigidbody> &objects) {
    assert(false);

    //if (this->m_collider == nullptr) {
    //    return;
    //}
    //for (auto&& collider : Physics.OverlapBox(worldPos, Vector3(area / 2.f, 500, area / 2.f))) {
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
void Heightmap::PaintCleared(Vector3 worldPos, float radius, 
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
    Vector2 a = Vector2(num2, num3);

    for (int32_t i = num3 - num5; i <= num3 + num5; i++) {
        for (int32_t j = num2 - num5; j <= num2 + num5; j++) {
            float num6 = a.Distance(Vector2(j, i));
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
float Heightmap::GetVegetationMask(const Vector3& worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos - Vector3(.5f, 0, .5f), x, y);


    // USE A DIFFERENT MASK OF ONLY ALPHA-TEX FLOATS
    return this->m_paintMask[y*WIDTH + x].a;
}

// public
bool Heightmap::IsCleared(const Vector3& worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos - Vector3(.5f, 0, .5f), x, y);
    auto&& pixel = this->m_paintMask[y * WIDTH + x];
    return pixel.r > 0.5f || pixel.g > 0.5f || pixel.b > 0.5f;
}

// public
bool Heightmap::IsCultivated(const Vector3& worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);
    return this->m_paintMask[y * WIDTH + x].g > 0.5f;
}

// public
void Heightmap::WorldToVertex(const Vector3& worldPos, int32_t& x, int32_t &y) {
    Vector3 vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    x = floor(vector.x + 0.5f) + WIDTH / 2;
    y = floor(vector.z + 0.5f) + WIDTH / 2;
}

// private
void Heightmap::WorldToNormalizedHM(const Vector3& worldPos, float& x, float &y) {
    float num = WIDTH;
    Vector3 vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    x = vector.x / num + 0.5f;
    y = vector.z / num + 0.5f;
}

// private
void Heightmap::LevelTerrain(const Vector3& worldPos, float radius, bool square, 
    HMBuildData::Heights_t* levelOnly) {

    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    Vector3 vector = worldPos - IZoneManager::ZoneToWorldPos(this->m_zone);
    float num3 = radius;
    int32_t num4 = ceil(num3);
    int32_t num5 = WIDTH + 1;
    Vector2 a = Vector2(num, num2);
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if ((square || a.Distance(Vector2(j, i)) <= num3) && j >= 0 && i >= 0 && j < num5&& i < num5) {
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
    if (x < 0 || y < 0 || x >= WIDTH || y >= WIDTH) {
        return Colors::BLACK;
    }
    return this->m_paintMask[y * WIDTH + x];
}

// public
float Heightmap::GetHeight(int32_t x, int32_t y) {
    int32_t num = WIDTH + 1;
    if (x < 0 || y < 0 || x >= num || y >= num) {
        return 0;
    }
    return this->m_heights[y * num + x];
}

// public
float Heightmap::GetBaseHeight(int32_t x, int32_t y) {
    int32_t num = WIDTH + 1;
    if (x < 0 || y < 0 || x >= num || y >= num) {
        return 0;
    }
    return this->m_buildData->m_baseHeights[y * num + x];
}

// public
void Heightmap::SetHeight(int32_t x, int32_t y, float h) {
    int32_t num = WIDTH + 1;
    if (x < 0 || y < 0 || x >= num || y >= num) {
        return;
    }
    this->m_heights[y * num + x] = h;
}

// public
bool Heightmap::IsPointInside(const Vector3& point, float radius) {
    //throw std::runtime_error("not implemented");

    float num = (float)Heightmap::WIDTH * 0.5f;
    Vector3 position = IZoneManager::ZoneToWorldPos(this->m_zone);
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
Vector3 Heightmap::GetWorldPosition() {
    return IZoneManager::ZoneToWorldPos(this->m_zone) + Vector3(WIDTH/2.f, 0, WIDTH/2.f);
}

