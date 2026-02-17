#include <cstddef>
#include <cstdlib>
#include <quill/sinks/ConsoleSink.h>

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
    LOG_INFO(AVL_LOGGER, "Initializing GeoManager");

    m_world = WorldManager()->GetWorld();
    assert(m_world);

    /* "VersionSetup"
    */
    if (m_world->m_worldGenVersion <= 0) {
        m_minMountainDistance = 1500;
    }

    if (m_world->m_worldGenVersion <= 1) {
        minDarklandNoise = 0.5f;
        maxMarshDistance = 8000;
    }
    // end

    VUtils::Random::State state(m_world->m_seed);
    m_offset0 = (float)state.range(-worldSize, worldSize);
    m_offset1 = (float)state.range(-worldSize, worldSize);
    m_offset2 = (float)state.range(-worldSize, worldSize);
    m_offset3 = (float)state.range(-worldSize, worldSize);
    m_riverSeed
            = state.range(std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max());
    m_streamSeed
            = state.range(std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max());
    m_offset4 = (float)state.range(-worldSize, worldSize);

    // TODO rename run-once generator functions from 'Find...' to 'Generate...' for clarity

    // TODO devs added a FastNoise class, which is much more
    //  complicated than Unity Perlin Noise...
    //  in no time, we're going to match minecraft levels of complexity...

    Generate();
}

void IGeoManager::Generate()
{
    GenerateLakes();
    GenerateRivers();
    GenerateStreams();
}

void IGeoManager::GenerateLakes()
{
    std::vector<Vector2f> list;
    // good on the devs for finally realizing the precision issues when perlin float values become large
    for (float num = -worldSize; num <= worldSize; num = (float)((double)num + 128.0)) {
        for (float num2 = -worldSize; num2 <= worldSize; num2 = (float)((double)num2 + 128.0)) {
            if (VUtils::Math::magnitude(num2, num) <= worldSize && GetBaseHeight(num2, num) < 0.05f) {
                list.push_back(Vector2f(num2, num));
            }
        }
    }
    m_lakes = MergePoints(list, 800);
}

// Basically blender merge nearby vertices
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
            points.pop_back();// .erase(points.end());
        }
        list.push_back(vector);
    }
    return list;
}

// Return the index in points of the nearest point to p
int IGeoManager::FindClosest(std::vector<Vector2f> const &points, Vector2f p, float maxDistance)
{
    int result = -1;
    float num  = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < points.size(); i++) {
        if (!(points[i] == p)) {
            //float num2 = p.distance_to(points[i]); // not optimal
            float num2 = p.sq_distance_to(points[i]);
            if (num2 < maxDistance * maxDistance && num2 < num) {
                result = (int) i;
                num    = num2;
            }
        }
    }
    return result;
}

void IGeoManager::GenerateStreams()
{
    VUtils::Random::State state(m_streamSeed);
    //int num = 0;
    for (int i = 0; i < streams; i++) {
        Vector2f vector;
        float num2;
        Vector2f vector2;// out
        if (FindStreamStartPoint(state, 100, 26, 31, vector, num2)
            && FindStreamEndPoint(state, 100, 36, 44, vector, 80, 200, vector2)) {
            Vector2f vector3 = (vector + vector2) * 0.5f;
            float height     = GetGenerationHeight(vector3.x, vector3.y);
            if (height >= 26 && height <= 44) {
                River river;
                river.p0              = vector;
                river.p1              = vector2;
                river.center          = vector3;
                river.widthMax        = 20;
                river.widthMin        = 20;
                float num3            = river.p0.distance_to(river.p1);// use sqdist?
                river.curveWidth      = num3 / 15;
                river.curveWavelength = num3 / 20;
                m_streams.push_back(river);                            // use move / emplacer
                //num++;
            }
        }
    }
    RenderRivers(state, m_streams);
}

bool IGeoManager::FindStreamEndPoint(VUtils::Random::State &state, int iterations, float minHeight,
                                     float maxHeight, Vector2f start, float minLength, float maxLength,
                                     Vector2f &end)
{
    float num  = (maxLength - minLength) / (float) iterations;
    float num2 = maxLength;
    for (int i = 0; i < iterations; i++) {
        num2 -= num;
        float f         = state.range(0.f, (float) (VUtils::PI * 2.0));
        Vector2f vector = start + Vector2f(std::sin(f), std::cos(f)) * num2;
        float height    = GetGenerationHeight(vector.x, vector.y);
        if (height > minHeight && height < maxHeight) {
            end = vector;
            return true;
        }
    }
    end = Vector2f::ZERO;
    return false;
}

