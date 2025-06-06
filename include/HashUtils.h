#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Prefab.h"

class ZDO;

namespace ankerl::unordered_dense {

    /*
    template<>
    struct hash<std::reference_wrapper<ZDO>> {
        using is_avalanching = void;

        auto operator()(std::reference_wrapper<ZDO> v) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<std::uintptr_t>{}(v.get());
            //return ankerl::unordered_dense::detail::wyhash::hash(v.m_encoded);
        }
    };*/
    
    /*
    struct prefab_hash {
        using is_transparent = void; // enable heterogeneous overloads
        using is_avalanching = void;

        auto operator()(avledet::util::Hash v) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<avledet::util::Hash>{}(v);
        }
    };

    template <>
    struct hash<Prefab> {
        using is_avalanching = void;

        auto operator()(const std::unique_ptr<Prefab>& v) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<avledet::util::Hash>{}(v->m_hash);
        }

        auto operator()(const Prefab& v) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<std::int32_t>{}(v.m_hash);
        }

        auto operator()(avledet::util::Hash v) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<std::int32_t>{}(v);
        }
    };*/

    template <>
    struct hash<Prefab> {
        using is_transparent = void;
        using is_avalanching = void; // mark class as high quality avalanching hash

        [[nodiscard]] auto operator()(const Prefab& prefab) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<avledet::util::Hash>{}(prefab.m_hash);
        }

        [[nodiscard]] auto operator()(avledet::util::Hash hash) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<avledet::util::Hash>{}(hash);
        }

        [[nodiscard]] auto operator()(std::string_view str) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<avledet::util::Hash>{}(VUtils::String::GetStableHashCode(str));
        }
    };

    struct string_hash {
        using is_transparent = void; // enable heterogeneous overloads
        using is_avalanching = void; // mark class as high quality avalanching hash

        [[nodiscard]] auto operator()(std::string_view str) const noexcept -> std::uint64_t {
            return ankerl::unordered_dense::hash<std::string_view>{}(str);
        }
    };

} // namespace ankerl::unordered_dense

/*
template<typename K, typename V> 
    requires std::is_same_v<K, std::string>
using avledet::util::Map = ankerl::unordered_dense::map<K, V, ankerl::unordered_dense::string_hash>;

template<typename K, typename V>
    requires !std::is_same_v<K, std::string>
using avledet::util::Map = ankerl::unordered_dense::map<K, V>;

template<typename K>
    requires std::is_same_v<K, std::string>
using avledet::util::Set = ankerl::unordered_dense::set<Args...>;

template<typename K>
    requires !std::is_same_v<K, std::string>
using avledet::util::Set = ankerl::unordered_dense::set<Args...>;
*/