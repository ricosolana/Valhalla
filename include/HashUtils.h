#pragma once

#include <concepts>
#include <type_traits>
#include "NetID.h"
#include "Vector.h"

namespace std {

	template <>
	struct hash<NetID>
	{
		std::size_t operator()(const NetID& id) noexcept {


			std::size_t hash1 = std::hash<decltype(NetID::m_id)>{}(id.m_id);
			std::size_t hash2 = std::hash<decltype(NetID::m_uuid)>{}(id.m_uuid);

			return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
		}
	};

	template <>
	struct hash<Vector2i>
	{
		std::size_t operator()(const Vector2i& vec) noexcept {
			return std::hash<int64_t>{}((static_cast<int64_t>(vec.x) << 32)
				| (static_cast<int64_t>(vec.y)));

			//std::size_t hash1 = std::hash<decltype(Vector2i::x)>()(vec.x);
			//std::size_t hash2 = std::hash<decltype(Vector2i::y)>()(vec.y);

			//return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
		}
	};

} // namespace std

// C# hash equivalencies
/*
namespace HashUtils {
	struct Hasher {
		//template<typename T>
		//int operator()(const T& value)
		//	requires (sizeof(T) <= 4 && std::is_fundamental_v<T> && !std::same_as<T, float>)
		//{
		//	return static_cast<int32_t>(value);
		//}
		//
		//int operator()(const int64_t &value) const;
		//int operator()(const float &value) const;
		//int operator()(const double &value) const;

		int operator()(const NetID &value) const;
		int operator()(const Vector2i& value) const;
	};
}
*/