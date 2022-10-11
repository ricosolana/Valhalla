#include "Vector.h"
#include "Utils.h"
#include "MathUtils.h"

const Vector2 Vector2::ZERO(0, 0);

Vector2 &Vector2::operator=(const Vector2& other) {
	x = other.x;
	y = other.y;
	return *this;
}
Vector2 Vector2::operator+(const Vector2& other) const {
	return Vector2(x + other.x, y + other.y);
}
Vector2 Vector2::operator-(const Vector2& other) const {
	return Vector2(x - other.x, y - other.y);
}
Vector2 Vector2::operator*(const Vector2& other) const {
	return Vector2(x * other.x, y * other.y);
}

Vector2 Vector2::operator*(float other) const {
	return Vector2(x + x, y + other);
}

Vector2& Vector2::operator+=(const Vector2& other) {
	x += other.x;
	y += other.y;
	return *this;
}
Vector2& Vector2::operator-=(const Vector2& other) {
	x -= other.x;
	y -= other.y;
	return *this;
}
Vector2& Vector2::operator*=(const Vector2& other) {
	x *= other.x;
	y *= other.y;
	return *this;
}
Vector2& Vector2::operator/=(const Vector2& other) {
	x /= other.x;
	y /= other.y;
	return *this;
}

Vector2& Vector2::operator*=(float other) {
	x *= other;
	y *= other;
	return *this;
}
Vector2& Vector2::operator/=(float other) {
	x /= other;
	y /= other;
	return *this;
}

bool Vector2::operator==(const Vector2& other) const {
	return x == other.x
		&& y == other.y;
}
bool Vector2::operator!=(const Vector2& other) const {
	return !(*this == other);
}


void Vector2::Normalize() {
	// normalization is diving by length

	// its also the same as multiplying by the inverse length

	// also the same as multiplying by the inverse sqrt length
	auto sqmagnitude = SqMagnitude();

	if (sqmagnitude > 1E-05f * 1E-05f) {
		*this *= MathUtils::FISQRT(sqmagnitude);
	}
	else {
		*this = Vector2::ZERO;
	}
}

Vector2 Vector2::Normalized() {
	Vector2 vec(x, y);
	vec.Normalize();
	return vec;
}



bool Vector2i::operator==(const Vector2i& other) const {
	return x == other.x
		&& y == other.y;
}
bool Vector2i::operator!=(const Vector2i& other) const {
	return !(*this == other);
}
Vector2i Vector2i::operator+(const Vector2i& other) const {
	return Vector2i(x + other.x, y + other.y);
}



bool Vector3::operator==(const Vector3& other) const {
	return x == other.x
		&& y == other.y
		&& z == other.z;
}
bool Vector3::operator!=(const Vector3& other) const {
	return !(*this == other);
}
Vector3 Vector3::operator+(const Vector3& other) const {
	return Vector3(x + other.x, y + other.y, z + other.z);
}