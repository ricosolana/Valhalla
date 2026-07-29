#pragma once

#include <cstdint>
#include <gtl/btree.hpp>
#include <ratio>
#include <span>
#include <vector>

#include <ankerl/unordered_dense.h>
#include <quill/bundled/fmt/format.h>
#include <quill/bundled/fmt/ostream.h>
#include <quill/DeferredFormatCodec.h>
#include <quill/DirectFormatCodec.h>
#include <quill/HelperMacros.h>
#include <quill/LogMacros.h>
#include <quill/bundled/fmt/base.h>
#include <magic_enum/magic_enum_all.hpp>
#include <tracy/Tracy.hpp>

#include "CompileSettings.h"
#include "Vector.h"
#include "VUtilsTraits.h"

// Order-dependent //dont put before 'CompileSettings.h'
#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
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

    // TODO rename under CSU
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

        static Color Lerp(Color const &a, Color const &b, float t);
    };

    // TODO renaem under CSU (if true)
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

        static Color32 Lerp(Color32 const &a, Color32 const &b, float t);
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

    enum class Theme
    {
        Crypt             = 1,
        SunkenCrypt       = 2,
        Cave              = 4,
        ForestCrypt       = 8,
        GoblinCamp        = 16,
        MeadowsVillage    = 32,
        MeadowsFarm       = 64,
        DvergerTown       = 128,
        DvergerBoss       = 256,
        ForestCryptHildir = 512,
        CaveHildir        = 1024,
        PlainsFortHildir  = 2048,
        AshlandRuins      = 4096,
        FortressRuins     = 8192
    };

    using ZoneID = Vector2s;

}// namespace avledet::util

// TODO migrate away...
using ZoneID = avledet::util::ZoneID;

template<class T>
//requires avledet::util::traits::is_iterable<T>
std::ostream &operator<<(std::ostream &st, std::vector<T> const &value)
{
    st << "[ ";
    for (int i = 0; i < value.size(); i++) {
        st << value[i];
        if (i < (int) value.size() - 1) {
            st << ",";
        }
        st << " ";
    }
    st << "]";
    return st;
}

/**
 * STD::VECTOR QUILL LOGGABLE FACADE
*/

//QUILL_LOGGABLE_DEFERRED_FORMAT()
template<class T>
//requires (avledet::util::traits::is_iterable<T> && )
struct fmtquill::formatter<std::vector<T>> : fmtquill::ostream_formatter
{};

template<class T>
//requires avledet::util::traits::is_iterable<T>
struct quill::Codec<std::vector<T>> : quill::DeferredFormatCodec<T>
{};

/**
 * ENUM CLASS QUILL LOGGABLE FACADE
*/

/* template<avledet::util::traits::scoped_enum T>
struct fmtquill::formatter<T> : fmtquill::ostream_formatter
{};

template<avledet::util::traits::scoped_enum T>
struct quill::Codec<T> : quill::DeferredFormatCodec<T>
{}; */


// TODO fix the below
//  was intended to add enum logging overloads


/* template<avledet::util::traits::scoped_enum T>
std::ostream& operator<<(std::ostream& os, T value)
{
    return os << magic_enum::enum_name(value);
}

// 2. concept: scoped enum AND no formatter already defined
template<typename T>
struct is_quill_provided_enum : std::false_type {};

template<>
struct is_quill_provided_enum<quill::LogLevel> : std::true_type {};

// add more specializations as needed

template<typename T>
concept scoped_enum_needs_formatter =
    avledet::util::traits::scoped_enum<T> &&
    !is_quill_provided_enum<T>::value;

// 3. formatter specialization only for those
template<scoped_enum_needs_formatter T>
struct fmtquill::formatter<T> : fmtquill::ostream_formatter
{};

// 4. codec specialization only for those
template<scoped_enum_needs_formatter T>
struct quill::Codec<T> : quill::DeferredFormatCodec<T>
{};
 */