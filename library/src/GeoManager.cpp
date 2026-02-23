#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <mutex>
#include <quill/sinks/ConsoleSink.h>
#include <shared_mutex>
#include <vector>

#include "GeoManager.h"
#include "Types.h"

/*
    TODO

    this class is deep outdated since ashlands

    many changes have been made since which havnt been migrated into here

    
*/

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    #include "VUtilsMath.h"
    #include "VUtilsMath2.h"
    #include "VUtilsMathf.h"
    #include "VUtilsRandom.h"
    #include "ZoneManager.h"

auto GEO_MANAGER = std::make_unique<IGeoManager>();

IGeoManager *GeoManager()
{
    return GEO_MANAGER.get();
}

void IGeoManager::PostWorldInit()
{
    m_world = WorldManager()->GetWorld();
    assert(m_world);

    this->VersionSetup(m_world->m_worldGenVersion);
    auto state = VUtils::Random::State(this->m_world->m_seed);
    // TODO impl FastNoise
    //if (m_noiseGen == null)
    //{
    //    m_noiseGen = FastNoise(this->m_world.m_seed);
    //    m_noiseGen.SetNoiseType(FastNoise.NoiseType.Cellular);
    //    m_noiseGen.SetCellularDistanceFunction(FastNoise.CellularDistanceFunction.Euclidean);
    //    m_noiseGen.SetCellularReturnType(FastNoise.CellularReturnType.Distance);
    //    m_noiseGen.SetFractalOctaves(2);
    //}
    //m_noiseGen.SetSeed(0);
    this->m_offset0 = (float)state.range(-10000, 10000);
    this->m_offset1 = (float)state.range(-10000, 10000);
    this->m_offset2 = (float)state.range(-10000, 10000);
    this->m_offset3 = (float)state.range(-10000, 10000);
    this->m_river_random = VUtils::Random::State(
            state.range(std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max()));
    this->m_stream_random
            = VUtils::Random::State(state.range(std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max()));

    this->m_offset4 = (float)state.range(-10000, 10000);

    this->Pregenerate();
}

void IGeoManager::VersionSetup(int version)
{
    if (version <= 0)
    {
        this->m_minMountainDistance = 1500.0f;
    }

    if (version <= 1)
    {
        this->minDarklandNoise = 0.5f;
        this->maxMarshDistance = 8000.0f;
    }
}

void IGeoManager::Pregenerate()
{
    this->FindLakes();
    this->m_rivers = this->PlaceRivers();
    this->m_streams = this->PlaceStreams();
}

std::vector<Vector2f> IGeoManager::GetLakes()
{
    return this->m_lakes;
}

std::vector<IGeoManager::River> IGeoManager::GetRivers()
{
    return this->m_rivers;
}

std::vector<IGeoManager::River> IGeoManager::GetStreams()
{
    return this->m_streams;
}

void IGeoManager::FindLakes()
{
    std::vector<Vector2f> list;
    for (float num = -10000.0f; num <= 10000.0f; num = (float)((double)num + 128.0))
    {
        for (float num2 = -10000.0f; num2 <= 10000.0f; num2 = (float)((double)num2 + 128.0))
        {
            if (Vector2f(num2, num).magnitude() <= 10000.0f && this->GetBaseHeight(num2, num) < 0.05f)
            {
                list.push_back(Vector2f(num2, num));
            }
        }
    }
    this->m_lakes = this->MergePoints(list, 800.0f);
}

std::vector<Vector2f> IGeoManager::MergePoints(std::vector<Vector2f> &points, float range)
{
    std::vector<Vector2f> list;
    while (!points.empty()) {
        Vector2f vector = points[0];
        points.erase(points.begin());// not efficient for vector
        while (!points.empty()) {
            int num = FindClosest(points, vector, range);
            if (num == -1) {
                break;
            }
            vector      = (vector + points[num]) * 0.5f;
            points[num] = points[points.size() - 1];
            points.erase(points.end());// .erase(points.end());
        }
        list.push_back(vector);
    }
    return list;
}

int IGeoManager::FindClosest(std::vector<Vector2f> const& points, Vector2f const& p, float maxDistance)
{
    int num = -1;
    float num2 = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < points.size(); i++)
    {
        if (!(points[i] == p))
        {
            float num3 = p.distance_to(points[i]);
            if (num3 < maxDistance && num3 < num2)
            {
                num = (int)i;
                num2 = num3;
            }
        }
    }
    return num;
}

std::vector<IGeoManager::River> IGeoManager::PlaceStreams()
{
    //VUtils::Random::State state(this->m_streamSeed);
    std::vector<River> list;
    for (int i = 0; i < 3000; i++)
    {
        Vector2f vector;
        float num2;
        Vector2f vector2;
        if (this->FindStreamStartPoint(100, 26.0f, 31.0f, vector, num2) && this->FindStreamEndPoint(100, 36.0f, 44.0f, vector, 80.0f, 200.0f, vector2))
        {
            Vector2f vector3 = (vector + vector2) * 0.5f;
            float pregenerationHeight = this->GetPregenerationHeight(vector3.x, vector3.y);
            if (pregenerationHeight >= 26.0f && pregenerationHeight <= 44.0f)
            {
                River river;
                river.p0 = vector;
                river.p1 = vector2;
                river.center = vector3;
                river.widthMax = 20.0f;
                river.widthMin = 20.0f;
                float num3 = river.p0.distance_to(river.p1);
                river.curveWidth = (float)((double)num3 / 15.0);
                river.curveWavelength = (float)((double)num3 / 20.0);
                list.push_back(river);
            }
        }
    }
    this->RenderRivers(list);
    return list;
}

bool IGeoManager::FindStreamEndPoint(int iterations, float minHeight, float maxHeight, Vector2f const& start, float minLength, float maxLength, Vector2f &end)
{
    float num = (float)(((double)maxLength - (double)minLength) / (double)iterations);
    float num2 = maxLength;
    for (int i = 0; i < iterations; i++)
    {
        num2 = (float)((double)num2 - (double)num);
        float num3 = m_stream_random.range(0.0f, 6.2831855f);
        Vector2f vector = start + Vector2f(std::sin(num3), std::cos(num3)) * num2;
        float pregenerationHeight = this->GetPregenerationHeight(vector.x, vector.y);
        if (pregenerationHeight > minHeight && pregenerationHeight < maxHeight)
        {
            end = vector;
            return true;
        }
    }
    end = Vector2f::ZERO;
    return false;
}

