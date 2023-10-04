#pragma once

#include "VUtils.h"
#include "VUtilsTraits.h"

template<typename T, size_t ...COUNTS>
    requires std::is_integral_v<T>&& std::is_unsigned_v<T>
class BitPack {
    static_assert((COUNTS + ...) == sizeof(T) * 8, "Exactly all bits must be utilized in mask");

public:
    using type = T;



    template<size_t index>
        requires (index < sizeof...(COUNTS))
    using count = VUtils::Traits::variadic_value_at_index<index, COUNTS...>;

    template<size_t index>
    static constexpr auto count_v = count<index>::value;



    template<size_t...>
    struct offset;

    template<size_t index>
    //requires (index == 0)
    struct offset<index>
        : std::integral_constant<size_t, 0>
    { };

    template<size_t index>
        requires (index > 0)
    struct offset<index>
        : VUtils::Traits::variadic_accumulate_values_to_index<index - 1ULL, COUNTS...>
    { };

    // now accumulate in reverse, first parameter pack ints are most significant (have highest offsets)
    // l

    template<size_t index>
    static constexpr auto offset_v = offset<index>::value;



    template<size_t index>
    using capacity = std::integral_constant<size_t, (1ULL << count<index>::value) - 1ULL>;

    template<size_t index>
    static constexpr auto capacity_v = capacity<index>::value;

private:
    T m_data{};

public:
    constexpr BitPack() {}
    constexpr BitPack(T data) : m_data(data) {}

    void operator=(const BitPack<T, COUNTS...>& other) {
        this->m_data = other.m_data;
    }

    bool operator==(const BitPack<T, COUNTS...>& other) const {
        return m_data == other.m_data;
    }

    bool operator!=(const BitPack<T, COUNTS...>& other) const {
        return !(*this == other);
    }

    operator bool() const {
        return static_cast<bool>(m_data);
    }

    operator T() const {
        return m_data;
    }

    // Get the value of a specified member at index
    template<uint8_t index>
    type Get() const {
        //return (m_data >> offset<index>::value) & capacity<index>::value;
        auto o = offset_v<index>;
        auto c = capacity_v<index>;
        return (m_data >> o) & c;
        //return (m_data >> offset<index>::value) & capacity<index>::value;
    }

    // Set the value of a specified member at index to 0
    template<uint8_t index>
    void Clear() {
        m_data &= ~(capacity_v<index> << offset_v<index>);

        assert(Get<index>() == 0);
    }

    // Set the value of a specified member at index
    template<uint8_t index>
    void Set(type value) {
        Clear<index>();
        Merge<index>(value);

        assert(Get<index>() == value);
    }

    // Clear the bits within a specified mask
    template<uint8_t index>
    void Unset(type value) {
        // flip to get negated mask
        //value ^= std::numeric_limits<type>::max();

        Set<index>(Get<index>() & static_cast<type>(~value));

        //value = ~value;

        // merge negated mask to rid bits
        //m_data &= (value & capacity_v<index>) << offset_v<index>;

        //m_data &= ((~value) & capacity_v<index>) << offset_v<index>;

        //m_data &= ((~value) & capacity_v<index>) << offset_v<index>;

        //m_data &= ((~value) & capacity_v<index>) << offset_v<index>;

        //assert((Get<index> & value) == 0);

        assert((Get<index>() & value) == 0);
    }

    // Merge the bits of a specified index with another value
    template<uint8_t index>
    void Merge(type value) {
        m_data |= (value & capacity_v<index>) << offset_v<index>;

        assert((Get<index>() & value) == value);
    }
};