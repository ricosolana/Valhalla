#pragma once

struct Vector3 {
	float x = 0, y = 0, z = 0;

	bool operator==(const Vector3& other) const;
	bool operator!=(const Vector3& other) const;
};