bool IGeoManager::FindStreamStartPoint(int iterations, float minHeight, float maxHeight, Vector2f& p, float& starth)
{
    for (int i = 0; i < iterations; i++)
    {
        float num = m_stream_random.range(-10000.0f, 10000.0f);
        float num2 = m_stream_random.range(-10000.0f, 10000.0f);
        float pregenerationHeight = this->GetPregenerationHeight(num, num2);
        if (pregenerationHeight > minHeight && pregenerationHeight < maxHeight)
        {
            p = Vector2f(num, num2);
            starth = pregenerationHeight;
            return true;
        }
    }
    p = Vector2f::ZERO;
    starth = 0.0f;
    return false;
}

std::vector<IGeoManager::River> IGeoManager::PlaceRivers()
{
    std::vector<River> list;
    std::vector<Vector2f> list2 = this->m_lakes;
    while (list2.size() > 1)
    {
        Vector2f vector = list2[0];
        int num = this->FindRandomRiverEnd(list, this->m_lakes, vector, 2000.0f, 0.4f, 128.0f);
        if (num == -1 && !this->HaveRiver(list, vector))
        {
            num = this->FindRandomRiverEnd(list, this->m_lakes, vector, 5000.0f, 0.4f, 128.0f);
        }
        if (num != -1)
        {
            River river;
            river.p0 = vector;
            river.p1 = this->m_lakes[num];
            river.center = (river.p0 + river.p1) * 0.5f;
            river.widthMax = m_river_random.range(60.0f, 100.0f);
            river.widthMin = m_river_random.range(60.0f, river.widthMax);
            float num2 = river.p0.distance_to(river.p1);
            river.curveWidth = (float)((double)num2 / 15.0);
            river.curveWavelength = (float)((double)num2 / 20.0);
            list.push_back(river);
        }
        else
        {
            list2.erase(list2.begin());
        }
    }
    this->RenderRivers(list);
    return list;
}

/*
int IGeoManager::FindClosestRiverEnd(std::vector<River> const& rivers, std::vector<Vector2f> const& points, Vector2f const&p, float maxDistance, float heightLimit, float checkStep)
{
    int num = -1;
    float num2 = 99999.0f;
    for (int i = 0; i < points.size(); i++)
    {
        if (!(points[i] == p))
        {
            float num3 = p.distance_to(points[i]);
            if (num3 < maxDistance && num3 < num2 && !this->HaveRiver(rivers, p, points[i]) && this->IsRiverAllowed(p, points[i], checkStep, heightLimit))
            {
                num = i;
                num2 = num3;
            }
        }
    }
    return num;
}*/

int IGeoManager::FindRandomRiverEnd(std::vector<River> const& rivers, std::vector<Vector2f> const&points, Vector2f const&p, float maxDistance, float heightLimit, float checkStep)
{
    std::vector<int> list = std::vector<int>();
    for (std::size_t i = 0; i < points.size(); i++)
    {
        if (!(points[i] == p) && p.distance_to(points[i]) < maxDistance && !this->HaveRiver(rivers, p, points[i]) && this->IsRiverAllowed(p, points[i], checkStep, heightLimit))
        {
            list.push_back((int)i);
        }
    }
    if (list.empty())
    {
        return -1;
    }
    return list[m_river_random.range(0, list.size())];
}

bool IGeoManager::HaveRiver(std::vector<River> const&rivers, Vector2f const&p0)
{
    for (const auto& river : rivers)
    {
        if (river.p0 == p0 || river.p1 == p0)
        {
            return true;
        }
    }
    return false;
}

bool IGeoManager::HaveRiver(std::vector<River> const&rivers, Vector2f const&p0, Vector2f const&p1)
{
    for (const auto& river : rivers)
    {
        if ((river.p0 == p0 && river.p1 == p1) || (river.p0 == p1 && river.p1 == p0))
        {
            return true;
        }
    }
    return false;
}

bool IGeoManager::IsRiverAllowed(Vector2f const&p0, Vector2f const&p1, float step, float heightLimit)
{
    float num = p0.distance_to(p1);
    Vector2f normalized = (p1 - p0).normal();
    bool flag = true;
    for (float num2 = step; num2 <= (float)((double)num - (double)step); num2 = (float)((double)num2 + (double)step))
    {
        Vector2f vector = p0 + normalized * num2;
        float baseHeight = this->GetBaseHeight(vector.x, vector.y);
        if (baseHeight > heightLimit)
        {
            return false;
        }
        if (baseHeight > 0.05f)
        {
            flag = false;
        }
    }
    return !flag;
}

void IGeoManager::RenderRivers(std::vector<River> const&rivers)
{
    avledet::util::Map<Vector2i, std::vector<RiverPoint>> dictionary;
    for (const auto& river : rivers)
    {
        float num = (float)((double)river.widthMin / 8.0);
        Vector2f normalized = (river.p1 - river.p0).normal();
        Vector2f vector(-normalized.y, normalized.x);
        float num2 = river.p0.distance_to(river.p1);
        for (float num3 = 0.0f; num3 <= num2; num3 = (float)((double)num3 + (double)num))
        {
            float num4 = (float)((double)num3 / (double)river.curveWavelength);
            float num5 = (float)(std::sin((double)num4) * std::sin((double)num4 * 0.634119987487793) * std::sin((double)num4 * 0.3341200053691864) * (double)river.curveWidth);
            float num6 = m_river_random.range(river.widthMin, river.widthMax);
            Vector2f vector2 = river.p0 + normalized * num3 + vector * num5;
            this->AddRiverPoint(dictionary, vector2, num6, river);
        }
    }

    for (auto &&keyValuePair : dictionary) {
        auto &&list = m_riverPoints[keyValuePair.first];

        list.insert(list.end(), keyValuePair.second.begin(), keyValuePair.second.end());
    }
}

