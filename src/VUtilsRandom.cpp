#include <stdint.h>
#include <limits>
#include "VUtilsRandom.h"

// It seems someone already had a random implementation made
// This doesnt bother me because this part was relatively easy anyway
//  https://gist.github.com/macklinb/a00be6b616cbf20fa95e4227575fe50b
// My thread post
//  https://forum.unity.com/threads/algorithm-implementations-of-random-and-perlinnoise.1348790/

// I primarily used Ghidra along with DnSpy to understand function signatures and what data was passed
//  back and forth
// This helped a fuckton
//  https://reverseengineering.stackexchange.com/questions/29393/is-there-a-way-to-find-the-implementation-of-methods-with-methodimploptions-inte

// I searched for several broad strings within the .rdata portion of the .dll
// Got references to that string, which pointed to a string table containing CSharp method names and native functions
// Find wherever the string tale is referenced (should be within a func that binds names to funcs)
// Get the offset into the string table for that reference, now add the offset into the func table

// 
// PerlinNoise was the harder part
// But I still pulled it off :)


namespace VUtils::Random {

	State::State(int32_t seed) {
		m_seed[0] = seed;
		m_seed[1] = m_seed[0] * 0x6c078965 + 1;
		m_seed[2] = m_seed[1] * 0x6c078965 + 1;
		m_seed[3] = m_seed[2] * 0x6c078965 + 1;
	}

    State::State(const State& other) {
        m_seed[0] = other.m_seed[0];
        m_seed[1] = other.m_seed[1];
        m_seed[2] = other.m_seed[2];
        m_seed[3] = other.m_seed[3];
    }

    uint32_t State::NextInt() {
        uint32_t mut1 = (m_seed[0] << 11) ^ m_seed[0];

        m_seed[0] = m_seed[1];
        m_seed[1] = m_seed[2];
        m_seed[2] = m_seed[3];
        mut1 = (((m_seed[3] >> 11) ^ mut1) >> 8) ^ m_seed[3] ^ mut1;
        m_seed[3] = mut1;

        return mut1;
    }

    float State::NextFloat() {
        return ((float)(NextInt() & 0x7FFFFF)) * 1.192093e-07;
    }

	float State::Range(float minInclude, float maxExclude) {
        float r = NextFloat();
        return (1.0f - r) * maxExclude + r * minInclude;
	}

    int32_t State::Range(int32_t minInclude, int32_t maxExclude) {
        if (minInclude > maxExclude)
            std::swap(minInclude, maxExclude);

        uint32_t diff = maxExclude - minInclude;
        if (diff) {
            return minInclude + (NextInt() % diff);
        }
        return minInclude;
    }

    Vector2 State::GetRandomUnitCircle() {

        // get random 
        float rad = Range(0.f, PI * 2.f);
        float x = cosf(rad);
        float y = sinf(rad);

        float ze = Range(0.f, 1.f);
        float d = sqrtf(ze);

        return Vector2(x * d, y * d);
        /*
        uint32_t uVar1;
        uint32_t* seed;
        uint32_t uVar2;
        uint32_t uVar3;
        float fVar4;
        float fVar5;
        float rad;

        uVar1 = seed[3];
        uVar3 = *seed << 0xb ^ *seed;
        uVar3 = (uVar1 >> 0xb ^ uVar3) >> 8 ^ uVar1 ^ uVar3;
        fVar4 = (float)(uint64_t)(uVar3 & 0x7fffff) * 1.192093e-07;
        rad = (1.0 - fVar4) * 6.283185 + fVar4 * 0.0;
        fVar4 = cosf(rad);
        rad = sinf(rad);
        *seed = seed[2];
        uVar2 = seed[1] << 0xb ^ seed[1];
        seed[1] = uVar1;
        seed[2] = uVar3;
        uVar2 = (uVar3 >> 0xb ^ uVar2) >> 8 ^ uVar3 ^ uVar2;
        seed[3] = uVar2;
        fVar5 = (float)(uint64_t)(uVar2 & 0x7fffff) * 1.192093e-07;
        fVar5 = sqrtf((1.0 - fVar5) + fVar5 * 0.0);
        *param_1 = CONCAT44(rad * fVar5, fVar4 * fVar5);*/
    }

    Vector3 State::OnUnitSphere() {
        float dist = Range(-1.f, 1.f);
        float rad = Range(0.f, PI * 2.f);

        float vecX = sqrtf(1.0 - dist * dist);

        return Vector3(cosf(rad) * vecX, sinf(rad) * vecX, dist);
    }

    Vector3 State::InsideUnitSphere() {
        // unity does this by sampling a point on the surface of sphere
        auto vec = OnUnitSphere();

        // then bringing that point in by a random distance
        float dist = powf(NextFloat(), 1.f / 3.f);
        return vec * dist;
    }

}
