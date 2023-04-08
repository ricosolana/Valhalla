#pragma once

#include "VUtils.h"
#include "VUtilsRandom.h"
#include "WorldManager.h"
#include "Vector.h"
#include "HeightMap.h"

class IGeoManager {
private:
	struct River {
		Vector2f p0;
		Vector2f p1;
		Vector2f center;
		float widthMin;
		float widthMax;
		float curveWidth;
		float curveWavelength;
	};

	struct RiverPoint {
		Vector2f p;
		float w;
		float w2;

		RiverPoint(Vector2f p_p, float p_w) {
			p = p_p;
			w = p_w;
			w2 = p_w * p_w;
		}
	};

	static constexpr float m_waterTreshold = 0.05f;

	World* m_world;

	//int m_version;
	float m_offset0;
	float m_offset1;
	float m_offset2;
	float m_offset3;
	float m_offset4;
	int32_t m_riverSeed;
	int32_t m_streamSeed;
	std::vector<Vector2f> m_lakes;
	std::vector<River> m_rivers;
	std::vector<River> m_streams;
	UNORDERED_MAP_t<Vector2i, std::vector<RiverPoint>> m_riverPoints;
	//std::vector<RiverPoint> m_cachedRiverPoints; //RiverPoint[] m_cachedRiverPoints;
	std::vector<RiverPoint>* m_cachedRiverPoints;
	Vector2i m_cachedRiverGrid = { -999999, -999999 };
	//ReaderWriterLockSlim m_riverCacheLock; // for terrian builder?
	//std::vector<Heightmap::Biome> m_biomes; // seems unused