void IGeoManager::AddRiverPoint(avledet::util::Map<Vector2i, std::vector<RiverPoint>> &riverPoints, Vector2f const&p, float r, River const&river)
{
    Vector2i riverGrid = this->GetRiverGrid(p.x, p.y);
    
    int num = (int)std::ceil((double)r / 64.0);
    for (int i = riverGrid.y - num; i <= riverGrid.y + num; i++)
    {
        for (int j = riverGrid.x - num; j <= riverGrid.x + num; j++)
        {
            Vector2i vector2i = Vector2i(j, i);
            if (this->InsideRiverGrid(vector2i, p, r))
            {
                this->AddRiverPoint(riverPoints, vector2i, p, r, river);
            }
        }
    }
}

void IGeoManager::AddRiverPoint(avledet::util::Map<Vector2i, std::vector<RiverPoint>> &riverPoints, Vector2i const&grid, Vector2f const&p, float r, River const&river)
{
    // TODO perhaps emplace_back a direct ctor
    riverPoints[grid].push_back({p, r});
}

bool IGeoManager::InsideRiverGrid(Vector2i const&grid, Vector2f const& p, float r)
{
    Vector2f vector((float)((double)grid.x * 64.0), (float)((double)grid.y * 64.0));
    Vector2f vector2 = p - vector;
    return std::abs(vector2.x) < (float)((double)r + 32.0) && std::abs(vector2.y) < (float)((double)r + 32.0);
}

Vector2i IGeoManager::GetRiverGrid(float wx, float wy)
{
    int num = (int)std::floor(((double)wx + 32.0) / 64.0);
    int num2 = (int)std::floor(((double)wy + 32.0) / 64.0);
    return Vector2i(num, num2);
}

void IGeoManager::GetRiverWeight(float wx, float wy, float& weight, float& width)
{
    Vector2i riverGrid = GetRiverGrid(wx, wy);

    // we could put these later, but... should we? I dont like code duplication
    //  like it much less than redundant operations

    weight = 0.0f;
    width = 0.0f;

    {
        std::shared_lock rlock(m_mutRiverCache);
        if (riverGrid == m_cachedRiverGrid) {
            if (m_cachedRiverPoints) {
                GetWeight(*m_cachedRiverPoints, wx, wy, weight, width);
            }            
            return;
        }
    }

    {
        auto &&find = m_riverPoints.find(riverGrid);
        if (find != m_riverPoints.end()) {
            GetWeight(find->second, wx, wy, weight, width);
            {
                std::unique_lock wlock(m_mutRiverCache);

                m_cachedRiverGrid   = riverGrid;
                m_cachedRiverPoints = &find->second;
            }
        } else {
            std::unique_lock wlock(m_mutRiverCache);

            m_cachedRiverGrid   = riverGrid;
            m_cachedRiverPoints = nullptr;
        }
    }
}

void IGeoManager::GetWeight(std::vector<RiverPoint> const& points, float wx, float wy, float& weight, float& width)
{
    Vector2f vector(wx, wy);
    weight = 0.0f;
    width = 0.0f;
    float num = 0.0f;
    float num2 = 0.0f;
    for (RiverPoint riverPoint : points)
    {
        float num3 = riverPoint.p.sq_distance_to(vector);
        if (num3 < riverPoint.w2)
        {
            float num4 = (float)std::sqrt((double)num3);
            float num5 = (float)(1.0 - (double)num4 / (double)riverPoint.w);
            if (num5 > weight)
            {
                weight = num5;
            }
            num = (float)((double)num + (double)riverPoint.w * (double)num5);
            num2 = (float)((double)num2 + (double)num5);
        }
    }
    if (num2 > 0.0f)
    {
        width = (float)((double)num / (double)num2);
    }
}

VUtils::BiomeArea IGeoManager::GetBiomeArea(Vector3f const& point)
{
    VUtils::Biome biome = this->GetBiome(point);
    VUtils::Biome biome2 = this->GetBiome(point - Vector3f(-64.0f, 0.0f, -64.0f));
    VUtils::Biome biome3 = this->GetBiome(point - Vector3f(64.0f, 0.0f, -64.0f));
    VUtils::Biome biome4 = this->GetBiome(point - Vector3f(64.0f, 0.0f, 64.0f));
    VUtils::Biome biome5 = this->GetBiome(point - Vector3f(-64.0f, 0.0f, 64.0f));
    VUtils::Biome biome6 = this->GetBiome(point - Vector3f(-64.0f, 0.0f, 0.0f));
    VUtils::Biome biome7 = this->GetBiome(point - Vector3f(64.0f, 0.0f, 0.0f));
    VUtils::Biome biome8 = this->GetBiome(point - Vector3f(0.0f, 0.0f, -64.0f));
    VUtils::Biome biome9 = this->GetBiome(point - Vector3f(0.0f, 0.0f, 64.0f));
    if (biome == biome2 && biome == biome3 && biome == biome4 && biome == biome5 && biome == biome6 && biome == biome7 && biome == biome8 && biome == biome9)
    {
        return VUtils::BiomeArea::Median;
    }
    return VUtils::BiomeArea::Edge;
}

VUtils::Biome IGeoManager::GetBiome(Vector3f const& point)
{
    return this->GetBiome(point.x, point.z, 0.02f, false);
}

bool IGeoManager::IsAshlands(float x, float y)
{
    double num = (double)WorldAngle(x, y) * 100.0;
    return (double)VUtils::Math::magnitude(x, (float)((double)y + (double)ashlandsYOffset)) > (double)ashlandsMinDistance + num;
}

float IGeoManager::GetAshlandsOceanGradient(float x, float y)
{
    double num = (double)WorldAngle(x, y + ashlandsYOffset) * 100.0;
    return (float)(((double)VUtils::Math::magnitude(x, y + ashlandsYOffset) - ((double)ashlandsMinDistance + num)) / 300.0);
}

float IGeoManager::GetAshlandsOceanGradient(Vector2f const& pos)
{
    return GetAshlandsOceanGradient(pos.x, pos.y);
}

float IGeoManager::GetAshlandsOceanGradient(Vector3f const& pos)
{
    return GetAshlandsOceanGradient(pos.x, pos.z);
}

bool IGeoManager::IsDeepnorth(float x, float y)
{
    float num = (float)((double)WorldAngle(x, y) * 100.0);
    return Vector2f(x, (float)((double)y + 4000.0)).magnitude() > (float)(12000.0 + (double)num);
}

