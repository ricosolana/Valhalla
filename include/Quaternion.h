#pragma once

#include "Vector.h"

struct Quaternion {
    static const Quaternion IDENTITY;

    float x, y, z, w;

    Quaternion();

    Quaternion(float x, float y, float z, float w);

    Quaternion(const Vector3& v, float w);

    Quaternion(const Quaternion& other) 
        : x(other.x), y(other.y), z(other.z), w(other.w) {

    }

    float LengthSquared() const;
    Vector3 xyz() const;

    Vector3 operator*(const Vector3& other) const;
    Quaternion operator*(const Quaternion& rhs) const;

    void operator*=(const Quaternion& rhs);

    bool operator==(const Quaternion& other) const;
    bool operator!=(const Quaternion& other) const;

    // Returns a Quaternion rotation accepting degrees in z -> x -> y (applied in order)
    static Quaternion Euler(float x, float y, float z);
    static Quaternion LookRotation(Vector3 forward, Vector3 upwards = Vector3::UP);
    static Quaternion Inverse(const Quaternion& rotation);
};

std::ostream& operator<<(std::ostream& st, Quaternion& quat);
