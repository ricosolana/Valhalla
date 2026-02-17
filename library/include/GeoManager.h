#pragma once

#include "VUtils.h"
#include <quill/Logger.h>

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    #include "HeightMap.h"
    #include "Vector.h"
    #include "VUtilsRandom.h"
    #include "WorldManager.h"

class IGeoManager
{
  private:
    struct River
    {
        Vector2f p0;
        Vector2f p1;
        Vector2f center;
        float widthMin;
        float widthMax;
        float curveWidth;
        float curveWavelength;
    };

    struct RiverPoint
    {
        Vector2f p;
        float w;
        float w2;

        RiverPoint(Vector2f p_p, float p_w)
        {
            p  = p_p;
            w  = p_w;
            w2 = p_w * p_w;
        }
    };

    static constexpr float m_waterTreshold = 0.05f;

    World *m_world;

    //int m_version;
    float m_offset0;
    float m_offset1;
    float m_offset2;
    float m_offset3;
    float m_offset4;
    std::int32_t m_riverSeed;
    std::int32_t m_streamSeed;
    std::vector<Vector2f> m_lakes;
    std::vector<River> m_rivers;
    std::vector<River> m_streams;

    std::mutex m_mutRiverCache;
    avledet::util::Map<Vector2i, std::vector<RiverPoint>> m_riverPoints;
    //std::vector<RiverPoint> m_cachedRiverPoints; //RiverPoint[] m_cachedRiverPoints;
    std::vector<RiverPoint> *m_cachedRiverPoints;
    Vector2i m_cachedRiverGrid = {-999999, -999999};
    //ReaderWriterLockSlim m_riverCacheLock; // for terrian builder?
    //std::vector<Heightmap::Biome> m_biomes; // seems unused

    static constexpr float heightMultiplier         = 200.0f;
    static constexpr float riverGridSize             = 64.0f;
    static constexpr float minRiverWidth             = 60.0f;
    static constexpr float maxRiverWidth             = 100;
    static constexpr float minRiverCurveWidth        = 50;
    static constexpr float maxRiverCurveWidth        = 80;
    static constexpr float minRiverCurveWaveLength   = 50;
    static constexpr float maxRiverCurveWaveLength   = 70;
    static constexpr int streams                     = 3000;
    static constexpr float streamWidth               = 20;
    static constexpr float meadowsMaxDistance        = 5000;
    static constexpr float minDeepForestNoise        = 0.4f;
    static constexpr float minDeepForestDistance     = 600;
    static constexpr float maxDeepForestDistance     = 6000;
    static constexpr float deepForestForestFactorMax = 0.9f;
    // Marsh is swamp
    static constexpr float marshBiomeScale      = 0.001f;
    static constexpr float minMarshNoise        = 0.6f;
    static constexpr float minMarshDistance     = 2000;
    /*static constexpr*/ float maxMarshDistance = 6000;
    static constexpr float minMarshHeight       = 0.05f;
    static constexpr float maxMarshHeight       = 0.25f;
    // Heath is plains
    //	PROOF: heathColor in MiniMap is used for plains biome
    static constexpr float heathBiomeScale  = 0.001f;
    static constexpr float minHeathNoise    = 0.4f;
    static constexpr float minHeathDistance = 3000;
    static constexpr float maxHeathDistance = 8000;
    // Darklands is mistlands
    //	PROOF: not definite, but these values are nearby in usage
    static constexpr float darklandBiomeScale   = 0.001f;
    /*static constexpr*/ float minDarklandNoise = 0.4f;
    static constexpr float minDarklandDistance  = 6000;
    static constexpr float maxDarklandDistance  = 10000;
    // ocean
    static constexpr float oceanBiomeScale             = 0.0005f;
    static constexpr float oceanBiomeMinNoise          = 0.4f;
    static constexpr float oceanBiomeMaxNoise          = 0.6f;
    static constexpr float oceanBiomeMinDistance       = 1000;
    static constexpr float oceanBiomeMinDistanceBuffer = 256;
    /*static constexpr*/ float m_minMountainDistance   = 1000;// usually mutable because version changes this
    static constexpr float mountainBaseHeightMin       = 0.4f;
    static constexpr float deepNorthMinDistance        = 12000;
    static constexpr float deepNorthYOffset            = 4000;
    static constexpr float ashlandsMinDistance         = 12000;
    static constexpr float ashlandsYOffset             = -4000;


    /*
	* See https://docs.unity3d.com/ScriptReference/Random.Range.html
	*	for random algorithm implementation discussion
	*	A c++ implementation of Unity.Random.Range needs to be created
	*		to perfectly recreate Valheim worldgen
	*/