VUtils::Biome IGeoManager::GetBiome(float wx, float wy, float oceanLevel, bool waterAlwaysOcean)
{
    float num = VUtils::Math::magnitude(wx, wy);
    float baseHeight = this->GetBaseHeight(wx, wy);
    float num2 = (float)((double)WorldAngle(wx, wy) * 100.0);
    if (waterAlwaysOcean && this->GetHeight(wx, wy) <= oceanLevel)
    {
        return VUtils::Biome::Ocean;
    }
    if (IsAshlands(wx, wy))
    {
        return VUtils::Biome::AshLands;
    }
    if (!waterAlwaysOcean && baseHeight <= oceanLevel)
    {
        return VUtils::Biome::Ocean;
    }
    if (IsDeepnorth(wx, wy))
    {
        if (baseHeight > 0.4f)
        {
            return VUtils::Biome::Mountain;
        }
        return VUtils::Biome::DeepNorth;
    }
    else
    {
        if (baseHeight > 0.4f)
        {
            return VUtils::Biome::Mountain;
        }
        if (VUtils::Math::PerlinNoise((double)((float)((double)this->m_offset0 + (double)wx)) * 0.0010000000474974513, (double)((float)((double)this->m_offset0 + (double)wy)) * 0.0010000000474974513) > 0.6f && num > 2000.0f && num < this->maxMarshDistance && baseHeight > 0.05f && baseHeight < 0.25f)
        {
            return VUtils::Biome::Swamp;
        }
        if (VUtils::Math::PerlinNoise((double)((float)((double)this->m_offset4 + (double)wx)) * 0.0010000000474974513, (double)((float)((double)this->m_offset4 + (double)wy)) * 0.0010000000474974513) > this->minDarklandNoise && num > (float)(6000.0 + (double)num2) && num < 10000.0f)
        {
            return VUtils::Biome::Mistlands;
        }
        if (VUtils::Math::PerlinNoise((double)((float)((double)this->m_offset1 + (double)wx)) * 0.0010000000474974513, (double)((float)((double)this->m_offset1 + (double)wy)) * 0.0010000000474974513) > 0.4f && num > (float)(3000.0 + (double)num2) && num < 8000.0f)
        {
            return VUtils::Biome::Plains;
        }
        if (VUtils::Math::PerlinNoise((double)((float)((double)this->m_offset2 + (double)wx)) * 0.0010000000474974513, (double)((float)((double)this->m_offset2 + (double)wy)) * 0.0010000000474974513) > 0.4f && num > (float)(600.0 + (double)num2) && num < 6000.0f)
        {
            return VUtils::Biome::BlackForest;
        }
        if (num > (float)(5000.0 + (double)num2))
        {
            return VUtils::Biome::BlackForest;
        }
        return VUtils::Biome::Meadows;
    }
    
}

float IGeoManager::WorldAngle(float wx, float wy)
{
    return (float)std::sin((double)((float)((double)((float)std::atan2((double)wx, (double)wy)) * 20.0)));
}

float IGeoManager::GetBaseHeight(float wx, float wy)
{
    float num4 = VUtils::Math::magnitude(wx, wy);
    double num5 = (double)wx;
    double num6 = (double)wy;
    num5 += 100000.0 + (double)this->m_offset0;
    num6 += 100000.0 + (double)this->m_offset1;
    float num7 = 0.0f;
    num7 = (float)((double)num7 + (double)VUtils::Math::PerlinNoise(num5 * 0.0020000000949949026 * 0.5, num6 * 0.0020000000949949026 * 0.5) * (double)VUtils::Math::PerlinNoise(num5 * 0.003000000026077032 * 0.5, num6 * 0.003000000026077032 * 0.5) * 1.0);
    num7 = (float)((double)num7 + (double)VUtils::Math::PerlinNoise(num5 * 0.0020000000949949026 * 1.0, num6 * 0.0020000000949949026 * 1.0) * (double)VUtils::Math::PerlinNoise(num5 * 0.003000000026077032 * 1.0, num6 * 0.003000000026077032 * 1.0) * (double)num7 * 0.8999999761581421);
    num7 = (float)((double)num7 + (double)VUtils::Math::PerlinNoise(num5 * 0.004999999888241291 * 1.0, num6 * 0.004999999888241291 * 1.0) * (double)VUtils::Math::PerlinNoise(num5 * 0.009999999776482582 * 1.0, num6 * 0.009999999776482582 * 1.0) * 0.5 * (double)num7);
    num7 = (float)((double)num7 - 0.07000000029802322);
    double num8 = (double)VUtils::Math::PerlinNoise(num5 * 0.0020000000949949026 * 0.25 + 0.12300000339746475, num6 * 0.0020000000949949026 * 0.25 + 0.15123000741004944);
    float num9 = VUtils::Math::PerlinNoise(num5 * 0.0020000000949949026 * 0.25 + 0.32100000977516174, num6 * 0.0020000000949949026 * 0.25 + 0.23100000619888306);
    float num10 = std::abs((float)(num8 - (double)num9));
    float num11 = (float)(1.0 - (double)VUtils::Math::LerpStep(0.02f, 0.12f, num10));
    num11 = (float)((double)num11 * (double)VUtils::Math::SmoothStep(744.0f, 1000.0f, num4));
    num7 = (float)((double)num7 * (1.0 - (double)num11));
    if (num4 > 10000.0f)
    {
        float num12 = VUtils::Math::LerpStep(10000.0f, 10500.0f, num4);
        num7 = VUtils::Math::Lerp(num7, -0.2f, num12);
        float num13 = 10490.0f;
        if (num4 > num13)
        {
            float num14 = VUtils::Math::LerpStep(num13, 10500.0f, num4);
            num7 = VUtils::Math::Lerp(num7, -2.0f, num14);
        }
        return num7;
    }
    if (num4 < this->m_minMountainDistance && num7 > 0.28f)
    {
        float num15 = (float)VUtils::Math::Clamp01(((double)num7 - 0.2800000011920929) / 0.09999999403953552);
        num7 = VUtils::Math::Lerp(VUtils::Math::Lerp(0.28f, 0.38f, num15), num7, VUtils::Math::LerpStep((float)((double)this->m_minMountainDistance - 400.0), this->m_minMountainDistance, num4));
    }
    return num7;
}

