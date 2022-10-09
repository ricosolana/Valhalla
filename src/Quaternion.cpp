#include "Quaternion.h"

const Quaternion Quaternion::IDENTITY = { 0, 0, 0, 1 };

bool Quaternion::operator==(const Quaternion& other) const {
	return x == other.x
		&& y == other.y
		&& z == other.z
		&& w == other.w;
}
bool Quaternion::operator!=(const Quaternion & other) const {
	return !(*this == other);
}

// determine whether the 

// it might be premature optimizaiton to
// pay too much attention to zdois impl's



