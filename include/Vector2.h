#pragma once

template<typename T>
struct Vector2 {
	T x = 0, y = 0;
	//bool operator==(const ZDOID& other) const;
	//bool operator!=(const ZDOID& other) const;
};

using Vector2i = Vector2<int32_t>;