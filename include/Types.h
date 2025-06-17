#pragma once

#include <cstdint>

#include <ratio>
#include <span>
#include <vector>

#include <ankerl/unordered_dense.h>
#include <quill/LogMacros.h>
#include <tracy/Tracy.hpp>

#include "CompileSettings.h"

#if VH_IS_ON(VH_USE_MODS)
    #include <sol/state.hpp>
#endif

namespace avledet::util {
    using Byte     = char;             // Unsigned 8 bit
    using Hash     = std::int32_t;     // Used for RPC method hashing
    using UserID   = std::int64_t;     // Should rename to UID
    using Bytes    = std::vector<Byte>;// Vector of bytes
    using ByteView = std::span<Byte>;
    using Strings  = std::vector<std::string>;

    using Ticks = std::chrono::duration<std::int64_t, std::ratio<1, 10000000>>;

    template<typename K, typename V, typename Hash = ankerl::unordered_dense::hash<K>,
             typename Equal = std::equal_to<K>>
    using Map = ankerl::unordered_dense::map<K, V, Hash, Equal>;

    template<typename K, typename Hash = ankerl::unordered_dense::hash<K>, typename Equal = std::equal_to<K>>
    using Set = ankerl::unordered_dense::set<K, Hash, Equal>;

    struct Color
    {
        float r, g, b, a;

        constexpr Color() :
            r(0),
            g(0),
            b(0),
            a(1)
        {
        }

        constexpr Color(float r, float g, float b) :
            r(r),
            g(g),
            b(b),
            a(1)
        {
        }

        constexpr Color(float r, float g, float b, float a) :
            r(r),
            g(g),
            b(b),
            a(a)
        {
        }

        Color Lerp(Color const &other, float t);
    };

    struct Color32
    {
        avledet::util::Byte r, g, b, a;

        constexpr Color32() :
            r(0),
            g(0),
            b(0),
            a(1)
        {
        }

        constexpr Color32(avledet::util::Byte r, avledet::util::Byte g, avledet::util::Byte b) :
            r(r),
            g(g),
            b(b),
            a(1)
        {
        }

        constexpr Color32(avledet::util::Byte r, avledet::util::Byte g, avledet::util::Byte b,
                          avledet::util::Byte a) :
            r(r),
            g(g),
            b(b),
            a(a)
        {
        }

        //Color Lerp(const Color& other, float t);

        Color32 Lerp(Color32 const &other, float t);
    };

    // TODO inline within Color
    namespace Colors {
        static constexpr Color BLACK = Color(0, 0, 0);
        static constexpr Color RED   = Color(1, 0, 0);
        static constexpr Color GREEN = Color(0, 1, 0);
        static constexpr Color BLUE  = Color(0, 0, 1);
    }// namespace Colors


    // TODO append ..Type
    enum class Biome : std::uint16_t
    {
        None        = 0,
        Meadows     = 1 << 0,
        Swamp       = 1 << 1,
        Mountain    = 1 << 2,
        BlackForest = 1 << 3,
        Plains      = 1 << 4,
        AshLands    = 1 << 5,
        DeepNorth   = 1 << 6,
        Ocean       = 1 << 8,
        Mistlands   = 1 << 9,
        BiomesMax// DO NOT USE
    };

    // TODO append ..type
    enum class BiomeArea : std::uint8_t
    {
        None       = 0,
        Edge       = 1 << 0,
        Median     = 1 << 1,
        Everything = Edge | Median,
    };

    // The priority type of this ZDO
    enum class ObjectType : std::uint8_t
    {
        DEFAULT,
        PRIORITIZED,
        SOLID,
        TERRAIN
    };

}// namespace avledet::util
