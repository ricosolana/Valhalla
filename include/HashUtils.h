#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"

class ZDO;

namespace ankerl::unordered_dense {

    /*
    template<>
    struct hash<std::reference_wrapper<ZDO>> {
        using is_avalanching = void;

        auto operator()(std::reference_wrapper<ZDO> v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::hash<std::uintptr_t>{}(v.get());
            //return ankerl::unordered_dense::detail::wyhash::hash(v.m_encoded);
        }
    };*/
    
    template <>
    struct hash<ZDOID> {
        using is_avalanching = void;

        auto operator()(ZDOID v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::hash<decltype(ZDOID::m_pack)::type>{}(v.m_pack);
            //return ankerl::unordered_dense::hash<decltype(ZDOID::m_encoded)>{}(v.m_encoded);
            //return ankerl::unordered_dense::detail::wyhash::hash(v.m_encoded);
        }
    };

    template <>
    struct hash<Vector2i> {
        using is_avalanching = void;

        auto operator()(Vector2i v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::detail::wyhash::hash(&v, sizeof(v));
        }
    };

    template <>
    struct hash<Vector2s> {
        using is_avalanching = void;

        auto operator()(Vector2s v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::detail::wyhash::hash(&v, sizeof(v));
        }
    };

    template <>
    struct hash<Vector3f> {
        using is_avalanching = void;

        auto operator()(Vector3f v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::detail::wyhash::hash(&v, sizeof(v));
        }
    };

    struct string_hash {
        using is_transparent = void; // enable heterogeneous overloads
        using is_avalanching = void; // mark class as high quality avalanching hash

        [[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t {
            return ankerl::unordered_dense::hash<std::string_view>{}(str);
        }
    };

} // namespace ankerl::unordered_dense

/*
template<typename K, typename V> 
    requires std::is_same_v<K, std::string>
using UNORDERED_MAP_t = ankerl::unordered_dense::map<K, V, ankerl::unordered_dense::string_hash>;

template<typename K, typename V>
    requires !std::is_same_v<K, std::string>
using UNORDERED_MAP_t = ankerl::unordered_dense::map<K, V>;

template<typename K>
    requires std::is_same_v<K, std::string>
using UNORDERED_SET_t = ankerl::unordered_dense::set<Args...>;

template<typename K>
    requires !std::is_same_v<K, std::string>
using UNORDERED_SET_t = ankerl::unordered_dense::set<Args...>;
*/