bool IGeoManager::FindStreamStartPoint(VUtils::Random::State &state, int iterations, float minHeight,
                                       float maxHeight, Vector2f &p, float &starth)
{
    for (int i = 0; i < iterations; i++) {
        auto num    = state.range((float) -worldSize, (float) worldSize);
        auto num2   = state.range((float) -worldSize, (float) worldSize);
        auto height = GetGenerationHeight(num, num2);
        if (height > minHeight && height < maxHeight) {
            p      = Vector2f(num, num2);
            starth = height;
            return true;
        }
    }
    p      = Vector2f::ZERO;
    starth = 0;
    return false;
}

void IGeoManager::GenerateRivers()
{
    VUtils::Random::State state(m_riverSeed);

    //std::vector<River> list;
    std::vector<Vector2f> list2(m_lakes);// TODO use list

    while (list2.size() > 1) {
        auto &&vector = list2[0];
        int num       = FindRandomRiverEnd(state, m_rivers, m_lakes, vector, 2000, 0.4f, 128);
        if (num == -1 && !HaveRiver(m_rivers, vector)) {
            num = FindRandomRiverEnd(state, m_rivers, m_lakes, vector, 5000, 0.4f, 128);
        }

        if (num != -1) {
            River river;
            river.p0              = vector;
            river.p1              = m_lakes[(std::size_t)num];
            river.center          = (river.p0 + river.p1) * 0.5f;
            river.widthMax        = state.range(minRiverWidth, maxRiverWidth);
            river.widthMin        = state.range(minRiverWidth, river.widthMax);
            float num2            = river.p0.distance_to(river.p1);
            river.curveWidth      = num2 / 15.f;
            river.curveWavelength = num2 / 20.f;
            m_rivers.push_back(river);
        } else {
            list2.erase(list2.begin());
        }
    }
    RenderRivers(state, m_rivers);
}

int IGeoManager::FindRandomRiverEnd(VUtils::Random::State &state, std::vector<River> const &rivers,
                                    std::vector<Vector2f> const &points, Vector2f p, float maxDistance,
                                    float heightLimit, float checkStep) const
{

    std::vector<int> list;
    for (std::size_t i = 0; i < points.size(); i++) {
        if (!(points[i] == p) && p.distance_to(points[i]) < maxDistance && !HaveRiver(rivers, p, points[i])
            && IsRiverAllowed(p, points[i], checkStep, heightLimit)) {
            list.push_back((int) i);
        }
    }

    if (list.empty())
        return -1;

    return list[(std::size_t)state.range(0, (int) list.size())];
}

bool IGeoManager::HaveRiver(std::vector<River> const &rivers, Vector2f p0) const
{
    for (auto &&river : rivers) {
        if (river.p0 == p0 || river.p1 == p0) {
            return true;
        }
    }
    return false;
}

bool IGeoManager::HaveRiver(std::vector<River> const &rivers, Vector2f p0, Vector2f p1) const
{
    for (auto &&river : rivers) {
        if ((river.p0 == p0 && river.p1 == p1) || (river.p0 == p1 && river.p1 == p0)) {
            return true;
        }
    }
    return false;
}

bool IGeoManager::IsRiverAllowed(Vector2f p0, Vector2f p1, float step, float heightLimit) const
{
    float num           = p0.distance_to(p1);
    Vector2f normalized = (p1 - p0).normal();
    bool flag           = true;
    for (float num2 = step; num2 <= num - step; num2 += step) {
        Vector2f vector  = p0 + normalized * num2;
        float baseHeight = GetBaseHeight(vector.x, vector.y);
        if (baseHeight > heightLimit)
            return false;

        if (baseHeight > 0.05f)
            flag = false;
    }
    return !flag;
}

