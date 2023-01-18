#pragma once

struct Quaternion {
    static const Quaternion IDENTITY;

    float x, y, z, w;

    Quaternion(float x, float y, float z, float w);

    bool operator==(const Quaternion& other) const;
    bool operator!=(const Quaternion& other) const;

    static Quaternion Euler(float x, float y, float z);
    static Quaternion LookRotation(Vector3 forward, Vector3 upwards = Vector3::UP);
};
