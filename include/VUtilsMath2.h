#pragma once

#include "Vector.h"

namespace VUtils::Math {

    // Fractional brownian noise
    float Fbm(const Vector3f& p, int octaves, float lacunarity, float gain);

    // Fractional brownian noise
    float FbmMaxValue(int octaves, float gain);

    // Fractional brownian noise
    float Fbm(const Vector2f& p, int octaves, float lacunarity, float gain);

    float YawFromDirection(const Vector3f& dir);

    // Wrap degrees
    float FixDegAngle(float p_Angle);

}
