#pragma once

#include "VUtils.h"
#include "VUtilsTraits.h"

template<typename T, size_t ...COUNTS>
    requires std::is_integral_v<T>&& std::is_unsigned_v<T>
class BitPack {
    //static_assert((COUNTS + ...) > 0, "Counts summed together must be greater than 0");
    //static_assert((COUNTS + ...) <= sizeof(T) * 8, "Counts summed together must fit within mask");

    static_assert((COUNTS + ...) == sizeof(T) * 8, "Exactly all bits must be utilized in mask");

public:
    using type = T;

    template<size_t index>
        requires (index < sizeof...(COUNTS))
    using count = VUtils::Traits::variadic_value_at_index<index, COUNTS...>;



    template<size_t...>
    struct offset;

    template<size_t index>
        requires (index == 0)
    struct offset<index>
        : std::integral_constant<size_t, 0>
    { };

    template<size_t index>
        requires (index > 0 && index < (sizeof...(COUNTS)))
    struct offset<index>
        : VUtils::Traits::variadic_accumulate_values_to_index<index - 1, COUNTS...>
    { };



    // require that the offset index not be greater than the container

    //template<size_t index>
    //using offset = VUtils::Traits::variadic_accumulate_values_to_index<index, COUNTS...>;    

    template<size_t index>
    //requires (index < sizeof...(COUNTS))
    using capacity = std::integral_constant<size_t, (1 << count<index>::value) - 1>;

private:
    T m_data{};

private:
    /*
    template<uint8_t index, uint8_t A, uint8_t... Bs>
    constexpr uint8_t OffsetImpl() const {
        if constexpr (index == (sizeof...(Bs)) || sizeof...(Bs) == 0) {
            return A;
        }
        else {
            return A + OffsetImpl<index, Bs...>();
        }
    }

    template<uint8_t index, uint8_t A, uint8_t... Bs>
    constexpr T CountImpl() const {
        if constexpr (index == (sizeof...(Bs)) || sizeof...(Bs) == 0) {
            return static_cast<T>(A);
        }
        else {
            return CountImpl<index, Bs...>();
        }
    }*/

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

    /*
    // Get the width of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    constexpr T GetCount() const {
        return CountImpl<sizeof...(COUNTS) - index - 1, COUNTS...>();
    }

    // Get the offset of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    constexpr T GetOffset() const {
        if constexpr (index == 0) {
            return 0;
        }
        else {
            return OffsetImpl<sizeof...(COUNTS) - index, COUNTS...>();
        }
    }*/

    // Get the value of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    type Get() const {
        //return (m_data >> GetOffset<index>()) & ((1 << GetCount<index>()) - 1);
        //return m_data >> offset<index>::value
        //return (m_data >> offset<index>::value) & ((1 << count<index>::value) - 1);
        return (m_data >> offset<index>::value) & capacity<index>::value;
    }

    // Set the value of a specified member at index to 0
    template<uint8_t index>
    void Clear() {
        //m_data &= ~(((1 << GetCount<index>()) - 1) << GetOffset<index>());
        //m_data &= ~(((1 << count<index>::value) - 1) << offset<index>::value);
        m_data &= ~(capacity<index>::value << offset<index>::value);

        assert(Get<index>() == 0);
    }

    // Set the value of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    void Set(type value) {
        Clear<index>();
        Merge<index>(value);

        assert(Get<index>() == value);
    }

    // Merge the bits of a specified index with another value
    template<uint8_t index>
    void Merge(type value) {
        //m_data |= (value & ((1 << GetCount<index>()) - 1)) << GetOffset<index>();
        //m_data |= (value & ((1 << count<index>::value) - 1)) << offset<index>::value;
        m_data |= (value & capacity<index>::value) << offset<index>::value;

        assert((Get<index>() & value) == value);
    }
};