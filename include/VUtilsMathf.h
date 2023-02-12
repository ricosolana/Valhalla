#pragma once

namespace VUtils::Mathf {

    float Clamp01(float value);

    float SmoothStep(float from, float to, float t);

    float Lerp(float a, float b, float t);

    // Bankers rounding
    float Round(float f);

}
