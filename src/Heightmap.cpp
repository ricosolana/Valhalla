#include "HeightMap.h"
#include "HeightmapBuilder.h"
#include "ZoneSystem.h"

// private
void Heightmap::Awake() {
    //if (!this->m_isDistantLod) {
    //m_heightmaps[this->m_zone] = this; // must externally manage heightmap; wont work safely
    //}
    m_collider = base.GetComponent<MeshCollider>();
}

// private
void Heightmap::OnDestroy() {
    m_heightmaps.erase(this);
    if (this->m_materialInstance) {
        UnityEngine.Object.DestroyImmediate(this->m_materialInstance);
    }
}

// private
void Heightmap::OnEnable() {
    Regenerate();
}

// public static
void Heightmap::ForceGenerateAll() {
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->HaveQueuedRebuild()) {
            LOG(INFO) << "Force generating hmap " << heightmap->m_zone.x << " " << heightmap->m_zone.y;
            //LOG(INFO) << "Force generating hmap " << heightmap.transform.position.ToString();
            heightmap->Regenerate();
        }
    }
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
    RebuildCollisionMesh();
    UpdateCornerDepths();
}

// private
void Heightmap::UpdateCornerDepths() {
    m_oceanDepth[0] = GetHeight(0, WIDTH);
    m_oceanDepth[1] = GetHeight(WIDTH, WIDTH);
    m_oceanDepth[2] = GetHeight(WIDTH, 0);
    m_oceanDepth[3] = GetHeight(0, 0);

    float num = ZoneSystem::WATER_LEVEL;

    m_oceanDepth[0] = std::max(0.f, num - m_oceanDepth[0]);
    m_oceanDepth[1] = std::max(0.f, num - m_oceanDepth[1]);
    m_oceanDepth[2] = std::max(0.f, num - m_oceanDepth[2]);
    m_oceanDepth[3] = std::max(0.f, num - m_oceanDepth[3]);
    //m_materialInstance.SetFloatArray("_depth", m_oceanDepth); // for shader only, server wont use
}

// public
float[] Heightmap::GetOceanDepth() {
    return this->m_oceanDepth;
}

// public static 
float Heightmap::GetOceanDepthAll(const Vector3& worldPos) {
    auto&& heightmap = FindHeightmap(worldPos);
    if (heightmap) {
        return heightmap->GetOceanDepth(worldPos);
    }
    return 0;
}

// public
float Heightmap::GetOceanDepth(const Vector3& worldPos) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);

    float t = (float)num / (float)WIDTH;
    float t2 = (float)num2 / (float)WIDTH;
    float a = Mathf.Lerp(this->m_oceanDepth[3], this->m_oceanDepth[2], t);
    float b = Mathf.Lerp(this->m_oceanDepth[0], this->m_oceanDepth[1], t);
    return Mathf.Lerp(a, b, t2);
}

// private
void Heightmap::Initialize() {
    int32_t num = WIDTH + 1;
    int32_t num2 = num * num;
    if (this->m_heights.Count != num2) {
        this->m_heights.clear();
        for (int32_t i = 0; i < num2; i++) {
            this->m_heights.Add(0f);
        }
        this->m_paintMask = new Texture2D(WIDTH, WIDTH);
        this->m_paintMask.wrapMode = TextureWrapMode.Clamp;
        this->m_materialInstance = new Material(this->m_material);
        this->m_materialInstance.SetTexture("_ClearedMaskTex", this->m_paintMask);
    }
}