    // Forward declarations
    void Generate();

    //void GenerateMountains();
    void GenerateLakes();
    std::vector<Vector2f> MergePoints(std::vector<Vector2f> &points, float range);
    int FindClosest(std::vector<Vector2f> const &points, Vector2f p, float maxDistance);
    void GenerateStreams();
    bool FindStreamEndPoint(VUtils::Random::State &state, int iterations, float minHeight, float maxHeight,
                            Vector2f start, float minLength, float maxLength, Vector2f &end);
    bool FindStreamStartPoint(VUtils::Random::State &state, int iterations, float minHeight, float maxHeight,
                              Vector2f &p, float &starth);
    void GenerateRivers();
    int FindRandomRiverEnd(VUtils::Random::State &state, std::vector<River> const &rivers,
                           std::vector<Vector2f> const &points, Vector2f p, float maxDistance,
                           float heightLimit, float checkStep) const;
    bool HaveRiver(std::vector<River> const &rivers, Vector2f p0) const;
    bool HaveRiver(std::vector<River> const &rivers, Vector2f p0, Vector2f p1) const;
    bool IsRiverAllowed(Vector2f p0, Vector2f p1, float step, float heightLimit) const;
    void RenderRivers(VUtils::Random::State &state, std::vector<River> const &rivers);
    void AddRiverPoint(avledet::util::Map<Vector2i, std::vector<RiverPoint>> &riverPoints, Vector2f p,
                       float r);
    void AddRiverPoint(avledet::util::Map<Vector2i, std::vector<RiverPoint>> &riverPoints, Vector2i grid,
                       Vector2f p, float r);
    //bool InsideRiverGrid(const Vector2i& grid, const Vector2f& p, float r);

    //Vector2i GetRiverGrid(float wx, float wy);
    void GetRiverWeight(float wx, float wy, float &outWeight, float &outWidth);
    void GetWeight(std::vector<RiverPoint> const &points, float wx, float wy, float &weight, float &width);
    float WorldAngle(float wx, float wy);
    float GetBaseHeight(float wx, float wy) const;
    float AddRivers(float wx, float wy, float h);

    float GetGenerationHeight(float x, float y);

    //float GetHeight(float wx, float wy, );
    //float GetBiomeHeight(Heightmap::Biome biome, float wx, float wy);
    float GetMarshHeight(float wx, float wy);
    float GetMeadowsHeight(float wx, float wy);
    float GetForestHeight(float wx, float wy);
    float GetMistlandsHeight(float wx, float wy, avledet::util::Color &mask);
    float GetPlainsHeight(float wx, float wy);
    float GetAshlandsHeight(float wx, float wy);
    float GetEdgeHeight(float wx, float wy);
    float GetOceanHeight(float wx, float wy);
    float BaseHeightTilt(float wx, float wy);
    float GetSnowMountainHeight(float wx, float wy);
    float GetDeepNorthHeight(float wx, float wy);

    double CreateAshlandsGap(float wx, float wy);
    double CreateDeepNorthGap(float wx, float wy);

  public:
    void PostWorldInit();

    bool InsideRiverGrid(Vector2i grid, Vector2f p, float r);

    Vector2i GetRiverGrid(float wx, float wy);

    avledet::util::BiomeArea GetBiomeArea(Vector3f point);

    // Get the biome at world coordinates
    avledet::util::Biome GetBiome(Vector3f point);

    // Get the biome at world coordinates
    avledet::util::Biome GetBiome(float x, float z);

    // Get all the biomes within a radius
    //avledet::util::Biome GetBiomes(float x, float y, float radius)

    // Get all the corner biomes within this zone
    avledet::util::Biome GetBiomes(float x, float y);

    // Get the terrain height at world coordinates
    float GetHeight(float x, float z);

    // Get the terrain height at world coordinates, with mistlands color mask
    //  preGeneration: bool (used only for river gen)
    float GetHeight(float x, float z, avledet::util::Color &mask);
    float GetBiomeHeight(avledet::util::Biome biome, float wx, float wy, avledet::util::Color &mask, bool preGeneration);

    bool InForest(Vector3f pos);

    float GetForestFactor(Vector3f pos);

    void GetTerrainDelta(VUtils::Random::State &state, Vector3f center, float radius, float &delta,
                         Vector3f &slopeDirection);

    int GetSeed();

    static constexpr std::int32_t worldSize = 10000;

    static constexpr float waterEdge = 10500;
};

// Manager class for everything related to coarse world heights and biomes during initial generation
IGeoManager *GeoManager();

#endif
