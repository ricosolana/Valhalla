#pragma once

// reverse engineered implementation of Unity Random and associated functions
// these are algorithms only, not steps, so shoo patent lawyers!

#include <random>

#include "VUtils.h"
#include "Vector.h"

namespace VUtils::Random {

    class State {
    private:
        uint32_t m_seed[4];


    public:
        State();
        State(int32_t seed);
        State(const State& other); // copy construct

        // Returns a random float from 0 to 1
        float NextFloat();
        uint32_t NextInt();
        float Value() { return NextFloat(); }

        float Range(float minInclude, float maxExclude);
        int32_t Range(int32_t minInclude, int32_t maxExclude);

        Vector2f InsideUnitCircle();
        Vector3f OnUnitSphere();
        Vector3f InsideUnitSphere();
    };

    OWNER_t GenerateUID();

    void GenerateAlphaNum(char* out, size_t outSize);

    std::string GenerateAlphaNum(size_t count);
}

