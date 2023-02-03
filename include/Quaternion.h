#pragma once

#include "Vector.h"

struct Quaternion {
    static const Quaternion IDENTITY;

    float x, y, z, w;

    Quaternion(float x, float y, float z, float w);

    Vector3 operator*(const Vector3& other) const;
    Quaternion operator*(const Quaternion& rhs) const;

    bool operator==(const Quaternion& other) const;
    bool operator!=(const Quaternion& other) const;

    // Returns a Quaternion rotation accepting degrees in z -> x -> y (applied in order)
    static Quaternion Euler(float x, float y, float z);
    static Quaternion LookRotation(Vector3 forward, Vector3 upwards = Vector3::UP);
};