void IGeoManager::RenderRivers(VUtils::Random::State &state, std::vector<River> const &rivers)
{
    //Dictionary<Vector2i, List<WorldGenerator.RiverPoint>> dictionary;
    avledet::util::Map<Vector2i, std::vector<RiverPoint>> dictionary;
    for (auto &&river : rivers) {
        float num                 = river.widthMin / 8.f;
        Vector2f const normalized = (river.p1 - river.p0).normal();
        Vector2f const a(-normalized.y, normalized.x);
        float num2 = river.p0.distance_to(river.p1);

        for (float num3 = 0; num3 <= num2; num3 += num) {
            float num4 = num3 / river.curveWavelength;
            float d    = std::sin(num4) * std::sin(num4 * 0.63412f) * std::sin(num4 * 0.33412f)
                      * river.curveWidth;
            float r    = state.range(river.widthMin, river.widthMax);
            Vector2f p = river.p0 + normalized * num3 + a * d;
            AddRiverPoint(dictionary, p, r);
        }
    }

    for (auto &&keyValuePair : dictionary) {
        auto &&list = m_riverPoints[keyValuePair.first];

        list.insert(list.end(), keyValuePair.second.begin(), keyValuePair.second.end());
    }
}

void IGeoManager::AddRiverPoint(avledet::util::Map<Vector2i, std::vector<RiverPoint>> &riverPoints,
                                Vector2f p, float r)
{
    Vector2i riverGrid = GetRiverGrid(p.x, p.y);
    int num            = (int) std::ceil(r / riverGridSize);// Mathf.CeilToInt(r / 64);
    for (int i = riverGrid.y - num; i <= riverGrid.y + num; i++) {
        for (int j = riverGrid.x - num; j <= riverGrid.x + num; j++) {
            Vector2i grid(j, i);
            if (InsideRiverGrid(grid, p, r))
                AddRiverPoint(riverPoints, grid, p, r);
        }
    }
}

void IGeoManager::AddRiverPoint(avledet::util::Map<Vector2i, std::vector<RiverPoint>> &riverPoints,
                                Vector2i grid, Vector2f p, float r)
{
    riverPoints[grid].push_back({p, r});
}

void IGeoManager::GetRiverWeight(float wx, float wy, float &outWeight, float &outWidth)
{
    Vector2i riverGrid = GetRiverGrid(wx, wy);

    std::scoped_lock<std::mutex> lock(m_mutRiverCache);
    if (riverGrid == m_cachedRiverGrid) {
        if (m_cachedRiverPoints) {
            return GetWeight(*m_cachedRiverPoints, wx, wy, outWeight, outWidth);
        }
    } else {
        auto &&find = m_riverPoints.find(riverGrid);
        if (find != m_riverPoints.end()) {
            GetWeight(find->second, wx, wy, outWeight, outWidth);
            m_cachedRiverGrid   = riverGrid;
            m_cachedRiverPoints = &find->second;
            return;
        }

        m_cachedRiverGrid   = riverGrid;
        m_cachedRiverPoints = nullptr;
    }

    outWeight = 0;
    outWidth  = 0;
}

void IGeoManager::GetWeight(std::vector<RiverPoint> const &points, float wx, float wy, float &outWeight,
                            float &outWidth)
{

    outWeight = 0;
    outWidth  = 0;

    Vector2f b(wx, wy);
    float num  = 0;
    float num2 = 0;

    for (auto &&riverPoint : points) {
        float num3 = (riverPoint.p - b).sq_magnitude();
        if (num3 < riverPoint.w2) {
            float num4 = std::sqrt(num3);
            float num5 = 1.f - num4 / riverPoint.w;
            outWeight  = std::max(num5, outWeight);

            num += riverPoint.w * num5;
            num2 += num5;
        }
    }

    if (num2 > 0.f)
        outWidth = num / num2;
}

float IGeoManager::WorldAngle(float wx, float wy)
{
    return std::sin(std::atan2(wx, wy) * 20.f);
}

