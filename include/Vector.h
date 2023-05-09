#pragma once

#include "VUtils.h"

#include <stdint.h>
#include <cmath>
#include <ostream>

#include "VUtilsMath.h"

template<typename T> requires std::is_arithmetic_v<T>
struct Vector2 {
    T x, y;

    constexpr Vector2() : x(0), y(0) {}
    constexpr Vector2(const T x, const T y) : x(x), y(y) {}
        
    // vector arithmetic
    void operator=(const Vector2<T>& other) {
        this->x = other.x;
        this->y = other.y;
    }



    Vector2<T> operator+(const Vector2<T>& rhs) const {
        return Vector2<T>(this->x + rhs.x, this->y + rhs.y);
    }

    Vector2<T> operator-(const Vector2<T>& rhs) const {
        return Vector2<T>(this->x - rhs.x, this->y - rhs.y);
    }

    Vector2<T> operator-() const {
        return Vector2<T>(-this->x, -this->y);
    }

    Vector2<T> operator*(const Vector2<T>& rhs) const {
        return Vector2<T>(this->x * rhs.x, this->y * rhs.y);
    }

    Vector2<T> operator/(const Vector2<T>& rhs) const {
        return Vector2<T>(this->x / rhs.x, this->y / rhs.y);
    }
    
    Vector2<T> operator*(const float rhs) const {
        return Vector2<T>(this->x * rhs, this->y * rhs);
    }

    Vector2<T> operator/(const float rhs) const {
        return Vector2<T>(this->x / rhs, this->y / rhs);
    }



    void operator+=(const Vector2<T>& rhs) {
        *this = *this + rhs;
    }

    void operator-=(const Vector2<T>& rhs) {
        *this = *this - rhs;
    }

    void operator*=(const Vector2<T>& rhs) {
        *this = *this * rhs;
    }

    void operator/=(const Vector2<T>& rhs) {
        *this = *this / rhs;
    }

    void operator*=(const float rhs) {
        *this = *this * rhs;
    }

    void operator/=(const float rhs) {
        *this = *this / rhs;
    }



    constexpr bool operator==(const Vector2<T>& rhs) const {
        return this->x == rhs.x && this->y == rhs.y;
    }

    constexpr bool operator!=(const Vector2<T>& rhs) const {
        return !(*this == rhs);
    }



    constexpr float Dot(const Vector2<T>& rhs) const {
        return (float)this->x * rhs.x + (float)this->y * rhs.y;
    }

    constexpr float SqMagnitude() const {
        return (float)this->x * this->x + (float)this->y * this->y;
    }

    constexpr float Magnitude() const {
        return std::sqrt(this->SqMagnitude());
    }
    
    constexpr float SqDistance(const Vector2<T>& rhs) const {
        return (this->x - rhs.x) * (this->x - rhs.x) + (this->y - rhs.y) * (this->y - rhs.y);
    }

    constexpr float Distance(const Vector2<T>& rhs) const {
        return std::sqrt(this->SqDistance(rhs));
    }

    constexpr Vector2<T> Normal() const {
        auto sqmagnitude = this->SqMagnitude();

        if (sqmagnitude > 1E-05f * 1E-05f) {
            //return *this / std::sqrt(sqmagnitude);
            return *this * VUtils::Math::FISQRT(sqmagnitude);
        }
        else {
            return Zero();
        }
    }



    static constexpr Vector2<T> Zero() {
        return Vector2<T>(0, 0);
    }
};

using Vector2f = Vector2<float>;
using Vector2i = Vector2<int32_t>;

std::ostream& operator<<(std::ostream& st, const Vector2f& vec);
std::ostream& operator<<(std::ostream& st, const Vector2i& vec);


template<typename T> requires std::is_arithmetic_v<T>
struct Vector3 {
    T x, y, z;

    constexpr Vector3() : x(0), y(0), z(0) {}
    constexpr Vector3(const T x, const T y, const T z) 
        : x(x), y(y), z(z) {}