float IGeoManager::AddRivers(float wx, float wy, float h)
{
    float num;
    float num2;
    this->GetRiverWeight(wx, wy, num, num2);
    if (num <= 0.0f)
    {
        return h;
    }
    float num3 = VUtils::Math::LerpStep(20.0f, 60.0f, num2);
    float num4 = VUtils::Math::Lerp(0.14f, 0.12f, num3);
    float num5 = VUtils::Math::Lerp(0.139f, 0.128f, num3);
    if (h > num4)
    {
        h = VUtils::Math::Lerp(h, num4, num);
    }
    if (h > num5)
    {
        float num6 = VUtils::Math::LerpStep(0.85f, 1.0f, num);
        h = VUtils::Math::Lerp(h, num5, num6);
    }
    return h;
}

float IGeoManager::GetHeight(float wx, float wy)
{
    VUtils::Biome biome = this->GetBiome(wx, wy, 0.02f, false);
    VUtils::Color color;
    return this->GetBiomeHeight(biome, wx, wy, color, false);
}

float IGeoManager::GetHeight(float wx, float wy, VUtils::Color &mask)
{
    VUtils::Biome biome = this->GetBiome(wx, wy, 0.02f, false);
    return this->GetBiomeHeight(biome, wx, wy, mask, false);
}

float IGeoManager::GetPregenerationHeight(float wx, float wy)
{
    VUtils::Biome biome = this->GetBiome(wx, wy, 0.02f, false);
    VUtils::Color color;
    return this->GetBiomeHeight(biome, wx, wy, color, true);
}

float IGeoManager::GetBiomeHeight(VUtils::Biome biome, float wx, float wy, VUtils::Color& mask, bool preGeneration)
{
    float num;
    if (preGeneration)
    {
        num = heightMultiplier; // GetHeightMultiplier();
    }
    else
    {
        num = (float)((double)heightMultiplier * this->CreateAshlandsGap(wx, wy) * this->CreateDeepNorthGap(wx, wy));
    }

    mask = VUtils::Colors::BLACK;
    if (VUtils::Math::magnitude(wx, wy) > 10500.0f)
    {
        return -2.0f * heightMultiplier; // GetHeightMultiplier();
    }

    switch (biome)
    {
    case VUtils::Biome::Meadows:
        return (float)((double)this->GetMeadowsHeight(wx, wy) * (double)num);
    case VUtils::Biome::Swamp:
        return (float)((double)this->GetMarshHeight(wx, wy) * (double)num);
    case VUtils::Biome::Mountain:
        return (float)((double)this->GetSnowMountainHeight(wx, wy) * (double)num);
    case VUtils::Biome::BlackForest:
        return (float)((double)this->GetForestHeight(wx, wy) * (double)num);
    case VUtils::Biome::Plains:
        return (float)((double)this->GetPlainsHeight(wx, wy) * (double)num);
    case VUtils::Biome::DeepNorth:
        return (float)((double)this->GetDeepNorthHeight(wx, wy) * (double)num);
    case VUtils::Biome::AshLands: {
        if (preGeneration)
        {
            return (float)((double)this->GetAshlandsHeightPregenerate(wx, wy) * (double)num);
        }
        return (float)((double)this->GetAshlandsHeight(wx, wy, mask, false) * (double)num);
    }
    case VUtils::Biome::Ocean:
        return (float)((double)this->GetOceanHeight(wx, wy) * (double)num);
    case VUtils::Biome::Mistlands: {
        if (preGeneration)
        {
            return (float)((double)this->GetForestHeight(wx, wy) * (double)num);
        }
        return (float)((double)this->GetMistlandsHeight(wx, wy, mask) * (double)num);
    }

    default:
        break;
    }
    
    return 0.0f;
}

float IGeoManager::GetMarshHeight(float wx, float wy)
{
    float num = wx;
    float num2 = wy;
    float num3 = 0.137f;
    wx = (float)((double)wx + 100000.0);
    wy = (float)((double)wy + 100000.0);
    double num4 = (double)wx;
    double num5 = (double)wy;
    float num6 = (float)((double)VUtils::Math::PerlinNoise(num4 * 0.03999999910593033, num5 * 0.03999999910593033) * (double)VUtils::Math::PerlinNoise(num4 * 0.07999999821186066, num5 * 0.07999999821186066));
    num3 = (float)((double)num3 + (double)num6 * 0.029999999329447746);
    num3 = this->AddRivers(num, num2, num3);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.10000000149011612, num5 * 0.10000000149011612) * 0.009999999776482582);
    return (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.4000000059604645, num5 * 0.4000000059604645) * 0.003000000026077032);
}

float IGeoManager::GetMeadowsHeight(float wx, float wy)
{
    float num = wx;
    float num2 = wy;
    float baseHeight = this->GetBaseHeight(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num3 = (double)wx;
    double num4 = (double)wy;
    float num5 = (float)((double)VUtils::Math::PerlinNoise(num3 * 0.009999999776482582, num4 * 0.009999999776482582) * (double)VUtils::Math::PerlinNoise(num3 * 0.019999999552965164, num4 * 0.019999999552965164));
    num5 = (float)((double)num5 + (double)VUtils::Math::PerlinNoise(num3 * 0.05000000074505806, num4 * 0.05000000074505806) * (double)VUtils::Math::PerlinNoise(num3 * 0.10000000149011612, num4 * 0.10000000149011612) * (double)num5 * 0.5);
    float num6 = baseHeight;
    num6 = (float)((double)num6 + (double)num5 * 0.10000000149011612);
    float num7 = 0.15f;
    float num8 = (float)((double)num6 - (double)num7);
    float num9 = (float)VUtils::Math::Clamp01((double)baseHeight / 0.4000000059604645);
    if (num8 > 0.0f)
    {
        num6 = (float)((double)num6 - (double)num8 * ((1.0 - (double)num9) * 0.75));
    }
    num6 = this->AddRivers(num, num2, num6);
    num6 = (float)((double)num6 + (double)VUtils::Math::PerlinNoise(num3 * 0.10000000149011612, num4 * 0.10000000149011612) * 0.009999999776482582);
    return (float)((double)num6 + (double)VUtils::Math::PerlinNoise(num3 * 0.4000000059604645, num4 * 0.4000000059604645) * 0.003000000026077032);
}

float IGeoManager::GetForestHeight(float wx, float wy)
{
    float num = wx;
    float num2 = wy;
    float num3 = this->GetBaseHeight(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num4 = (double)wx;
    double num5 = (double)wy;
    float num6 = (float)((double)VUtils::Math::PerlinNoise(num4 * 0.009999999776482582, num5 * 0.009999999776482582) * (double)VUtils::Math::PerlinNoise(num4 * 0.019999999552965164, num5 * 0.019999999552965164));
    num6 = (float)((double)num6 + (double)VUtils::Math::PerlinNoise(num4 * 0.05000000074505806, num5 * 0.05000000074505806) * (double)VUtils::Math::PerlinNoise(num4 * 0.10000000149011612, num5 * 0.10000000149011612) * (double)num6 * 0.5);
    num3 = (float)((double)num3 + (double)num6 * 0.10000000149011612);
    num3 = this->AddRivers(num, num2, num3);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.10000000149011612, num5 * 0.10000000149011612) * 0.009999999776482582);
    return (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.4000000059604645, num5 * 0.4000000059604645) * 0.003000000026077032);
}