float IGeoManager::GetBaseHeight(float wx1, float wy1) const
{
    //float num2 = VUtils.Length(wx, wy);
    float num2 = VUtils::Math::magnitude(wx1, wy1);
    double wx = (double)wx + 100000.0 + (double)m_offset0;
    double wy = (double)wy + 100000.0 + (double)m_offset1;
    float num3 = 0.0f;
    // trying to convert 0.002 to double bytes is apparently imperfect. wtf
    //  double can handle QUITE a number of right hand decimals, ESPECIALLY when a number is lt 0.

    //                                                                     0.0019999999
    //                                                                     0.0020000000000000005
    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise((float)(wx * 0.0020000000949949026 * 0.5), (float)(wy * 0.0020000000949949026 * 0.5))
            * (double)VUtils::Math::PerlinNoise((float)(wx * 0.003000000026077032 * 0.5), (float)(wy * 0.003000000026077032 * 0.5) * 1.0));
    




    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise((float)(wx * 0.0020000000949949026 * 1.0), (float)(wy * 0.0020000000949949026 * 1.0))
            * (double)VUtils::Math::PerlinNoise((float)(wx * 0.003000000026077032 * 1.0), (float)(wy * 0.003000000026077032 * 1.0)) * (double)num3 * 0.8999999761581421);





    num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise((float)(wx * 0.004999999888241291 * 1.0), (float)(wy * 0.004999999888241291 * 1.0))
            * (double)VUtils::Math::PerlinNoise((float)(wx * 0.009999999776482582 * 1.0), (float)(wy * 0.009999999776482582 * 1.0)) * 0.5 * (double)num3);



    num3 = (float)((double)num3 - 0.07000000029802322);
    double num4 = (double)VUtils::Math::PerlinNoise((float)(wx * 0.0020000000949949026 * 0.25 + 0.12300000339746475), (float)(wy * 0.0020000000949949026 * 0.25 + 0.15123000741004944));
    float num5 = VUtils::Math::PerlinNoise((float)(wx * 0.0020000000949949026 * 0.25 + 0.32100000977516174), (float)(wy * 0.0020000000949949026 * 0.25 + 0.23100000619888306));
    float v    = std::abs((float)(num4 - (double)num5));
    float num6 = (float)(1.0 - (double)VUtils::Math::LerpStep(0.02f, 0.12f, v));
    num6 = (float)((double)num6 * (double)VUtils::Math::SmoothStep(744.0f, 1000.0f, num2));
    num3 = (float)((double)num3 * (1.0 - (double)num6));
    if (num2 > 10000.0f) {
        float t    = VUtils::Math::LerpStep(10000.0f, waterEdge, num2);
        num3       = VUtils::Math::Lerp(num3, -0.2f, t);
        float num7 = 10490.0f;
        if (num2 > num7) {
            float t2 = VUtils::Math::LerpStep(num7, waterEdge, num2);
            num3     = VUtils::Math::Lerp(num3, -2.0f, t2);
        }
    }
    else if (num2 < m_minMountainDistance && num3 > 0.28f) {
        float t3 = (float)VUtils::Math::Clamp01(((double)num3 - 0.2800000011920929) / 0.09999999403953552);

        num3 = VUtils::Math::Lerp(
                VUtils::Math::Lerp(0.28f, 0.38f, t3), num3,
                VUtils::Math::LerpStep((float)((double)m_minMountainDistance - 400.0), m_minMountainDistance, num2));
    }
    return num3;
}

// this doesnt actually add rivers to the world
// it might add two river points/weights together
float IGeoManager::AddRivers(float wx, float wy, float h)
{
    float num;
    float v;
    GetRiverWeight(wx, wy, num, v);
    if (num <= 0) {
        return h;
    }

    float t    = VUtils::Math::LerpStep(20.0f, 60.0f, v);
    float num2 = VUtils::Math::Lerp(0.14f, 0.12f, t);
    float num3 = VUtils::Math::Lerp(0.139f, 0.128f, t);
    if (h > num2) {
        h = VUtils::Math::Lerp(h, num2, num);
    }
    if (h > num3) {
        float t2 = VUtils::Math::LerpStep(0.85f, 1.0f, num);
        h        = VUtils::Math::Lerp(h, num3, t2);
    }
    return h;
}

float IGeoManager::GetMarshHeight(float wx, float wy)
{
    float wx2 = wx;
    float wy2 = wy;
    float num = 0.137f;
    wx += 100000.f;
    wy += 100000.f;
    float num2 = VUtils::Math::PerlinNoise(wx * 0.04f, wy * 0.04f)
                 * VUtils::Math::PerlinNoise(wx * 0.08f, wy * 0.08f);
    num += num2 * 0.03f;
    num = AddRivers(wx2, wy2, num);
    num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
    return num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
}

float IGeoManager::GetMeadowsHeight(float wx, float wy)
{
    float wx2        = wx;
    float wy2        = wy;
    float baseHeight = GetBaseHeight(wx, wy);
    wx += 100000.f + m_offset3;
    wy += 100000.f + m_offset3;
    float num = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f)
                * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
    num += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f)
           * num * 0.5f;
    float num2 = baseHeight;
    num2 += num * 0.1f;
    float num3 = 0.15f;
    float num4 = num2 - num3;
    float num5 = VUtils::Mathf::Clamp01(baseHeight / 0.4f);
    if (num4 > 0.f)
        num2 -= num4 * (1.f - num5) * 0.75f;

    num2 = AddRivers(wx2, wy2, num2);
    num2 += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
    return num2 + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
}

