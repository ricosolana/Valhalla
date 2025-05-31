#pragma once

#include <cstdint>
#include <Vector.h>

namespace avledet::util {

    namespace CSU {
        class Random {
        private:
            std::uint32_t m_seed[4];

        public:
            Random();
            Random(std::int32_t seed);
            Random(const Random& other); // copy construct

            // Returns a random float from 0 to 1
            float next_float();
            std::uint32_t next_int();
            //float Value() { return next_float(); }

            float range(float minInclude, float maxExclude);
            std::int32_t range(std::int32_t minInclude, std::int32_t maxExclude);

            Vector2f inside_unit_circle();
            Vector3f on_unit_sphere();
            Vector3f inside_unit_sphere();

        public:
            const std::uint32_t* extract_seed() {
                return m_seed;
            }
        };
    }// namespace avledet::util::CSU

    //USER_ID_t GenerateUID();

    //void GenerateAlphaNum(char* out, size_t outSize);

    //std::string GenerateAlphaNum(size_t count);

}// namespace avledet::util

namespace VUtils::Random {
    using State = avledet::util::CSU::Random;
}