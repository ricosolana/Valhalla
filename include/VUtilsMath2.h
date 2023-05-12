#pragma once

#include "Vector.h"

namespace VUtils::Math {

    // Fractional brownian noise
    float Fbm(Vector3f p, int octaves, float lacunarity, float gain);

    // Fractional brownian noise
    float FbmMaxValue(int octaves, float gain);

    // Fractional brownian noise
    float Fbm(Vector2f p, int octaves, float lacunarity, float gain);

    float YawFromDirection(Vector3f dir);

    // Wrap degrees
    float FixDegAngle(float p_Angle);

}
