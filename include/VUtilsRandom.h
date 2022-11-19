#pragma once

// reverse engineered implementation of Unity Random and associated functions
// these are algorithms only, not steps, so shoo patent lawyers!

#include "VUtils.h"

namespace VUtils::Random {

	struct Seed_t {
		int32_t data[4];
	};



	void SetSeed(int32_t seed);

	Seed_t GetSeed();

	float Range(float minInclude, float maxExclude);
	int32_t Range(int32_t minInclude, uint32_t maxExclude);



	float PerlinNoise(float x, float y);

}