    // vector arithmetic
    void operator=(const Vector3<T>& other) {
        this->x = other.x;
        this->y = other.y;
        this->z = other.z;
    }



    Vector3<T> operator+(const Vector3<T>& rhs) const {
        return Vector3<T>(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
    }

    Vector3<T> operator-(const Vector3<T>& rhs) const {
        return Vector3<T>(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
    }

    Vector3<T> operator-() const {
        return Vector3<T>(-this->x, -this->y, -this->z);
    }

    Vector3<T> operator*(const Vector3<T>& rhs) const {
        return Vector3<T>(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
    }

    Vector3<T> operator/(const Vector3<T>& rhs) const {
        return Vector3<T>(this->x / rhs.x, this->y / rhs.y, this->z / rhs.z);
    }

    Vector3<T> operator*(const float rhs) const {
        return Vector3<T>(this->x * rhs, this->y * rhs, this->z * rhs);
    }

    Vector3<T> operator/(const float rhs) const {
        return Vector3<T>(this->x / rhs, this->y / rhs, this->z / rhs);
    }



    void operator+=(const Vector3<T>& rhs) {
        *this = *this + rhs;
    }

    void operator-=(const Vector3<T>& rhs) {
        *this = *this - rhs;
    }

    void operator*=(const Vector3<T>& rhs) {
        *this = *this * rhs;
    }

    void operator/=(const Vector3<T>& rhs) {
        *this = *this / rhs;
    }

    void operator*=(const float rhs) {
        *this = *this * rhs;
    }

    void operator/=(const float rhs) {
        *this = *this / rhs;
    }


    
    constexpr bool operator==(const Vector3<T>& rhs) const {
        return this->x == rhs.x && this->y == rhs.y && this->z == rhs.z;
    }

    constexpr bool operator!=(const Vector3<T>& rhs) const {
        return !(*this == rhs);
    }



    constexpr Vector3<T> Cross(const Vector3<T>& rhs) const {
        return Vector3<T>(
            this->y * rhs.z - this->z * rhs.y,
            this->z * rhs.x - this->x * rhs.z,
            this->x * rhs.y - this->y * rhs.x
        );
    }

    constexpr float Dot(const Vector3<T>& rhs) const {
        return this->x * rhs.x 
            + this->y * rhs.y 
            + this->z * rhs.z;
    }

    constexpr float SqMagnitude() const {
        return this->x * this->x 
            + this->y * this->y 
            + this->z * this->z;
    }

    constexpr float Magnitude() const {
        return std::sqrt(this->SqMagnitude());
    }

    constexpr float SqDistance(const Vector3<T>& rhs) const {
        return (this->x - rhs.x) * (this->x - rhs.x) 
            + (this->y - rhs.y) * (this->y - rhs.y) 
            + (this->z - rhs.z) * (this->z - rhs.z);
    }

    constexpr float Distance(const Vector3<T>& rhs) const {
        return std::sqrt(this->SqDistance(rhs));
    }

    constexpr Vector3<T> Normal() const {
        auto sqmagnitude = this->SqMagnitude();

        if (sqmagnitude > 1E-05f * 1E-05f) {
            //return *this / std::sqrt(sqmagnitude);
            return *this * VUtils::Math::FISQRT(sqmagnitude);
        }
        else {
            return Zero();
        }
    }



    static constexpr Vector3<T> Zero() {
        return Vector3<T>(0, 0, 0);
    }

    static constexpr Vector3<T> Up() {
        return Vector3<T>(0, 1, 0);
    }

    static constexpr Vector3<T> Down() {
        return Vector3<T>(0, -1, 0);
    }

    static constexpr Vector3<T> Forward() {
        return Vector3<T>(0, 0, 1);
    }
};

using Vector3f = Vector3<float>;

std::ostream& operator<<(std::ostream& st, const Vector3f& vec);