float IGeoManager::GetMistlandsHeight(float wx, float wy, VUtils::Color &mask)
{
    float num = wx;
    float num2 = wy;
    float num3 = this->GetBaseHeight(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num4 = (double)wx;
    double num5 = (double)wy;
    float num6 = VUtils::Math::PerlinNoise(num4 * 0.019999999552965164 * 0.699999988079071, num5 * 0.019999999552965164 * 0.699999988079071) * VUtils::Math::PerlinNoise(num4 * 0.03999999910593033 * 0.699999988079071, num5 * 0.03999999910593033 * 0.699999988079071);
    num6 = (float)((double)num6 + (double)VUtils::Math::PerlinNoise(num4 * 0.029999999329447746 * 0.699999988079071, num5 * 0.029999999329447746 * 0.699999988079071) * (double)VUtils::Math::PerlinNoise(num4 * 0.05000000074505806 * 0.699999988079071, num5 * 0.05000000074505806 * 0.699999988079071) * (double)num6 * 0.5);
    num6 = ((num6 > 0.0f) ? ((float)std::pow((double)num6, 1.5)) : num6);
    num3 = (float)((double)num3 + (double)num6 * 0.4000000059604645);
    num3 = this->AddRivers(num, num2, num3);
    float num7 = (float)VUtils::Math::Clamp01((double)num6 * 7.0);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.10000000149011612, num5 * 0.10000000149011612) * 0.029999999329447746 * (double)num7);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.4000000059604645, num5 * 0.4000000059604645) * 0.009999999776482582 * (double)num7);
    float num8 = (float)(1.0 - (double)num7 * 1.2000000476837158);
    num8 = (float)((double)num8 - (1.0 - (double)VUtils::Math::LerpStep(0.1f, 0.3f, num7)));
    float num9 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.4000000059604645, num5 * 0.4000000059604645) * 0.0020000000949949026);
    float num10 = num3;
    num10 = (float)((double)num10 * 400.0);
    num10 = std::ceil(num10);
    num10 = (float)((double)num10 / 400.0);
    num3 = VUtils::Math::Lerp(num9, num10, num7);
    mask = VUtils::Color(0.0f, 0.0f, 0.0f, num8);
    return num3;
}

float IGeoManager::GetPlainsHeight(float wx, float wy)
{
    float num = wx;
    float num2 = wy;
    float baseHeight = this->GetBaseHeight(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num3 = (double)wx;
    double num4 = (double)wy;
    float num5 = (float)((double)VUtils::Math::PerlinNoise(num3 * 0.009999999776482582, num4 * 0.009999999776482582) * (double)VUtils::Math::PerlinNoise(num3 * 0.019999999552965164, num4 * 0.019999999552965164));
    num5 = (float)((double)num5 + (double)VUtils::Math::PerlinNoise(num3 * 0.05000000074505806, num4 * 0.05000000074505806) * (double)VUtils::Math::PerlinNoise(num3 * 0.10000000149011612, num4 * 0.10000000149011612) * (double)num5 * 0.5);
    float num6 = baseHeight;
    num6 = (float)((double)num6 + (double)num5 * 0.10000000149011612);
    float num7 = 0.15f;
    float num8 = num6 - num7;
    float num9 = (float)VUtils::Math::Clamp01((double)baseHeight / 0.4000000059604645);
    if (num8 > 0.0f)
    {
        num6 = (float)((double)num6 - (double)num8 * (1.0 - (double)num9) * 0.75);
    }
    num6 = this->AddRivers(num, num2, num6);
    num6 = (float)((double)num6 + (double)VUtils::Math::PerlinNoise(num3 * 0.10000000149011612, num4 * 0.10000000149011612) * 0.009999999776482582);
    return (float)((double)num6 + (double)VUtils::Math::PerlinNoise(num3 * 0.4000000059604645, num4 * 0.4000000059604645) * 0.003000000026077032);
}

float IGeoManager::GetMenuHeight(float wx, float wy)
{
    double baseHeight = (double)this->GetBaseHeight(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num = (double)wx;
    double num2 = (double)wy;
    float num3 = VUtils::Math::PerlinNoise(num * 0.009999999776482582, num2 * 0.009999999776482582) * VUtils::Math::PerlinNoise(num * 0.019999999552965164, num2 * 0.019999999552965164);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num * 0.05000000074505806, num2 * 0.05000000074505806) * (double)VUtils::Math::PerlinNoise(num * 0.10000000149011612, num2 * 0.10000000149011612) * (double)num3 * 0.5);
    return (float)((double)((float)((double)((float)(baseHeight + (double)num3 * 0.10000000149011612)) + (double)VUtils::Math::PerlinNoise(num * 0.10000000149011612, num2 * 0.10000000149011612) * 0.009999999776482582)) + (double)VUtils::Math::PerlinNoise(num * 0.4000000059604645, num2 * 0.4000000059604645) * 0.003000000026077032);
}