// private
void Heightmap::Generate() {
    this->Initialize();
    int32_t num = WIDTH + 1;
    int32_t num2 = num * num;
    Vector3 position; // = m_zone;

    if (this->m_buildData == nullptr
        || this->m_buildData.m_baseHeights.Count != num2
        || this->m_buildData.m_center != position
        || this->m_buildData.m_scale != this->m_scale
        || this->m_buildData.m_worldGen != WorldGenerator.instance) {
        this->m_buildData = HeightmapBuilder::RequestTerrainSync(position, WIDTH, this->m_scale, this->m_isDistantLod, WorldGenerator.instance);
        this->m_cornerBiomes = this->m_buildData.m_cornerBiomes;
    }

    for (int32_t i = 0; i < num2; i++) {
        this->m_heights[i] = this->m_buildData.m_baseHeights[i];
    }

    this->m_paintMask.SetPixels(this->m_buildData.m_baseMask);
    this->ApplyModifiers();
}

// private
float Heightmap::Distance(float x, float y, float rx, float ry) {
    float num = x - rx;
    float num2 = y - ry;
    float num3 = Mathf.Sqrt(num * num + num2 * num2);
    float num4 = 1.414f - num3;
    return num4 * num4 * num4;
}

// public
std::vector<Heightmap::Biome> Heightmap::GetBiomes() {
    std::vector<Biome> list;
    Biome mask = None;
    for (auto&& item : this->m_cornerBiomes) {
        if ((mask & item) != item) {
            list.push_back(item);
            mask |= item;
        }
    }
    return list;
}

// public
bool Heightmap::HaveBiome(Heightmap::Biome biome) {
    return (this->m_cornerBiomes[0] & biome) != Biome::None 
        || (this->m_cornerBiomes[1] & biome) != Biome::None
        || (this->m_cornerBiomes[2] & biome) != Biome::None
        || (this->m_cornerBiomes[3] & biome) > Biome::None;
}

// public
Heightmap::Biome Heightmap::GetBiome(const Vector3& point) {
    if (this->m_cornerBiomes[0] == this->m_cornerBiomes[1] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[2] 
        && this->m_cornerBiomes[0] == this->m_cornerBiomes[3]) {
        return this->m_cornerBiomes[0];
    }

    float x = point.x;
    float z = point.z;
    this->WorldToNormalizedHM(point, x, z);

    // TODO please fix...
    // so crappy
    std::vector<float> tempBiomeWeights;
    tempBiomeWeights.resize(513);

    tempBiomeWeights[(int32_t)this->m_cornerBiomes[0]] += this->Distance(x, z, 0, 0);
    tempBiomeWeights[(int32_t)this->m_cornerBiomes[1]] += this->Distance(x, z, 1, 0);
    tempBiomeWeights[(int32_t)this->m_cornerBiomes[2]] += this->Distance(x, z, 0, 1);
    tempBiomeWeights[(int32_t)this->m_cornerBiomes[3]] += this->Distance(x, z, 1, 1);

    int32_t result = 0;
    float num = -99999;
    for (int32_t j = 1; j < tempBiomeWeights.size(); j++) {
        if (tempBiomeWeights[j] > num) {
            result = j;
            num = tempBiomeWeights[j];
        }
    }
    return (Biome)result;
}

// public
Heightmap::BiomeArea Heightmap::GetBiomeArea() {
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
    std::vector<TerrainModifier> allInstances = TerrainModifier.GetAllInstances();
    float[] array = nullptr;
    float[] array2 = nullptr;
    for (auto&& terrainModifier : allInstances) {
        if (terrainModifier.enabled && this->TerrainVSModifier(terrainModifier)) {
            if (terrainModifier.m_playerModifiction && array == nullptr) {
                array = this->m_heights.ToArray();
                array2 = this->m_heights.ToArray();
            }
            this->ApplyModifier(terrainModifier, array, array2);
        }
    }
    TerrainComp terrainComp = TerrainComp.FindTerrainCompiler(base.transform.position);
    if (terrainComp) {
        if (array == nullptr) {
            array = this->m_heights.ToArray();
            array2 = this->m_heights.ToArray();
        }
        terrainComp.ApplyToHeightmap(this->m_paintMask, this->m_heights, array, array2, this);
    }
    this->m_paintMask.Apply();
}

