#pragma once

#include "NetID.h"
#include "Vector.h"

namespace std {

    template <>
    struct hash<NetID>
    {
        std::size_t operator()(const NetID& id) const {
            std::size_t hash1 = std::hash<decltype(NetID::m_id)>{}(id.m_id);
            std::size_t hash2 = std::hash<decltype(NetID::m_uuid)>{}(id.m_uuid);

            return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
        }
    };

    template <>
    struct hash<Vector2i>
    {
        std::size_t operator()(const Vector2i& vec) const {
            return (std::hash<int32_t>{}(vec.x) << 8) ^ (std::hash<int32_t>{}(vec.y));
        }
    };

    template <>
    struct hash<Vector3>
    {
        std::size_t operator()(const Vector3& vec) const {
            return std::hash<float>{}(vec.x) ^ (std::hash<float>{}(vec.y) << 8) ^ (std::hash<float>{}(vec.z) << 16);
        }
    };

    template <>
    struct equal_to<Vector2i>
    {
        bool operator()(const Vector2i& lhs, const Vector2i& rhs) const {
            return lhs.x == rhs.x && lhs.y == rhs.y;
        }
    };

    template <>
    struct equal_to<Vector3>
    {
        bool operator()(const Vector3& lhs, const Vector3& rhs) const {
            return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
        }
    };

} // namespace std
