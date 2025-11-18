#pragma once

#include "DataStream.h"
#include "Hashes.h"
#include "Quaternion.h"
#include "Vector.h"

#include <cstdint>
#include <limits>
#include <vector>

// maybe create new ns
namespace avledet::gen {

    class RandomSpawn
    {
      public:
        float m_chance;
        avledet::util::Theme m_dungeon_require_theme;
        avledet::util::Biome m_biome;
        bool m_not_in_lava;
        int m_min_elevation;
        int m_max_elevation;
        std::vector<std::uint16_t> m_indexed;// values up 65535 max

      public:
        RandomSpawn(util::Reader &reader);

      public:
        static std::vector<RandomSpawn> parse_list(util::Reader &reader);
    };

}// namespace avledet::gen
