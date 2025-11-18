#include "RandomSpawn.h"
#include "Types.h"
#include <type_traits>
#include <vector>

// TODO this seems finicky,
//  but we dont need this to be used anywhere outside of here (for now)
template<>
struct avledet::util::Streamer<avledet::gen::RandomSpawn>
{
    decltype(auto) operator()(Reader &reader) const
    {
        return gen::RandomSpawn(reader);
    }
};

namespace avledet::gen {

    RandomSpawn::RandomSpawn(util::Reader &reader)
    {
        // parse an individual rs
        this->m_chance = reader.read<float>();

        static_assert(std::is_same_v<std::underlying_type_t<avledet::util::Theme>, std::int32_t>);
        this->m_dungeon_require_theme = reader.read<avledet::util::Theme>();

        static_assert(std::is_same_v<std::underlying_type_t<util::Biome>, std::uint16_t>);
        this->m_biome = reader.read<util::Biome>();

        this->m_not_in_lava = reader.read<bool>();

        this->m_min_elevation = reader.read<std::int32_t>();
        this->m_max_elevation = reader.read<std::int32_t>();

        this->m_indexed = reader.read<decltype(m_indexed)>();
    }

    std::vector<RandomSpawn> RandomSpawn::parse_list(util::Reader &reader)
    {
        return reader.read<std::vector<RandomSpawn>>();
    }

}// namespace avledet::gen