float IGeoManager::GetAshlandsHeightPregenerate(float wx, float wy)
{
    float num = wx;
    float num2 = wy;
    float num3 = this->GetBaseHeight(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num4 = (double)wx;
    double num5 = (double)wy;
    float num6 = (float)((double)VUtils::Math::PerlinNoise(num4 * 0.009999999776482582, num5 * 0.009999999776482582) * (double)VUtils::Math::PerlinNoise(num4 * 0.019999999552965164, num5 * 0.019999999552965164));
    num6 = (float)((double)num6 + (double)VUtils::Math::PerlinNoise(num4 * 0.05000000074505806, num5 * 0.05000000074505806) * (double)VUtils::Math::PerlinNoise(num4 * 0.10000000149011612, num5 * 0.10000000149011612) * (double)num6 * 0.5);
    num3 = (float)((double)num3 + (double)num6 * 0.10000000149011612);
    num3 = (float)((double)num3 + 0.10000000149011612);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.10000000149011612, num5 * 0.10000000149011612) * 0.009999999776482582);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num4 * 0.4000000059604645, num5 * 0.4000000059604645) * 0.003000000026077032);
    return this->AddRivers(num, num2, num3);
}

float IGeoManager::GetAshlandsHeight(float wx, float wy, VUtils::Color &mask, bool cheap)
{
    return 10.0f;
/*
    // TODO impl FastNoise
    double num = (double)wx;
    double num2 = (double)wy;
    double num3 = (double)this->GetBaseHeight((float)num, (float)num2, false);
    double num4 = (double)WorldAngle((float)num, (float)num2) * 100.0;
    double num5 = VUtils::Math::magnitude(num, num2 + (double)ashlandsYOffset - (double)ashlandsYOffset * 0.3) - ((double)ashlandsMinDistance + num4);
    num5 = std::abs(num5) / 1000.0;
    num5 = 1.0 - VUtils::Math::Clamp01(num5);
    num5 = VUtils::Math::MathfLikeSmoothStep(0.1, 1.0, num5);
    double num6 = std::abs(num);
    num6 = 1.0 - VUtils::Math::Clamp01(num6 / 7500.0);
    num5 *= num6;
    double num7 = VUtils::Math::magnitude(num, num2) - 10150.0;
    num7 = 1.0 - VUtils::Math::Clamp01(num7 / 600.0);
    num += (double)(100000.0f + this->m_offset3);
    num2 += (double)(100000.0f + this->m_offset3);
    double num8 = 0.0;
    double num9 = 1.0;
    double num10 = 0.33000001311302185;
    int num11 = (cheap ? 2 : 5);
    for (int i = 0; i < num11; i++)
    {
        num8 += num9 * VUtils::Math::MathfLikeSmoothStep(0.0, 1.0, m_noiseGen.GetCellular(num * num10, num2 * num10));
        num10 *= 2.0;
        num9 *= 0.5;
    }
    num8 = VUtils::Math::Remap(num8, -1.0, 1.0, 0.0, 1.0);
    double num12 = VUtils::Math::Lerp(num5, VUtils::Math::BlendOverlay(num5, num8), 0.5);
    double num13 = (double)(VUtils::Math::PerlinNoise(num * 0.009999999776482582, num2 * 0.009999999776482582) * VUtils::Math::PerlinNoise(num * 0.019999999552965164, num2 * 0.019999999552965164));
    num13 += (double)(VUtils::Math::PerlinNoise(num * 0.05000000074505806, num2 * 0.05000000074505806) * VUtils::Math::PerlinNoise(num * 0.10000000149011612, num2 * 0.10000000149011612)) * num13 * 0.5;
    double num14 = VUtils::Math::Lerp(num3, 0.15000000596046448, 0.75);
    num14 += num12 * 0.5;
    num14 = VUtils::Math::Lerp(-1.0, num14, VUtils::Math::MathfLikeSmoothStep(0.0, 1.0, num7));
    double num15 = 0.15;
    double num16 = 0.0;
    double num17 = 1.0;
    double num18 = 8.0;
    int num19 = (cheap ? 2 : 3);
    for (int j = 0; j < num19; j++)
    {
        num16 += num17 * m_noiseGen.GetCellular(num * num18, num2 * num18);
        num18 *= 2.0;
        num17 *= 0.5;
    }
    num16 = VUtils::Math::Remap(num16, -1.0, 1.0, 0.0, 1.0);
    num16 = VUtils::Math::Clamp01(std::pow(num16, 4.0) * 2.0);
    double num20 = m_noiseGen.GetSimplexFractal(num * 0.075, num2 * 0.075);
    num20 = VUtils::Math::Remap(num20, -1.0, 1.0, 0.0, 1.0);
    num20 = std::pow(num20, 1.399999976158142);
    num14 *= num20;
    double num21 = VUtils::Math::Fbm(Vector2f((float)(num * 0.009999999776482582), (float)(num2 * 0.009999999776482582)), 3, 2.0, 0.5);
    num21 *= VUtils::Math::Clamp01(VUtils::Math::Remap(num5, 0.0, 0.5, 0.5, 1.0));
    num21 = VUtils::Math::LerpStep(0.699999988079071, 1.0, num21);
    num21 = std::pow(num21, 2.0);
    double num22 = VUtils::Math::BlendOverlay(num21, num16);
    num22 *= VUtils::Math::Clamp01((num14 - num15 - 0.02) / 0.01);
    double num23 = (double)VUtils::Math::PerlinNoise(num * 0.05 + 5124.0, num2 * 0.05 + 5000.0);
    num23 = std::pow(num23, 2.0);
    num23 = VUtils::Math::Remap(num23, 0.0, 1.0, 0.009999999776482582, 0.054999999701976776);
    double num24 = (double)std::clamp((float)(num14 - num23), (float)(num15 + 0.009999999776482582), 5000.0f);
    num14 = VUtils::Math::Lerp(num14, num24, num22);
    mask = VUtils::Color(0.0f, 0.0f, 0.0f, (float)num22);
    return (float)num14;*/
}

float IGeoManager::GetEdgeHeight(float wx, float wy)
{
    float num = VUtils::Math::magnitude(wx, wy);
    float num2 = 10490.0f;
    if (num > num2)
    {
        float num3 = VUtils::Math::LerpStep(num2, 10500.0f, num);
        return (float)(-2.0 * (double)num3);
    }
    float num4 = VUtils::Math::LerpStep(10000.0f, 10100.0f, num);
    float num5 = this->GetBaseHeight(wx, wy);
    num5 = VUtils::Math::Lerp(num5, 0.0f, num4);
    return this->AddRivers(wx, wy, num5);
}

