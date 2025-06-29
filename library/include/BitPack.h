#pragma once

#include <cstdint>

#include "VUtilsTraits.h"

template<typename T, std::size_t... BitAllocs>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
class BitPack
{
    static_assert((BitAllocs + ...) == sizeof(T) * 8, "Exactly all bits must be utilized in mask");

  public:
    using type = T;


    template<std::size_t index>
        requires(index < sizeof...(BitAllocs))
    using count = VUtils::Traits::variadic_value_at_index<index, BitAllocs...>;

    template<std::size_t index>
    static constexpr auto count_v = count<index>::value;


    template<std::size_t...>
    struct offset;

    template<std::size_t index>
    //requires (index == 0)
    struct offset<index> : std::integral_constant<std::size_t, 0>
    {};

    template<std::size_t index>
        requires(index > 0)
    struct offset<index> : VUtils::Traits::variadic_accumulate_values_to_index<index - 1ULL, BitAllocs...>
    {};

    // now accumulate in reverse, first parameter pack ints are most significant (have highest offsets)
    // l

    template<std::size_t index>
    static constexpr auto offset_v = offset<index>::value;


    template<std::size_t index>
    using capacity = std::integral_constant<std::size_t, (1ULL << count<index>::value) - 1ULL>;

    template<std::size_t index>
    static constexpr auto capacity_v = capacity<index>::value;

  private:
    T m_data;

  public:
    constexpr BitPack() :
        m_data {}
    {
    }

    constexpr BitPack(T data) :
        m_data(data)
    {
    }

    constexpr friend auto operator<=>(BitPack<T, BitAllocs...> const &lhs,
                                      BitPack<T, BitAllocs...> const &rhs) noexcept
            = default;

    ////constexpr friend bool operator==(BitPack<T, BitAllocs...> const &lhs,
    ////                                 BitPack<T, BitAllocs...> const &rhs) noexcept
    ////{
    ////    return lhs.m_data == rhs.m_data;
    ////}

    ////constexpr friend bool operator<(BitPack<T, BitAllocs...> const &lhs,
    ////                                BitPack<T, BitAllocs...> const &rhs) noexcept
    ////{
    ////    return lhs.m_data < rhs.m_data;
    ////}

    //spaceship (3-way) <=> operator might be broken, but again, 7 year old paper...
    // https://www.foonathan.net/2018/10/spaceship-proposals/
    //constexpr friend auto operator<=>(BitPack<T, BitAllocs...> const &lhs,
    //                                  BitPack<T, BitAllocs...> const &rhs) noexcept
    //{
    //    return lhs.m_data <=> rhs.m_data;
    //}

    operator bool() const
    {
        return static_cast<bool>(m_data);
    }

    //operator T() const
    //{
    //    return m_data;
    //}

    // Get the value of a specified member at index
    template<std::uint8_t index>
    type get() const
    {
        //return (m_data >> offset<index>::value) & capacity<index>::value;
        auto o = offset_v<index>;
        auto c = capacity_v<index>;
        return (m_data >> o) & c;
        //return (m_data >> offset<index>::value) & capacity<index>::value;
    }

    // Set the value of a specified member at index to 0
    template<std::uint8_t index>
    void clear()
    {
        m_data &= ~(capacity_v<index> << offset_v<index>);

        assert(get<index>() == 0);
    }

    // Set the value of a specified member at index
    template<std::uint8_t index>
    void set(type value)
    {
        clear<index>();
        merge<index>(value);

        assert(get<index>() == value);
    }

    // Clear the bits within a specified mask
    //  TODO needs better name
    template<std::uint8_t index>
    void unset(type value)
    {
        // flip to get negated mask
        //value ^= std::numeric_limits<type>::max();

        set<index>(get<index>() & static_cast<type>(~value));

        //value = ~value;

        // merge negated mask to rid bits
        //m_data &= (value & capacity_v<index>) << offset_v<index>;

        //m_data &= ((~value) & capacity_v<index>) << offset_v<index>;

        //m_data &= ((~value) & capacity_v<index>) << offset_v<index>;

        //m_data &= ((~value) & capacity_v<index>) << offset_v<index>;

        //assert((Get<index> & value) == 0);

        assert((get<index>() & value) == 0);
    }

    // Merge the bits of a specified index with another value
    template<std::uint8_t index>
    void merge(type value)
    {
        m_data |= (value & capacity_v<index>) << offset_v<index>;

        assert((get<index>() & value) == value);
    }

    T get_underlying() const
    {
        return m_data;
    }
};