// private
void Heightmap::ApplyModifier(TerrainModifier modifier, float[] baseHeights, float[] levelOnly) {
    if (modifier.m_level) {
        this->LevelTerrain(modifier.transform.position + Vector3::UP * modifier.m_levelOffset, modifier.m_levelRadius, modifier.m_square, baseHeights, levelOnly, modifier.m_playerModifiction);
    }
    if (modifier.m_smooth) {
        this->SmoothTerrain2(modifier.transform.position + Vector3::UP * modifier.m_levelOffset, modifier.m_smoothRadius, levelOnly, modifier.m_smoothPower, modifier.m_playerModifiction);
    }
    if (modifier.m_paintCleared) {
        this->PaintCleared(modifier.transform.position, modifier.m_paintRadius, modifier.m_paintType, modifier.m_paintHeightCheck, false);
    }
}

// public
bool Heightmap::CheckTerrainModIsContained(TerrainModifier modifier) {
    Vector3 position = modifier.transform.position;
    float num = modifier.GetRadius() + 0.1f;
    Vector3 position2 = base.transform.position;
    float num2 = (float)WIDTH * 0.5f;
    return position.x + num <= position2.x + num2 && position.x - num >= position2.x - num2 && position.z + num <= position2.z + num2 && position.z - num >= position2.z - num2;
}

// public
bool Heightmap::TerrainVSModifier(TerrainModifier modifier) {
    Vector3 position = modifier.transform.position;
    float num = modifier.GetRadius() + 4;
    Vector3 position2 = base.transform.position;
    float num2 = (float)WIDTH * 0.5f;
    return position.x + num >= position2.x - num2 && position.x - num <= position2.x + num2 && position.z + num >= position2.z - num2 && position.z - num <= position2.z + num2;
}

// private
Vector3 Heightmap::CalcNormal2(std::vector<Vector3> vertises, int32_t x, int32_t y) {
    int32_t num = WIDTH + 1;
    Vector3 vector = vertises[y * num + x];
    Vector3 rhs;

    if (x == WIDTH) {
        Vector3 b = vertises[y * num + x - 1];
        rhs = vector - b;
    }
    else if (x == 0) {
        rhs = vertises[y * num + x + 1] - vector;
    }
    else {
        rhs = vertises[y * num + x + 1] - vertises[y * num + x - 1];
    }

    Vector3 lhs;
    if (y == WIDTH) {
        Vector3 b2 = this->CalcVertex(x, y - 1);
        lhs = vector - b2;
    }
    else if (y == 0) {
        lhs = this->CalcVertex(x, y + 1) - vector;
    }
    else {
        lhs = vertises[(y + 1) * num + x] - vertises[(y - 1) * num + x];
    }

    Vector3 result = lhs.Cross(rhs);
    result.Normalize();
    return result;
}

// private
Vector3 Heightmap::CalcNormal(int32_t x, int32_t y) {
    Vector3 vector = this->CalcVertex(x, y);
    Vector3 rhs;
    if (x == WIDTH) {
        Vector3 b = this->CalcVertex(x - 1, y);
        rhs = vector - b;
    }
    else {
        rhs = this->CalcVertex(x + 1, y) - vector;
    }

    Vector3 lhs;
    if (y == WIDTH) {
        Vector3 b2 = this->CalcVertex(x, y - 1);
        lhs = vector - b2;
    }
    else {
        lhs = this->CalcVertex(x, y + 1) - vector;
    }

    return lhs.Cross(rhs).Normalized();
}

// private
Vector3 Heightmap::CalcVertex(int32_t x, int32_t y) {
    int32_t num = WIDTH + 1;
    Vector3 a = Vector3((float)WIDTH * -0.5f, 0, (float)WIDTH * -0.5f);
    float y2 = this->m_heights[y * num + x];
    return a + Vector3((float)x, y2, (float)y);
}