	static constexpr float	riverGridSize = 64;
	static constexpr float	minRiverWidth = 60;
	static constexpr float	maxRiverWidth = 100;
	static constexpr float	minRiverCurveWidth = 50;
	static constexpr float	maxRiverCurveWidth = 80;
	static constexpr float	minRiverCurveWaveLength = 50;
	static constexpr float	maxRiverCurveWaveLength = 70;
	static constexpr int	streams = 3000;
	static constexpr float	streamWidth = 20;
	static constexpr float	meadowsMaxDistance = 5000;
	static constexpr float	minDeepForestNoise = 0.4f;
	static constexpr float	minDeepForestDistance = 600;
	static constexpr float	maxDeepForestDistance = 6000;
	static constexpr float	deepForestForestFactorMax = 0.9f;
	// Marsh is swamp
	static constexpr float	marshBiomeScale = 0.001f;
	static constexpr float	minMarshNoise = 0.6f;
	static constexpr float	minMarshDistance = 2000;
	/*static constexpr*/ float	maxMarshDistance = 6000;
	static constexpr float	minMarshHeight = 0.05f;
	static constexpr float	maxMarshHeight = 0.25f;
	// Heath is plains
	//	PROOF: heathColor in MiniMap is used for plains biome
	static constexpr float	heathBiomeScale = 0.001f;
	static constexpr float	minHeathNoise = 0.4f;
	static constexpr float	minHeathDistance = 3000;
	static constexpr float	maxHeathDistance = 8000;
	// Darklands is mistlands
	//	PROOF: not definite, but these values are nearby in usage
	static constexpr float	darklandBiomeScale = 0.001f;
	/*static constexpr*/ float	minDarklandNoise = 0.4f;
	static constexpr float	minDarklandDistance = 6000;
	static constexpr float	maxDarklandDistance = 10000;
	// ocean
	static constexpr float	oceanBiomeScale = 0.0005f;
	static constexpr float	oceanBiomeMinNoise = 0.4f;
	static constexpr float	oceanBiomeMaxNoise = 0.6f;
	static constexpr float	oceanBiomeMinDistance = 1000;
	static constexpr float	oceanBiomeMinDistanceBuffer = 256;
	/*static constexpr*/ float	m_minMountainDistance = 1000; // usually mutable because version changes this
	static constexpr float	mountainBaseHeightMin = 0.4f;
	static constexpr float	deepNorthMinDistance = 12000;
	static constexpr float	deepNorthYOffset = 4000;
	static constexpr float	ashlandsMinDistance = 12000;
	static constexpr float	ashlandsYOffset = -4000;


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
	std::vector<Vector2f> MergePoints(std::vector<Vector2f>& points, float range);
	int FindClosest(const std::vector<Vector2f>& points, const Vector2f& p, float maxDistance);
	void GenerateStreams();
	bool FindStreamEndPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, const Vector2f& start, float minLength, float maxLength, Vector2f& end);
	bool FindStreamStartPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, Vector2f& p, float& starth);
	void GenerateRivers();
	int FindRandomRiverEnd(VUtils::Random::State& state, const std::vector<River>& rivers, const std::vector<Vector2f>& points, 
		const Vector2f& p, float maxDistance, float heightLimit, float checkStep) const;
	bool HaveRiver(const std::vector<River>& rivers, const Vector2f& p0) const;
	bool HaveRiver(const std::vector<River>& rivers, const Vector2f& p0, const Vector2f& p1) const;
	bool IsRiverAllowed(const Vector2f& p0, const Vector2f& p1, float step, float heightLimit) const;
	void RenderRivers(VUtils::Random::State& state, const std::vector<River>& rivers);
	void AddRiverPoint(UNORDERED_MAP_t<Vector2i, std::vector<RiverPoint>>& riverPoints,
		const Vector2f& p,
		float r);
	void AddRiverPoint(UNORDERED_MAP_t<Vector2i, std::vector<RiverPoint>>& riverPoints, const Vector2i& grid, const Vector2f& p, float r);
	//bool InsideRiverGrid(const Vector2i& grid, const Vector2f& p, float r);

	//Vector2i GetRiverGrid(float wx, float wy);
	void GetRiverWeight(float wx, float wy, float& outWeight, float& outWidth);
	void GetWeight(const std::vector<RiverPoint>& points, float wx, float wy, float& weight, float& width);
	float WorldAngle(float wx, float wy);
	float GetBaseHeight(float wx, float wy) const;
	float AddRivers(float wx, float wy, float h);

	float GetGenerationHeight(float x, float y);

	//float GetHeight(float wx, float wy, );
	//float GetBiomeHeight(Heightmap::Biome biome, float wx, float wy);
	float GetMarshHeight(float wx, float wy);
	float GetMeadowsHeight(float wx, float wy);
	float GetForestHeight(float wx, float wy);
	float GetMistlandsHeight(float wx, float wy, float& mask);
	float GetPlainsHeight(float wx, float wy);
	float GetAshlandsHeight(float wx, float wy);
	float GetEdgeHeight(float wx, float wy);
	float GetOceanHeight(float wx, float wy);
	float BaseHeightTilt(float wx, float wy);
	float GetSnowMountainHeight(float wx, float wy);
	float GetDeepNorthHeight(float wx, float wy);

public:
	void Init();

	bool InsideRiverGrid(const Vector2i& grid, const Vector2f& p, float r);

	Vector2i GetRiverGrid(float wx, float wy);

	BiomeArea GetBiomeArea(const Vector3f& point);

	// Get the biome at world coordinates
	Biome GetBiome(const Vector3f& point);

	// Get the biome at world coordinates
	Biome GetBiome(float x, float z);

	// Get all the biomes within a radius
	//Biome GetBiomes(float x, float y, float radius)

	// Get all the corner biomes within this zone
	Biome GetBiomes(float x, float y);

	// Get the terrain height at world coordinates
	float GetHeight(float x, float z);

	// Get the terrain height at world coordinates, with mistlands color mask
	float GetHeight(float x, float z, float& mask);
	float GetBiomeHeight(Biome biome, float wx, float wy, float& mask);

	bool InForest(const Vector3f& pos);

	float GetForestFactor(const Vector3f& pos);

	void GetTerrainDelta(VUtils::Random::State& state, const Vector3f& center, float radius, float& delta, Vector3f& slopeDirection);

	int GetSeed();

	static constexpr int32_t worldSize = 10000;

	static constexpr float waterEdge = 10500;
};

// Manager class for everything related to coarse world heights and biomes during initial generation
IGeoManager* GeoManager();
