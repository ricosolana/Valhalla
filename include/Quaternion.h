#pragma once

#include "Vector.h"

struct Quaternion {
    static const Quaternion IDENTITY;

    float x, y, z, w;

    Quaternion();

    Quaternion(float x, float y, float z, float w);

    Quaternion(const Vector3f& v, float w);

    Quaternion(const Quaternion& other) 
        : x(other.x), y(other.y), z(other.z), w(other.w) {

    }

    float LengthSquared() const;
    Vector3f xyz() const;

    Vector3f operator*(const Vector3f& other) const;
    Quaternion operator*(const Quaternion& rhs) const;

    void operator*=(const Quaternion& rhs);

    bool operator==(const Quaternion& other) const;
    bool operator!=(const Quaternion& other) const;

    // Returns a Quaternion rotation accepting degrees in z -> x -> y (applied in order)
    static Quaternion Euler(float x, float y, float z);
    static Quaternion LookRotation(Vector3f forward, Vector3f upwards);
    static Quaternion LookRotation(Vector3f forward) {
        return LookRotation(forward, Vector3f::Up());
    }
    static Quaternion Inverse(const Quaternion& rotation);
};

std::ostream& operator<<(std::ostream& st, const Quaternion& quat);
