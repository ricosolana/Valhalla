#pragma once

#include <concepts>

// unsafe to introduce global enum operators
/*
// A bunch of enum helper methods to make masks and bitshifts way fricking easier
template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator&(Enum lhs, Enum rhs) {
    return (Enum)(std::to_underlying(lhs) & std::to_underlying(rhs));
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator|(Enum lhs, Enum rhs) {
    return (Enum)(std::to_underlying(lhs) | std::to_underlying(rhs));
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator^(Enum lhs, Enum rhs) {
    return (Enum)(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator>>(Enum lhs, Enum rhs) {
    return (Enum)(std::to_underlying(lhs) >> std::to_underlying(rhs));
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator<<(Enum lhs, Enum rhs) {
    return (Enum)(std::to_underlying(lhs) << std::to_underlying(rhs));
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator~(Enum lhs) {
    return (Enum)(~std::to_underlying(lhs));
}


// Enum and primitive binary ops
template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator&(Enum lhs, V rhs) {
    return lhs & (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator|(Enum lhs, V rhs) {
    return lhs | (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator^(Enum lhs, V rhs) {
    return lhs ^ (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator>>(Enum lhs, V rhs) {
    return lhs << (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator<<(Enum lhs, V rhs) {
    return lhs << (Enum)rhs;
}


// Enum and Enum binary assign ops
template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator&=(Enum& lhs, Enum rhs) {
    using T = std::underlying_type_t<Enum>;
    reinterpret_cast<T&>(lhs) &= std::to_underlying(rhs);
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator|=(Enum& lhs, Enum rhs) {
    using T = std::underlying_type_t<Enum>;
    reinterpret_cast<T&>(lhs) |= std::to_underlying(rhs);
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator^=(Enum& lhs, Enum rhs) {
    using T = std::underlying_type_t<Enum>;
    reinterpret_cast<T&>(lhs) ^= std::to_underlying(rhs);
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator>>=(Enum& lhs, Enum rhs) {
    using T = std::underlying_type_t<Enum>;
    reinterpret_cast<T&>(lhs) >>= std::to_underlying(rhs);
}

template<typename Enum>
    requires std::is_scoped_enum_v<Enum>
decltype(auto) operator<<=(Enum& lhs, Enum rhs) {
    using T = std::underlying_type_t<Enum>;
    reinterpret_cast<T&>(lhs) <<= std::to_underlying(rhs);
}


// Enum and primitive binary assign ops
template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator&=(Enum& lhs, V rhs) {
    lhs &= (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator|=(Enum& lhs, V rhs) {
    lhs |= (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator^=(Enum& lhs, V rhs) {
    lhs ^= (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator>>=(Enum& lhs, V rhs) {
    lhs >>= (Enum)rhs;
}

template<typename Enum, typename V>
    requires std::is_scoped_enum_v<Enum>&& std::is_fundamental_v<V>
decltype(auto) operator<<=(Enum& lhs, V rhs) {
    lhs <<= (Enum)rhs;
}



// Primitive and Enum binary ops
template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator&(V lhs, Enum rhs) {
    return lhs & std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator|(V lhs, Enum rhs) {
    return lhs | std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator^(V lhs, Enum rhs) {
    return lhs ^ std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator>>(V lhs, Enum rhs) {
    return lhs >> std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator<<(V lhs, Enum rhs) {
    return lhs << std::to_underlying(rhs);
}



// Primitive and enum binary assign ops
template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator&=(V& lhs, Enum rhs) {
    lhs &= std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator|=(V& lhs, Enum rhs) {
    lhs |= std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator^=(V& lhs, Enum rhs) {
    lhs ^= std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator>>=(V& lhs, Enum rhs) {
    lhs >>= std::to_underlying(rhs);
}

template<typename V, typename Enum>
    requires std::is_fundamental_v<V>&& std::is_scoped_enum_v<Enum>
decltype(auto) operator<<=(V& lhs, Enum rhs) {
    lhs <<= std::to_underlying(rhs);
}*/