float IGeoManager::GetForestHeight(float wx, float wy)
{
    float wx2 = wx;
    float wy2 = wy;
    float num = GetBaseHeight(wx, wy);
    wx += 100000.f + m_offset3;
    wy += 100000.f + m_offset3;
    float num2 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f)
                 * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
    num2 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f)
            * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num2 * 0.5f;
    num += num2 * 0.1f;
    num = AddRivers(wx2, wy2, num);
    num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
    return num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
}

float IGeoManager::GetMistlandsHeight(float wx, float wy, float &mask)
{
    float wx2 = wx;
    float wy2 = wy;
    float num = GetBaseHeight(wx, wy);
    wx += 100000.f + m_offset3;
    wy += 100000.f + m_offset3;
    float num2 = VUtils::Math::PerlinNoise(wx * 0.02f * 0.7f, wy * 0.02f * 0.7f)
                 * VUtils::Math::PerlinNoise(wx * 0.04f * 0.7f, wy * 0.04f * 0.7f);
    num2 += VUtils::Math::PerlinNoise(wx * 0.03f * 0.7f, wy * 0.03f * 0.7f)
            * VUtils::Math::PerlinNoise(wx * 0.05f * 0.7f, wy * 0.05f * 0.7f) * num2 * 0.5f;
    num2 = (num2 > 0) ? std::pow(num2, 1.5f) : num2;
    num += num2 * 0.4f;
    num        = AddRivers(wx2, wy2, num);
    float num3 = VUtils::Mathf::Clamp01(num2 * 7.f);
    num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.03f * num3;
    num += VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.01f * num3;
    float num4 = 1.f - num3 * 1.2f;
    num4 -= 1.f - VUtils::Math::LerpStep(0.1f, 0.3f, num3);
    float a    = num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.002f;
    float num5 = num;
    num5 *= 400.f;
    num5 = std::ceil(num5);
    num5 /= 400.f;
    //num = VUtils::Mathf::Lerp(a, num5, num3);
    num = std::lerp(a, num5, num3);
    //mask = avledet::util::Color{ 0, 0, 0, num4 };
    mask = num4;
    return num;
}

float IGeoManager::GetPlainsHeight(float wx, float wy)
{
    float wx2        = wx;
    float wy2        = wy;
    float baseHeight = GetBaseHeight(wx, wy);
    wx += 100000.f + m_offset3;
    wy += 100000.f + m_offset3;
    float num = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f)
                * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
    num += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f)
           * num * 0.5f;
    float num2 = baseHeight;
    num2 += num * 0.1f;
    float num3 = 0.15f;
    float num4 = num2 - num3;
    float num5 = VUtils::Mathf::Clamp01(baseHeight / 0.4f);
    if (num4 > 0.f)
        num2 -= num4 * (1.f - num5) * 0.75f;

    num2 = AddRivers(wx2, wy2, num2);
    num2 += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
    return num2 + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
}

float IGeoManager::GetAshlandsHeight(float wx, float wy)
{
    float wx2 = wx;
    float wy2 = wy;
    float num = GetBaseHeight(wx, wy);
    wx += 100000.f + m_offset3;
    wy += 100000.f + m_offset3;
    float num2 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f)
                 * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
    num2 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f)
            * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num2 * 0.5f;
    num += num2 * 0.1f;
    num += 0.1f;
    num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
    num += VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
    return AddRivers(wx2, wy2, num);
}

float IGeoManager::GetEdgeHeight(float wx, float wy)
{
    float magnitude = VUtils::Math::magnitude(wx, wy);
    float num       = 10490;
    if (magnitude > num) {
        float num2 = VUtils::Math::LerpStep(num, 10500, magnitude);
        return -2.f * num2;
    }
    float t    = VUtils::Math::LerpStep(10000, 10100, magnitude);
    float num3 = GetBaseHeight(wx, wy);
    num3       = VUtils::Mathf::Lerp(num3, 0, t);
    return AddRivers(wx, wy, num3);
}

float IGeoManager::GetOceanHeight(float wx, float wy)
{
    return GetBaseHeight(wx, wy);
}

float IGeoManager::BaseHeightTilt(float wx, float wy)
{
    float baseHeight  = GetBaseHeight(wx - 1.f, wy);
    float baseHeight2 = GetBaseHeight(wx + 1.f, wy);
    float baseHeight3 = GetBaseHeight(wx, wy - 1.f);
    float baseHeight4 = GetBaseHeight(wx, wy + 1.f);
    return abs(baseHeight2 - baseHeight) + abs(baseHeight3 - baseHeight4);
}

