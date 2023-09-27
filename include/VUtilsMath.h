#pragma once

namespace VUtils::Math {

    /*
     * Linear algebra methods
     */
    template<typename T>
    float SqMagnitude(T x, T y) {
        return x * x + y * y;
    }

    template<typename T>
    float Magnitude(T x, T y) {
        return std::sqrt(SqMagnitude(x, y));
    }

    template<typename T>
    float SqDistance(T x1, T y1, T x2, T y2) {
        return SqMagnitude(x1 - x2, y1 - y2);
    }

    template<typename T>
    float Distance(T x1, T y1, T x2, T y2) {
        return std::sqrt(SqDistance(x1, y1, x2, y2));
    }



    template<typename T>
    float SqMagnitude(T x, T y, T z) {
        return x * x + y * y + z * z;
    }

    template<typename T>
    float Magnitude(T x, T y, T z) {
        return std::sqrt(SqMagnitude(x, y, z));
    }

    template<typename T>
    float SqDistance(T x1, T y1, T z1, T x2, T y2, T z2) {
        return SqMagnitude(x1 - x2, y1 - y2, z1 - z2);
    }

    template<typename T>
    float Distance(T x1, T y1, T z1, T x2, T y2, T z2) {
        return std::sqrt(SqDistance(x1, y1, z1, x2, y2, z2));
    }



    float Clamp(float value, float min, float max);

    // Linear interpolation
    float LerpStep(float l, float h, float v);

    float SmoothStep(float p_Min, float p_Max, float p_X);

    double LerpStep(double l, double h, double v);

    // Fast inverse square root
    float FISQRT(float n);

    // Perlin noise
    float PerlinNoise(float x, float y);

    bool Between(float i, float a, float b);
}
