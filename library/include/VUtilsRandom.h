#pragma once

// reverse engineered implementation of Unity Random and associated functions
// these are algorithms only, not steps, so shoo patent lawyers!

#include "Vector.h"
#include "VUtils.h"
#include <cstdint>

namespace avledet::util {

    namespace CSU {
        class Random
        {
          private:
            std::uint32_t m_seed[4];

          public:
            Random();
            Random(std::int32_t seed);
            Random(Random const &other);// copy construct

            // Returns a random float from 0 to 1
            float next_float();
            std::uint32_t next_int();

            float value()
            {
                return next_float();
            }

            float range(float minInclude, float maxExclude);
            std::int32_t range(std::int32_t minInclude, std::int32_t maxExclude);

            Vector2f inside_unit_circle();
            Vector3f on_unit_sphere();
            Vector3f inside_unit_sphere();

          public:
            //const std::uint32_t* extract_seed() {
            //    return m_seed;
            //}
        };

    }// namespace CSU

    avledet::util::UserID GenerateUID();

    std::string generate(std::string_view charset, std::size_t count);

    void GenerateAlphaNum(char *out, std::size_t outSize);

    std::string GenerateAlphaNum(std::size_t count);

    //TODO migrate away
    using State = CSU::Random;

}// namespace avledet::util

//TODO migrate away
namespace VUtils {
    namespace Random = avledet::util;
}