// private
void Heightmap::RebuildCollisionMesh() {
    if (this->m_collisionMesh == nullptr) {
        this->m_collisionMesh = new Mesh();
    }

    int32_t num = WIDTH + 1;
    float num2 = -999999;
    float num3 = 999999;
    m_tempVertises.clear();
    for (int32_t i = 0; i < num; i++) {
        for (int32_t j = 0; j < num; j++) {
            Vector3 vector = this->CalcVertex(j, i);
            m_tempVertises.push_back(vector);
            if (vector.y > num2) {
                num2 = vector.y;
            }
            if (vector.y < num3) {
                num3 = vector.y;
            }
        }
    }

    this->m_collisionMesh.SetVertices(m_tempVertises);
    uint32_t num4 = (num - 1) * (num - 1) * 6;
    if (this->m_collisionMesh.GetIndexCount(0) != num4) {
        m_tempIndices.clear();
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
    }
    float num5 = (float)WIDTH * 0.5f;
    this->m_bounds.SetMinMax(base.transform.position + Vector3(-num5, num3, -num5), base.transform.position + Vector3(num5, num2, num5));
    this->m_boundingSphere.position = this->m_bounds.center;
    this->m_boundingSphere.radius = this->m_boundingSphere.position.Distance(this->m_bounds.max);
}

// private
void Heightmap::SmoothTerrain2(const Vector3& worldPos, float radius, 
    float[] levelOnlyHeights, float power, 
    bool playerModifiction) {

    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    float b = worldPos.y - base.transform.position.y;
    float num3 = radius;
    int32_t num4 = VUtils::Math::CeilToInt(num3);
    Vector2 a = Vector2((float)num, (float)num2);
    int32_t num5 = WIDTH + 1;
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            float num6 = a.Distance(Vector2((float)j, (float)i));
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
                    if (playerModifiction) {
                        float num9 = levelOnlyHeights[i * num5 + j];
                        num8 = Mathf.Clamp(num8, num9 - 1f, num9 + 1f);
                    }
                    this->SetHeight(j, i, num8);
                }
            }
        }
    }
}

// private
bool Heightmap::AtMaxWorldLevelDepth(const Vector3& worldPos) {
    float num;
    this->GetWorldHeight(worldPos, num);
    float num2;
    this->GetWorldBaseHeight(worldPos, num2);
    return Mathf.Max(-(num - num2), 0f) >= 7.95f;
}

// private
bool Heightmap::GetWorldBaseHeight(const Vector3& worldPos, float& height) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    int32_t num3 = WIDTH + 1;
    if (num < 0 || num2 < 0 || num >= num3 || num2 >= num3) {
        height = 0;
        return false;
    }
    height = this->m_buildData.m_baseHeights[num2 * num3 + num] + base.transform.position.y;
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
    height = this->m_heights[num2 * num3 + num] + base.transform.position.y;
    return true;
}

// private
bool Heightmap::GetAverageWorldHeight(const Vector3& worldPos, float radius, float &height) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2 a = Vector2((float)num, (float)num2);
    int32_t num5 = WIDTH + 1;
    float num6 = 0;
    int32_t num7 = 0;
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if (a.Distance(Vector2((float)j, (float)i)) <= num3 
                && j >= 0 && i >= 0 && j < num5&& i < num5) {
                num6 += this->GetHeight(j, i);
                num7++;
            }
        }
    }
    if (num7 == 0) {
        height = 0;
        return false;
    }
    height = num6 / (float)num7 + base.transform.position.y;
    return true;
}

