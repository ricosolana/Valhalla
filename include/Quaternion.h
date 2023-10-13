#pragma once

#include "Vector.h"

// A custom completely managed implementation of UnityEngine.Quaternion
// https://gist.github.com/HelloKitty/91b7af87aac6796c3da9
struct Quaternion {
    static const Quaternion IDENTITY;

    float x, y, z, w;

    Quaternion();

    Quaternion(float x, float y, float z, float w);

    Quaternion(Vector3f v, float w);

    Quaternion(const Quaternion &other) 
        : x(other.x), y(other.y), z(other.z), w(other.w) {

    }

    float LengthSquared() const;
    Vector3f xyz() const;
    Vector3f EulerAngles() const;
    float Dot(Quaternion b) const;

    Vector3f operator*(Vector3f other) const;
    Quaternion operator*(Quaternion rhs) const;

    void operator*=(Quaternion rhs);

    bool operator==(Quaternion other) const;
    bool operator!=(Quaternion other) const;

    // Returns a Quaternion rotation accepting degrees in z -> x -> y (applied in order)
    static Quaternion Euler(float x, float y, float z);
    // Returns a Quaternion rotation accepting degrees in vector z -> x -> y (applied in order)
    static Quaternion Euler(Vector3f angles);
    static Quaternion LookRotation(Vector3f forward, Vector3f upwards);
    static Quaternion LookRotation(Vector3f forward) {
        return LookRotation(forward, Vector3f::Up());
    }
    static Quaternion Inverse(Quaternion rotation);

    static Vector3f Internal_ToEulerRad(Quaternion rotation);

    static Vector3f NormalizeAngles(Vector3f angles);
    static float NormalizeAngle(float angle);
};

std::ostream& operator<<(std::ostream& st, Quaternion quat);
