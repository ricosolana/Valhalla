#pragma once

namespace VUtils::Math {

    /*
     * Linear algebra methods
     */

    float SqMagnitude(float x, float y);

    float Magnitude(float x, float y);

    float SqDistance(float x1, float y1, float x2, float y2);

    float get_distance(float x1, float y1, float x2, float y2);

    float SqMagnitude(float x, float y, float z);

    float Magnitude(float x, float y, float z);

    float SqDistance(float x1, float y1, float z1, float x2, float y2, float z2);

    float get_distance(float x1, float y1, float z1, float x2, float y2, float z2);



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
