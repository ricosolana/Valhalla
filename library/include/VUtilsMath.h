#pragma once

namespace VUtils::Math {

    /*
     * Linear algebra methods
     */

    float sq_magnitude(float x, float y);

    double sq_magnitude(double x, double y);

    float magnitude(float x, float y);

    double magnitude(double x, double y);

    float sq_distance_to(float x1, float y1, float x2, float y2);

    float distance_to(float x1, float y1, float x2, float y2);

    float sq_magnitude(float x, float y, float z);

    float magnitude(float x, float y, float z);

    float sq_distance_to(float x1, float y1, float z1, float x2, float y2, float z2);

    float distance_to(float x1, float y1, float z1, float x2, float y2, float z2);



    float Lerp(float a, float b, float t);

    double Lerp(double a, double b, double t);

    float Clamp(float value, float min, float max);

    // Linear interpolation
    float LerpStep(float l, float h, float v);

    double LerpStep(double l, double h, double v);

    float SmoothStep(float p_Min, float p_Max, float p_X);

    double MathfLikeSmoothStep(double from, double to, double t);

    double Clamp01(double value);

    // Fast inverse square root
    float FISQRT(float n);

    // Perlin noise
    float PerlinNoise(float x, float y);

    //float PerlinNoise(double x, double y);

    bool Between(float i, float a, float b);
}// namespace VUtils::Math
