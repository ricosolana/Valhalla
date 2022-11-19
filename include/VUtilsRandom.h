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

	float PerlinNoise(float x, float y);

}

