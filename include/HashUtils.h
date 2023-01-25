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
            return std::hash<int64_t>{}((static_cast<int64_t>(vec.x) << 32)
                | (static_cast<int64_t>(vec.y)));

            //std::size_t hash1 = std::hash<decltype(Vector2i::x)>()(vec.x);
            //std::size_t hash2 = std::hash<decltype(Vector2i::y)>()(vec.y);

            //return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
        }
    };

    template <>
    struct hash<Vector3>
    {
        std::size_t operator()(const Vector3& vec) const {

            return std::hash<float>{}(vec.x) ^ (std::hash<float>{}(vec.y) << 8) ^ (std::hash<float>{}(vec.z) << 16);
        }
    };

} // namespace std
