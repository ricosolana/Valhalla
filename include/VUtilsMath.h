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



    template<typename T>
    T Clamp(T value, T min, T max) {
        return std::min(std::max(value, min), max);
    }

    template<typename T>
    T Clamp01(T value) {
        return Clamp(value, (T)0, (T)1);
    }



    template<typename T>
    T LerpStep(T l, T h, T v) {
        return Clamp01((v - l) / (h - l));
    }

    template<typename T>
    T SmoothStep(T p_Min, T p_Max, T p_X) {
        auto num = Clamp01((p_X - p_Min) / (p_Max - p_Min));
        return num * num * ((T)3 - (T)2 * num);
    }

    template<typename T>
    T LerpStep(T l, T h, T v) {
        return Clamp01((v - l) / (h - l));
    }

    template<typename T>
    T Lerp(T a, T b, T t) {
        return a + (b - a) * Clamp01(t);
    }



    // Fast inverse square root
    float FISQRT(float n);

    // Perlin noise
    float PerlinNoise(float x, float y);

    bool Between(float i, float a, float b);
}
