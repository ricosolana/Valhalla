#include "Vector3.h"

bool Vector3::operator==(const Vector3& other) const {
	return x == other.x
		&& y == other.y
		&& z == other.z;
}
bool Vector3::operator!=(const Vector3& other) const {
	return !(*this == other);
}
