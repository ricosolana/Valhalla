#pragma once

#include <concepts>
#include "NetSync.h"
//struct NetSyncID;

// C# hash equivalencies
namespace HashUtils {
	struct Hasher {
		template<typename T>
		int operator()(const T& value)
			requires (sizeof(T) <= 4 && std::is_fundamental_v<T> && !std::same_as<T, float>)
		{
			return static_cast<int32_t>(value);
		}

		int operator()(const int64_t &value) const;
		int operator()(const float &value) const;
		int operator()(const double &value) const;
		int operator()(const NetSync::ID &value) const;
	};
}
