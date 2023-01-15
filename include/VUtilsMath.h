#pragma once

struct Vector2;
struct Vector3;

namespace VUtils::Math {

    /*
     * Linear algebra methods
     */

    float SqMagnitude(float x, float y);

    float Magnitude(float x, float y);

    float SqDistance(float x1, float y1, float x2, float y2);

    float Distance(float x1, float y1, float x2, float y2);


    float SqMagnitude(float x, float y, float z);

    float Magnitude(float x, float y, float z);

    float SqDistance(float x1, float y1, float z1, float x2, float y2, float z2);

    float Distance(float x1, float y1, float z1, float x2, float y2, float z2);






    float Clamp(float value, float min, float max);

    float Clamp01(float value);

    float Lerp(float a, float b, float t);

    // Linear interpolation
    float LerpStep(float l, float h, float v);

    float SmoothStep(float p_Min, float p_Max, float p_X);

    double LerpStep(double l, double h, double v);



    // Fractional brownian noise
    float Fbm(const Vector3 &p, int octaves, float lacunarity, float gain);

    // Fractional brownian noise
    float FbmMaxValue(int octaves, float gain);

    // Fractional brownian noise
    float Fbm(const Vector2 &p, int octaves, float lacunarity, float gain);



    // fucky methods

    // Fast inverse square root
    float FISQRT(float n);



    // Perlin noise
    float PerlinNoise(float x, float y);
}
