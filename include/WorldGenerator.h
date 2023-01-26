#pragma once

#include "VUtils.h"
#include "VUtilsRandom.h"
#include "WorldManager.h"
#include "Vector.h"
#include "HeightMap.h"

namespace WorldGenerator {

	void Init();

	bool InsideRiverGrid(const Vector2i& grid, const Vector2& p, float r);

	Vector2i GetRiverGrid(float wx, float wy);

	BiomeArea GetBiomeArea(const Vector3& point);
	Biome GetBiome(const Vector3& point);
	Biome GetBiome(float wx, float wy);

	float GetHeight(float wx, float wy);
	float GetHeight(float wx, float wy, Color& color);
	float GetBiomeHeight(Biome biome, float wx, float wy, Color& color);

	bool InForest(const Vector3& pos);

	float GetForestFactor(const Vector3& pos);

	void GetTerrainDelta(VUtils::Random::State& state, const Vector3& center, float radius, float& delta, Vector3& slopeDirection);

	int GetSeed();

	static constexpr int32_t worldSize = 10000;

	static constexpr float waterEdge = 10500;
}