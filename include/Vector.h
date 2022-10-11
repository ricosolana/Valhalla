#pragma once

#include <stdint.h>
#include <cmath>
#include "MathUtils.h"

struct Vector2 {
	float x = 0, y = 0;

	Vector2(float x = 0, float y = 0) 
		: x(x), y(y) {}

	Vector2(const Vector2& other) = default; //copy
	Vector2(Vector2&& other) = default; // move


	Vector2 &operator=(const Vector2& other);
	Vector2 operator+(const Vector2& other) const;
	Vector2 operator-(const Vector2& other) const;
	Vector2 operator*(const Vector2& other) const;
	Vector2 operator/(const Vector2& other) const;
	

	Vector2 operator*(float other) const;

	Vector2& operator+=(const Vector2& other);
	Vector2& operator-=(const Vector2& other);
	Vector2& operator*=(const Vector2& other);
	Vector2& operator/=(const Vector2& other);

	Vector2& operator*=(float other);
	Vector2& operator/=(float other);

	bool operator==(const Vector2& other) const;
	bool operator!=(const Vector2& other) const;



	float SqMagnitude() const {
		return MathUtils::SqMagnitude(x, y);
	}

	float Magnitude() const {
		return MathUtils::Magnitude(x, y);
	}

	float Distance(const Vector2& other) const {
		return MathUtils::Distance(x, y, other.x, other.y);
	}

	float SqDistance(const Vector2& other) const {
		return MathUtils::SqDistance(x, y, other.x, other.y);
	}

	void Normalize();
	Vector2 Normalized();

	static const Vector2 ZERO;
};

struct Vector2i {
	int32_t x = 0, y = 0;

	Vector2i(int32_t x = 0, int32_t y = 0)
		: x(x), y(y) {}

	bool operator==(const Vector2i& other) const;
	bool operator!=(const Vector2i& other) const;
	Vector2i operator+(const Vector2i& other) const;

	float Magnitude() {
		return Magnitude(x, y);
	}

	static float Magnitude(float x, float y) {
		return sqrt(x * x + y * y);
	}
};

struct Vector3 {
	float x = 0, y = 0, z = 0;

	Vector3(float x = 0, float y = 0, float z = 0)
		: x(x), y(y), z(z) {}

	bool operator==(const Vector3& other) const;
	bool operator!=(const Vector3& other) const;
	Vector3 operator+(const Vector3& other) const;

	float Magnitude() {
		return Magnitude(x, y, z);
	}

	static float Magnitude(float x, float y, float z) {
		return sqrt(x * x + y * y + z * z);
	}
};
