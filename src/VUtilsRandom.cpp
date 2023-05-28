#include <stdint.h>
#include <limits>
#include <openssl/rand.h>
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

    State::State() 
        : State(steady_clock::now().time_since_epoch().count()) {}

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

    Vector2f State::InsideUnitCircle() {

        // get random 
        float rad = Range(0.f, PI * 2.f);
        float x = cosf(rad);
        float y = sinf(rad);

        float ze = Range(0.f, 1.f);
        float d = sqrtf(ze);

        return Vector2f(x * d, y * d);
    }

    Vector3f State::OnUnitSphere() {
        float dist = Range(-1.f, 1.f);
        float rad = Range(0.f, PI * 2.f);

        float vecX = sqrtf(1.0 - dist * dist);

        return Vector3f(cosf(rad) * vecX, sinf(rad) * vecX, dist);
    }

    Vector3f State::InsideUnitSphere() {
        // unity does this by sampling a point on the surface of sphere
        auto vec = OnUnitSphere();

        // then bringing that point in by a random distance
        float dist = powf(NextFloat(), 1.f / 3.f);
        return vec * dist;
    }


    /*
    void GenerateBytes(BYTE_t* out, unsigned int count) {
        RAND_bytes(reinterpret_cast<unsigned char*>(out), count);
    }

    BYTES_t GenerateBytes(unsigned int count) {
        BYTES_t result(count);

        GenerateBytes(result.data(), count);

        return result;
    }

    static const std::string charsAlphaNum = "abcdefghijklmnpqrstuvwxyzABCDEFGHIJKLMNPQRSTUVWXYZ023456789";
    */
    
    static constexpr std::string_view CHARS_ALPHA_NUM = "abcdefghijklmnpqrstuvwxyzABCDEFGHIJKLMNPQRSTUVWXYZ023456789";

    /*
        Valheim GenerateUID method has a sliced probabilistic range:
        return object.GetHashCode .. UnityEngine.Random.Range(1, int.MaxValue);

        [INT_MIN + 1, INT_MAX + INT_MAX)
        [-2147483648 + 1, 2147483647 + 2147483647)
        [0xFFFFFFFF80000001, 00000000FFFFFFFE)

        [0x1, 0x00000000FFFFFFFE) U [0xFFFFFFFF80000001, 0xFFFFFFFFFFFFFFFF]

        [0x1, 0x00000000FFFFFFFD] U [0xFFFFFFFF80000001, 0xFFFFFFFFFFFFFFFF]

        unsigned:
        [1, 4294967293] U [18446744071562067969, 18446744073709551615]

        signed:
        [-2,147,483,647, +4,294,967,293]
    */

    // Generates a Random UID
    //  This function is non-conforming to the Valheim spec
    //  It returns a positive number for conformance with the strict 32-bit OWNER_t
    OWNER_t GenerateUID() {
        State state;
        return (int64_t) state.Range(
            std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()) +
            (int64_t) state.Range(1, std::numeric_limits<int32_t>::max());
        //return state.Range(1, std::numeric_limits<int32_t>::max());
    }

    void GenerateAlphaNum(char* out, size_t outSize) {
        VUtils::Random::State state;
        for (size_t i = 0; i < outSize; i++) {
            out[i] = CHARS_ALPHA_NUM[state.Range((int32_t)0, (int32_t)CHARS_ALPHA_NUM.length())];
        }
    }

    std::string GenerateAlphaNum(size_t count) {
        std::string res;
        res.resize(count);

        GenerateAlphaNum(res.data(), count);

        return res;
    }
}