// private
bool Heightmap::GetMinWorldHeight(const Vector3& worldPos, float radius, float &height) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    float num3 = radius;
    int32_t num4 = ceil(num3);
    Vector2 a = Vector2((float)num, (float)num2);
    int32_t num5 = WIDTH + 1;
    height = 99999;
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if (a.Distance(Vector2((float)j, (float)i)) <= num3 
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
    Vector2 a = Vector2((float)num, (float)num2);
    int32_t num5 = WIDTH + 1;
    height = -99999;
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if (a.Distance(Vector2((float)j, (float)i)) <= num3 
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

// public static
bool Heightmap::AtMaxLevelDepth(const Vector3& worldPos) {
    auto heightmap = FindHeightmap(worldPos);
    return heightmap && heightmap->AtMaxWorldLevelDepth(worldPos);
}

// public static
bool Heightmap::GetHeight(const Vector3& worldPos, float& height) {
    auto heightmap = FindHeightmap(worldPos);
    if (heightmap && heightmap->GetWorldHeight(worldPos, height)) {
        return true;
    }
    height = 0;
    return false;
}

// public static
bool Heightmap::GetAverageHeight(const Vector3& worldPos, float& radius, float height) {
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
    Ray ray = new Ray(point + Vector3::UP * 100.f, Vector3::DOWN);
    RaycastHit raycastHit;
    if (this->m_collider.Raycast(ray, raycastHit, 300)) {
        return raycastHit.point.y;
    }
    return -10000;
}

// private
void Heightmap::FindObjectsToMove(Vector3 worldPos, float area, std::vector<Rigidbody> &objects) {
    if (this->m_collider == nullptr) {
        return;
    }
    for (auto&& collider : Physics.OverlapBox(worldPos, Vector3(area / 2.f, 500, area / 2.f))) {
        if (!(collider == this->m_collider) && collider.attachedRigidbody) {
            Rigidbody attachedRigidbody = collider.attachedRigidbody;
            ZNetView component = attachedRigidbody.GetComponent<ZNetView>();
            if (!component || component.IsOwner()) {
                objects.push_back(attachedRigidbody);
            }
        }
    }
}

// private
void Heightmap::PaintCleared(Vector3 worldPos, float radius, TerrainModifier.PaintType paintType, bool heightCheck, bool apply) {
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
                case TerrainModifier.PaintType.Dirt:
                    color = color.Lerp(m_paintMaskDirt, num7);
                    break;
                case TerrainModifier.PaintType.Cultivate:
                    color = color.Lerp(m_paintMaskCultivated, num7);
                    break;
                case TerrainModifier.PaintType.Paved:
                    color = color.Lerp(m_paintMaskPaved, num7);
                    break;
                case TerrainModifier.PaintType.Reset:
                    color = color.Lerp(m_paintMaskNothing, num7);
                    break;
                }
                color.a = a2;
                this->m_paintMask.SetPixel(j, i, color);
            }
        }
    }

    if (apply) {
        this->m_paintMask.Apply();
    }
}

// public
float Heightmap::GetVegetationMask(const Vector3& worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos - Vector3(.5f, 0, .5f), x, y);
    return this->m_paintMask.GetPixel(x, y).a;
}

// public
bool Heightmap::IsCleared(const Vector3& worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos - Vector3(.5f, 0, .5f), x, y);
    Color pixel = this->m_paintMask.GetPixel(x, y);
    return pixel.r > 0.5f || pixel.g > 0.5f || pixel.b > 0.5f;
}

// public
bool Heightmap::IsCultivated(const Vector3& worldPos) {
    int32_t x;
    int32_t y;
    this->WorldToVertex(worldPos, x, y);
    return this->m_paintMask.GetPixel(x, y).g > 0.5f;
}

// public
void Heightmap::WorldToVertex(const Vector3& worldPos, int32_t& x, int32_t &y) {
    Vector3 vector = worldPos - base.transform.position;
    x = floor(vector.x + 0.5f) + WIDTH / 2;
    y = floor(vector.z + 0.5f) + WIDTH / 2;
}

// private
void Heightmap::WorldToNormalizedHM(const Vector3& worldPos, float& x, float y) {
    float num = (float)WIDTH * this->m_scale;
    Vector3 vector = worldPos - base.transform.position;
    x = vector.x / num + 0.5f;
    y = vector.z / num + 0.5f;
}

