#pragma once

#include <stdint.h>
#include <cmath>
#include <ostream>

#include "VUtilsMath.h"

struct Vector2 {
    float x, y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) 
        : x(x), y(y) {}

    Vector2(const Vector2& other) = default; //copy
    Vector2(Vector2&& other) = default; // move

    // vector arithmetic
    Vector2 &operator=(const Vector2& other);
    Vector2 operator+(const Vector2& other) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2 operator*(const Vector2& other) const;
    Vector2 operator/(const Vector2& other) const;
    
    // scalar arithmetic
    Vector2 operator*(float other) const;

    Vector2& operator+=(const Vector2& other);
    Vector2& operator-=(const Vector2& other);
    Vector2& operator*=(const Vector2& other);
    Vector2& operator/=(const Vector2& other);

    Vector2& operator*=(float other);
    Vector2& operator/=(float other);

    bool operator==(const Vector2& other) const;
    bool operator!=(const Vector2& other) const;



    float SqMagnitude() const {
        return VUtils::Math::SqMagnitude(x, y);
    }

    float Magnitude() const {
        return VUtils::Math::Magnitude(x, y);
    }

    float Distance(const Vector2& other) const {
        return VUtils::Math::Distance(x, y, other.x, other.y);
    }

    float SqDistance(const Vector2& other) const {
        return VUtils::Math::SqDistance(x, y, other.x, other.y);
    }


    //normalize and returns this
    Vector2 &Normalize();

    //normalize and returns a copy
    Vector2 Normalized();

    static const Vector2 ZERO;
};

std::ostream& operator<<(std::ostream& st, Vector2& vec);

struct Vector2i {
    int32_t x, y;

    Vector2i() : x(0), y(0) {}
    Vector2i(int32_t x, int32_t y)
        : x(x), y(y) {}

    Vector2i(const Vector2i& other) = default; //copy
    Vector2i(Vector2i&& other) = default; // move


    Vector2i& operator=(const Vector2i& other);
    Vector2i operator+(const Vector2i& other) const;
    Vector2i operator-(const Vector2i& other) const;
    Vector2i operator*(const Vector2i& other) const;
    Vector2i operator/(const Vector2i& other) const;


    Vector2i operator*(float other) const;

    Vector2i& operator+=(const Vector2i& other);
    Vector2i& operator-=(const Vector2i& other);
    Vector2i& operator*=(const Vector2i& other);
    Vector2i& operator/=(const Vector2i& other);

    Vector2i& operator*=(float other);
    Vector2i& operator/=(float other);

    bool operator==(const Vector2i& other) const;
    bool operator!=(const Vector2i& other) const;



    float SqMagnitude() const {
        return VUtils::Math::SqMagnitude(x, y);
    }

    float Magnitude() const {
        return VUtils::Math::Magnitude(x, y);
    }

    float Distance(const Vector2i& other) const {
        return VUtils::Math::Distance(x, y, other.x, other.y);
    }

    float SqDistance(const Vector2i& other) const {
        return VUtils::Math::SqDistance(x, y, other.x, other.y);
    }

    Vector2i& Normalize();
    Vector2i Normalized();

    static const Vector2i ZERO;
};

std::ostream& operator<<(std::ostream& st, Vector2i& vec);

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z)
        : x(x), y(y), z(z) {}
    ///Vector3(int x, int y, int z)
    //    : x(x), y(y), z(z) {}


    Vector3(const Vector3& other) = default; //copy
    Vector3(Vector3&& other) = default; // move





    Vector3& operator=(const Vector3& other);
    Vector3 operator+(const Vector3& other) const;
    Vector3 operator-(const Vector3& other) const;
    Vector3 operator*(const Vector3& other) const;
    Vector3 operator/(const Vector3& other) const;


    Vector3 operator*(float other) const;

    Vector3& operator+=(const Vector3& other);
    Vector3& operator-=(const Vector3& other);
    Vector3& operator*=(const Vector3& other);
    Vector3& operator/=(const Vector3& other);

    Vector3& operator*=(float other);
    Vector3& operator/=(float other);

    bool operator==(const Vector3& other) const;
    bool operator!=(const Vector3& other) const;



    Vector3 Cross(const Vector3 &rhs) const {
        return Vector3(
            y * rhs.z - z * rhs.y, 
            z * rhs.x - x * rhs.z, 
            x * rhs.y - y * rhs.x
        );
    }

    float Dot(const Vector3& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }



    float SqMagnitude() const {
        return VUtils::Math::SqMagnitude(x, y, z);
    }

    float Magnitude() const {
        return VUtils::Math::Magnitude(x, y, z);
    }

    float Distance(const Vector3& other) const {
        return VUtils::Math::Distance(x, y, z, other.x, other.y, other.z);
    }

    float SqDistance(const Vector3& other) const {
        return VUtils::Math::SqDistance(x, y, z, other.x, other.y, other.z);
    }

    Vector3& Normalize();
    Vector3 Normalized();

    static const Vector3 ZERO;
    static const Vector3 UP;
    static const Vector3 DOWN;
    static const Vector3 FORWARD;
};

std::ostream& operator<<(std::ostream& st, Vector3& vec);
