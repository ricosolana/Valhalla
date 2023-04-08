#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"

namespace ankerl::unordered_dense {

    template <>
    struct hash<ZDOID> {
        using is_avalanching = void;

        auto operator()(const ZDOID& v) const noexcept -> uint64_t {
            //return ankerl::unordered_dense::detail::wyhash::mix(v.m_id, v.m_uuid);
            return ankerl::unordered_dense::detail::wyhash::hash(&v, sizeof(v));
        }
    };

    template <>
    struct hash<Vector2i> {
        using is_avalanching = void;

        auto operator()(const Vector2i& v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::detail::wyhash::hash(&v, sizeof(v));
        }
    };

    template <>
    struct hash<Vector3f> {
        using is_avalanching = void;

        auto operator()(const Vector3f& v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::detail::wyhash::hash(&v, sizeof(v));
        }
    };
} // namespace ankerl::unordered_dense