float IGeoManager::GetSnowMountainHeight(float wx, float wy)
{
    float wx2  = wx;
    float wy2  = wy;
    float num  = GetBaseHeight(wx, wy);
    float num2 = BaseHeightTilt(wx, wy);
    wx += 100000.f + m_offset3;
    wy += 100000.f + m_offset3;
    float num3 = num - 0.4f;
    num += num3;
    float num4 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f)
                 * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
    num4 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f)
            * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num4 * 0.5f;
    num += num4 * 0.2f;
    num = AddRivers(wx2, wy2, num);
    num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
    num += VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
    return num + VUtils::Math::PerlinNoise(wx * 0.2f, wy * 0.2f) * 2.f * num2;
}

float IGeoManager::GetDeepNorthHeight(float wx, float wy)
{
    float wx2 = wx;
    float wy2 = wy;
    float num = GetBaseHeight(wx, wy);
    wx += 100000.f + m_offset3;
    wy += 100000.f + m_offset3;
    float num2 = std::max(0.f, num - 0.4f);
    num += num2;
    float num3 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f)
                 * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
    num3 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f)
            * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num3 * 0.5f;
    num += num3 * 0.2f;
    num *= 1.2f;
    num = AddRivers(wx2, wy2, num);
    num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
    return num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
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

//
// public accessed methods:
//


bool IGeoManager::InsideRiverGrid(Vector2i grid, Vector2f p, float r)
{
    Vector2f b((float) grid.x * riverGridSize, (float) grid.y * riverGridSize);
    Vector2f vector = p - b;
    return std::abs(vector.x) < r + (riverGridSize * .5f) && std::abs(vector.y) < r + (riverGridSize * .5f);
}

Vector2i IGeoManager::GetRiverGrid(float wx, float wy)
{
    auto x = (std::int32_t) std::floor((wx + riverGridSize * .5f) / riverGridSize);
    auto y = (std::int32_t) std::floor((wy + riverGridSize * .5f) / riverGridSize);
    return Vector2i(x, y);
}

avledet::util::BiomeArea IGeoManager::GetBiomeArea(Vector3f point)
{
    auto &&biome = GetBiome(point);

    auto &&biome2
            = GetBiome(point - Vector3f(-IZoneManager::UNITS_PER_ZONE, 0, -IZoneManager::UNITS_PER_ZONE));
    auto &&biome3
            = GetBiome(point - Vector3f(IZoneManager::UNITS_PER_ZONE, 0, -IZoneManager::UNITS_PER_ZONE));
    auto &&biome4 = GetBiome(point - Vector3f(IZoneManager::UNITS_PER_ZONE, 0, IZoneManager::UNITS_PER_ZONE));
    auto &&biome5
            = GetBiome(point - Vector3f(-IZoneManager::UNITS_PER_ZONE, 0, IZoneManager::UNITS_PER_ZONE));
    auto &&biome6 = GetBiome(point - Vector3f(-IZoneManager::UNITS_PER_ZONE, 0, 0));
    auto &&biome7 = GetBiome(point - Vector3f(IZoneManager::UNITS_PER_ZONE, 0, 0));
    auto &&biome8 = GetBiome(point - Vector3f(0, 0, -IZoneManager::UNITS_PER_ZONE));
    auto &&biome9 = GetBiome(point - Vector3f(0, 0, IZoneManager::UNITS_PER_ZONE));
    if (biome == biome2 && biome == biome3 && biome == biome4 && biome == biome5 && biome == biome6
        && biome == biome7 && biome == biome8 && biome == biome9) {
        return avledet::util::BiomeArea::Median;
    }
    return avledet::util::BiomeArea::Edge;
}

// public
avledet::util::Biome IGeoManager::GetBiome(Vector3f point)
{
    return GetBiome(point.x, point.z);
}