// private
void Heightmap::LevelTerrain(const Vector3& worldPos, float radius, bool square, float[] baseHeights, float[] levelOnly, bool playerModification) {
    int32_t num;
    int32_t num2;
    this->WorldToVertex(worldPos, num, num2);
    Vector3 vector = worldPos - base.transform.position;
    float num3 = radius;
    int32_t num4 = ceil(num3);
    int32_t num5 = WIDTH + 1;
    Vector2 a = Vector2(num, num2);
    for (int32_t i = num2 - num4; i <= num2 + num4; i++) {
        for (int32_t j = num - num4; j <= num + num4; j++) {
            if ((square || a.Distance(Vector2(j, i)) <= num3) && j >= 0 && i >= 0 && j < num5&& i < num5) {
                float num6 = vector.y;
                if (playerModification) {
                    float num7 = baseHeights[i * num5 + j];
                    num6 = VUtils::Math::Clamp(num6, num7 - 8f, num7 + 8f);
                    levelOnly[i * num5 + j] = num6;
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
    return this->m_paintMask.GetPixel(x, y);
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
    float num = (float)Heightmap::WIDTH * 0.5f;
    Vector3 position = base.transform.position;
    return point.x + radius >= position.x - num && point.x - radius <= position.x + num && point.z + radius >= position.z - num && point.z - radius <= position.z + num;
}

// public static
robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>, HashUtils::Hasher>& Heightmap::GetAllHeightmaps() {
    return m_heightmaps;
}

// public static
Heightmap* Heightmap::FindHeightmap(const Vector3& point) {
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->IsPointInside(point, 0)) {
            return heightmap.get();
        }
    }
    return nullptr;
}

// public static
void Heightmap::FindHeightmap(const Vector3& point, float radius, std::vector<Heightmap*>& heightmaps) {
    for (auto&& pair : m_heightmaps) {
        auto&& heightmap = pair.second;
        if (heightmap->IsPointInside(point, radius)) {
            heightmaps.push_back(heightmap.get());
        }
    }
}

// public static
Heightmap::Biome Heightmap::FindBiome(const Vector3& point) {
    auto heightmap = FindHeightmap(point);
    if (heightmap) {
        return heightmap->GetBiome(point);
    }
    return Biome::None;
}

// public static
bool Heightmap::HaveQueuedRebuild(const Vector3& point, float radius) {
    //tempHmaps.clear();
    std::vector<Heightmap*> heightmaps;
    FindHeightmap(point, radius, heightmaps);
    for (auto&& hmap : heightmaps) {
        //if (hmap->)
    }


    using (std::vector<Heightmap>.Enumerator enumerator = Heightmap.tempHmaps.GetEnumerator()) {
        while (enumerator.MoveNext()) {
            if (enumerator.Current.HaveQueuedRebuild()) {
                return true;
            }
        }
    }
    return false;
}

// public
void Heightmap::Clear() {
    this->m_heights.clear();
    this->m_paintMask = nullptr;
    this->m_materialInstance = nullptr;
    this->m_buildData = nullptr;
    if (this->m_collisionMesh) {
        this->m_collisionMesh.clear();
    }
    if (this->m_renderMesh) {
        this->m_renderMesh.clear();
    }
    if (this->m_collider) {
        this->m_collider.sharedMesh = nullptr;
    }
}

// public
TerrainComp Heightmap::GetAndCreateTerrainCompiler() {
    TerrainComp terrainComp = TerrainComp.FindTerrainCompiler(base.transform.position);
    if (terrainComp) {
        return terrainComp;
    }
    return UnityEngine.Object.Instantiate<GameObject>(this->m_terrainCompilerPrefab, base.transform.position, Quaternion::IDENTITY).GetComponent<TerrainComp>();
}

// public
Vector3 Heightmap::GetCenter() {
    return this->m_bounds.center;
}

