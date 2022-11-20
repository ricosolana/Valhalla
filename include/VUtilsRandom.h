#pragma once

// reverse engineered implementation of Unity Random and associated functions
// these are algorithms only, not steps, so shoo patent lawyers!

#include "VUtils.h"
#include "Vector.h"

namespace VUtils::Random {

	class State {
	private:
		int32_t m_seed[4];

	private:
		// Returns a random float from 0 to 1
		float Next();

		void Shuffle();

	public:
		State(int32_t seed);
		State(const State& other); // copy construct

		float Range(float minInclude, float maxExclude);
		int32_t Range(int32_t minInclude, uint32_t maxExclude);

		Vector2 GetRandomUnitCircle();

		Vector3 OnUnitSphere();
		Vector3 InsideUnitSphere();
	};

}