// public
avledet::util::Biome IGeoManager::GetBiome(float wx, float wy)
{
    auto magnitude  = VUtils::Math::magnitude(wx, wy);
    auto baseHeight = GetBaseHeight(wx, wy);
    float num       = WorldAngle(wx, wy) * 100.f;

    // bottom curve of world are ashlands
    if (VUtils::Math::magnitude(wx, wy + ashlandsYOffset) > ashlandsMinDistance + num)
        return avledet::util::Biome::AshLands;

    if (baseHeight <= 0.02f)
        return avledet::util::Biome::Ocean;

    // top curve of world is deep north
    if (VUtils::Math::magnitude(wx, wy + deepNorthYOffset) > deepNorthMinDistance + num) {
        if (baseHeight > mountainBaseHeightMin)
            return avledet::util::Biome::Mountain;
        return avledet::util::Biome::DeepNorth;
    }

    if (baseHeight > mountainBaseHeightMin)
        return avledet::util::Biome::Mountain;

    if (VUtils::Math::PerlinNoise((m_offset0 + wx) * marshBiomeScale, (m_offset0 + wy) * marshBiomeScale)
                > minMarshNoise
        && magnitude > minMarshDistance && magnitude < maxMarshDistance && baseHeight > minMarshHeight
        && baseHeight < maxMarshHeight)
        return avledet::util::Biome::Swamp;

    if (VUtils::Math::PerlinNoise((m_offset4 + wx) * darklandBiomeScale,
                                  (m_offset4 + wy) * darklandBiomeScale)
                > minDarklandNoise
        && magnitude > minDarklandDistance + num && magnitude < maxDarklandDistance)
        return avledet::util::Biome::Mistlands;

    if (VUtils::Math::PerlinNoise((m_offset1 + wx) * heathBiomeScale, (m_offset1 + wy) * heathBiomeScale)
                > minHeathNoise
        && magnitude > minHeathDistance + num && magnitude < maxHeathDistance)
        return avledet::util::Biome::Plains;

    if (VUtils::Math::PerlinNoise((m_offset2 + wx) * 0.001f, (m_offset2 + wy) * 0.001f) > minDeepForestNoise
        && magnitude > minDeepForestDistance + num && magnitude < maxDeepForestDistance)
        return avledet::util::Biome::BlackForest;

    if (magnitude > meadowsMaxDistance + num)
        return avledet::util::Biome::BlackForest;

    return avledet::util::Biome::Meadows;
}

avledet::util::Biome IGeoManager::GetBiomes(float x, float z)
{
    //ZoneID zone = IZoneManager::WorldToZonePos(Vector3f(x, 0., z));
    //Vector3f center = IZoneManager::ZoneToWorldPos(zone) + ;
    return avledet::util::Biome(std::to_underlying(GetBiome(x - IZoneManager::UNITS_PER_ZONE / 2,
                                                            z - IZoneManager::UNITS_PER_ZONE / 2))
                                || std::to_underlying(GetBiome(x - IZoneManager::UNITS_PER_ZONE / 2,
                                                               z + IZoneManager::UNITS_PER_ZONE / 2))
                                || std::to_underlying(GetBiome(x + IZoneManager::UNITS_PER_ZONE / 2,
                                                               z - IZoneManager::UNITS_PER_ZONE / 2))
                                || std::to_underlying(GetBiome(x + IZoneManager::UNITS_PER_ZONE / 2,
                                                               z + IZoneManager::UNITS_PER_ZONE / 2))
                                || std::to_underlying(GetBiome(x, z)));
}

float IGeoManager::GetHeight(float wx, float wy)
{
    // wtf was i thinking
    //float dummy;
    avledet::util::Color dummy;
    return GetHeight(wx, wy, dummy);
}

float IGeoManager::GetHeight(float wx, float wy, avledet::util::Color &mask)
{
    auto biome = GetBiome(wx, wy);
    return GetBiomeHeight(biome, wx, wy, mask, false);
}

// Used only early during generation
float IGeoManager::GetGenerationHeight(float wx, float wy)
{
    auto biome = GetBiome(wx, wy);
    //if (biome == avledet::util::Biome::Mistlands)
        //return GetForestHeight(wx, wy) * 200.f;
    avledet::util::Color dummy;
    return GetBiomeHeight(biome, wx, wy, dummy, false);
}

