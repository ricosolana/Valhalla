#pragma once

class Vector3;

struct Quaternion {
	static const Quaternion IDENTITY;

	float x, y, z, w;

	Quaternion();
	Quaternion(float x, float y, float z, float w);

	Vector3 operator*(const Vector3& other) const;

	bool operator==(const Quaternion& other) const;
	bool operator!=(const Quaternion& other) const;




	//undefined(*)[16]
		//Internal_FromEulerRad_Injected_Impl(undefined(*param_1)[16], float* param_2, undefined4 param_3);

	static Quaternion Euler(float x, float y, float z);

	static Quaternion LookRotation(Vector3 forward, Vector3 upwards = Vector3::UP);

};