float IGeoManager::GetOceanHeight(float wx, float wy)
{
    return this->GetBaseHeight(wx, wy);
}

float IGeoManager::BaseHeightTilt(float wx, float wy)
{
    float baseHeight = this->GetBaseHeight((float)((double)wx - 1.0), wy);
    double baseHeight2 = (double)this->GetBaseHeight((float)((double)wx + 1.0), wy);
    float baseHeight3 = this->GetBaseHeight(wx, (float)((double)wy - 1.0));
    float baseHeight4 = this->GetBaseHeight(wx, (float)((double)wy + 1.0));
    return (float)((double)std::abs((float)(baseHeight2 - (double)baseHeight)) + (double)std::abs((float)((double)baseHeight3 - (double)baseHeight4)));
}

float IGeoManager::GetSnowMountainHeight(float wx, float wy)
{
    float num = wx;
    float num2 = wy;
    float num3 = this->GetBaseHeight(wx, wy);
    float num4 = this->BaseHeightTilt(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num5 = (double)wx;
    double num6 = (double)wy;
    float num7 = (float)((double)num3 - 0.4000000059604645);
    num3 = (float)((double)num3 + (double)num7);
    float num8 = (float)((double)VUtils::Math::PerlinNoise(num5 * 0.009999999776482582, num6 * 0.009999999776482582) * (double)VUtils::Math::PerlinNoise(num5 * 0.019999999552965164, num6 * 0.019999999552965164));
    num8 = (float)((double)num8 + (double)VUtils::Math::PerlinNoise(num5 * 0.05000000074505806, num6 * 0.05000000074505806) * (double)VUtils::Math::PerlinNoise(num5 * 0.10000000149011612, num6 * 0.10000000149011612) * (double)num8 * 0.5);
    num3 = (float)((double)num3 + (double)num8 * 0.20000000298023224);
    num3 = this->AddRivers(num, num2, num3);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num5 * 0.10000000149011612, num6 * 0.10000000149011612) * 0.009999999776482582);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num5 * 0.4000000059604645, num6 * 0.4000000059604645) * 0.003000000026077032);
    return (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num5 * 0.20000000298023224, num6 * 0.20000000298023224) * 2.0 * (double)num4);
}

float IGeoManager::GetDeepNorthHeight(float wx, float wy)
{
    float num = wx;
    float num2 = wy;
    float num3 = this->GetBaseHeight(wx, wy);
    wx = (float)((double)wx + 100000.0 + (double)this->m_offset3);
    wy = (float)((double)wy + 100000.0 + (double)this->m_offset3);
    double num4 = (double)wx;
    double num5 = (double)wy;
    float num6 = std::max(0.0f, (float)((double)num3 - 0.4000000059604645));
    num3 = (float)((double)num3 + (double)num6);
    float num7 = (float)((double)VUtils::Math::PerlinNoise(num4 * 0.009999999776482582, num5 * 0.009999999776482582) * (double)VUtils::Math::PerlinNoise(num4 * 0.019999999552965164, num5 * 0.019999999552965164));
    num7 = (float)((double)num7 + (double)VUtils::Math::PerlinNoise(num4 * 0.05000000074505806, num5 * 0.05000000074505806) * (double)VUtils::Math::PerlinNoise(num4 * 0.10000000149011612, num5 * 0.10000000149011612) * (double)num7 * 0.5);
    num3 = (float)((double)num3 + (double)num7 * 0.20000000298023224);
    num3 = (float)((double)num3 * 1.2000000476837158);
    num3 = this->AddRivers(num, num2, num3);
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise((double)(wx * 0.1f), (double)(wy * 0.1f)) * 0.009999999776482582);
    return (float)((double)num3 + (double)VUtils::Math::PerlinNoise((double)(wx * 0.4f), (double)(wy * 0.4f)) * 0.003000000026077032);
}

double IGeoManager::CreateAshlandsGap(float wx, float wy)
{
    double num = (double)WorldAngle(wx, wy) * 100.0;
    double num2 = (double)VUtils::Math::magnitude(wx, wy + ashlandsYOffset) - ((double)ashlandsMinDistance + num);
    num2 = VUtils::Math::Clamp01(std::abs(num2) / 400.0);
    return VUtils::Math::MathfLikeSmoothStep(0.0, 1.0, (double)((float)num2));
}

double IGeoManager::CreateDeepNorthGap(float wx, float wy)
{
    double num = (double)WorldAngle(wx, wy) * 100.0;
    double num2 = (double)VUtils::Math::magnitude(wx, wy + 4000.0f) - (12000.0 + num);
    num2 = VUtils::Math::Clamp01(std::abs(num2) / 400.0);
    return VUtils::Math::MathfLikeSmoothStep(0.0, 1.0, (double)((float)num2));
}

bool IGeoManager::InForest(Vector3f const& pos)
{
    return GetForestFactor(pos) < 1.15f;
}

float IGeoManager::GetForestFactor(Vector3f const& pos)
{
    float num = 0.4f;
    return VUtils::Math::Fbm(pos * 0.01f * num, 3, 1.6f, 0.7f);
}

void IGeoManager::GetTerrainDelta(VUtils::Random::State &state, Vector3f const& center, float radius, float &delta, Vector3f &slopeDirection)
{
    int num = 10;
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();
    Vector3f vector = center;
    Vector3f vector2 = center;
    for (int i = 0; i < num; i++)
    {
        Vector2f vector3 = state.inside_unit_circle() * radius;
        Vector3f vector4 = center + Vector3f(vector3.x, 0.0f, vector3.y);
        float height = this->GetHeight(vector4.x, vector4.z);
        if (height < num3)
        {
            num3 = height;
            vector2 = vector4;
        }
        if (height > num2)
        {
            num2 = height;
            vector = vector4;
        }
    }
    delta = (float)((double)num2 - (double)num3);
    slopeDirection = (vector2 - vector).normal();
}

// public
int IGeoManager::GetSeed()
{
    return m_world->m_seed;
}
#endif// AVL_GENERATE_ZONES