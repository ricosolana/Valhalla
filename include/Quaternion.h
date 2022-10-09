#pragma once

struct Quaternion {
	static const Quaternion IDENTITY;

	float x, y, z, w;

	bool operator==(const Quaternion& other) const;
	bool operator!=(const Quaternion& other) const;

};