// public
float IGeoManager::GetBiomeHeight(avledet::util::Biome biome, float wx, float wy, avledet::util::Color &mask, bool preGeneration)
{
		float num;
		if (preGeneration)
		{
			//num = WorldGenerator.GetHeightMultiplier();
            num = 200.0f;
		}
		else
		{
			num = (float)(200.0 * this.CreateAshlandsGap(wx, wy) * this.CreateDeepNorthGap(wx, wy));
		}
		mask = Color.black;
		if (this.m_world.m_menu)
		{
			if (biome == Heightmap.Biome.Mountain)
			{
				return (float)((double)this.GetSnowMountainHeight(wx, wy, true) * (double)num);
			}
			return (float)((double)this.GetMenuHeight(wx, wy) * (double)num);
		}
		else
		{
			if (DUtils.Length(wx, wy) > 10500f)
			{
				return -2f * WorldGenerator.GetHeightMultiplier();
			}
			if (biome <= Heightmap.Biome.Plains)
			{
				switch (biome)
				{
				case Heightmap.Biome.Meadows:
					return (float)((double)this.GetMeadowsHeight(wx, wy) * (double)num);
				case Heightmap.Biome.Swamp:
					return (float)((double)this.GetMarshHeight(wx, wy) * (double)num);
				case Heightmap.Biome.Meadows | Heightmap.Biome.Swamp:
					break;
				case Heightmap.Biome.Mountain:
					return (float)((double)this.GetSnowMountainHeight(wx, wy, false) * (double)num);
				default:
					if (biome == Heightmap.Biome.BlackForest)
					{
						return (float)((double)this.GetForestHeight(wx, wy) * (double)num);
					}
					if (biome == Heightmap.Biome.Plains)
					{
						return (float)((double)this.GetPlainsHeight(wx, wy) * (double)num);
					}
					break;
				}
			}
			else if (biome <= Heightmap.Biome.DeepNorth)
			{
				if (biome != Heightmap.Biome.AshLands)
				{
					if (biome == Heightmap.Biome.DeepNorth)
					{
						return (float)((double)this.GetDeepNorthHeight(wx, wy) * (double)num);
					}
				}
				else
				{
					if (preGeneration)
					{
						return (float)((double)this.GetAshlandsHeightPregenerate(wx, wy) * (double)num);
					}
					return (float)((double)this.GetAshlandsHeight(wx, wy, out mask, false) * (double)num);
				}
			}
			else
			{
				if (biome == Heightmap.Biome.Ocean)
				{
					return (float)((double)this.GetOceanHeight(wx, wy) * (double)num);
				}
				if (biome == Heightmap.Biome.Mistlands)
				{
					if (preGeneration)
					{
						return (float)((double)this.GetForestHeight(wx, wy) * (double)num);
					}
					return (float)((double)this.GetMistlandsHeight(wx, wy, out mask) * (double)num);
				}
			}
			return 0f;
		}

    switch (biome) {
    case avledet::util::Biome::Meadows: return GetMeadowsHeight(wx, wy) * 200.f;
    case avledet::util::Biome::Swamp: return GetMarshHeight(wx, wy) * 200.f;
    case avledet::util::Biome::Mountain: return GetSnowMountainHeight(wx, wy) * 200.f;
    case avledet::util::Biome::BlackForest: return GetForestHeight(wx, wy) * 200.f;
    case avledet::util::Biome::Plains: return GetPlainsHeight(wx, wy) * 200.f;
    case avledet::util::Biome::AshLands: return GetAshlandsHeight(wx, wy) * 200.f;
    case avledet::util::Biome::DeepNorth: return GetDeepNorthHeight(wx, wy) * 200.f;
    case avledet::util::Biome::Ocean: return GetOceanHeight(wx, wy) * 200.f;
    case avledet::util::Biome::Mistlands: return GetMistlandsHeight(wx, wy, mask) * 200.f;
    default: break;
    }
    return 0;
}

// public
bool IGeoManager::InForest(Vector3f pos)
{
    return GetForestFactor(pos) < 1.15f;
}

// public
float IGeoManager::GetForestFactor(Vector3f pos)
{
    float d = 0.4f;
    return VUtils::Math::Fbm(pos * 0.01f * d, 3, 1.6f, 0.7f);
}

// public
void IGeoManager::GetTerrainDelta(VUtils::Random::State &state, Vector3f center, float radius, float &delta,
                                  Vector3f &slopeDirection)
{
    int num    = 10;
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();
    Vector3f b = center;
    Vector3f a = center;
    for (int i = 0; i < num; i++) {
        Vector2f vector  = state.inside_unit_circle() * radius;
        Vector3f vector2 = center + Vector3f(vector.x, 0.f, vector.y);
        float height     = GetHeight(vector2.x, vector2.z);
        if (height < num3) {
            num3 = height;
            a    = vector2;
        }
        if (height > num2) {
            num2 = height;
            b    = vector2;
        }
    }
    delta          = num2 - num3;
    slopeDirection = (a - b).normal();
}

// public
int IGeoManager::GetSeed()
{
    return m_world->m_seed;
}
#endif// AVL_GENERATE_ZONES