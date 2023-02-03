#include <cmath>
#include "VUtilsMathf.h"
#include "VUtils.h"

namespace VUtils::Mathf {

    float Clamp01(float value) {
        if (value < 0.f)
            return 0;
        else if (value > 1.f)
            return 1;
        return value;
    }

    float SmoothStep(float from, float to, float t) {
        t = Clamp01(t);
        t = -2.f * t * t * t + 3.f * t * t;
        return to * t + from * (1.f - t);
    }

    float Lerp(float a, float b, float t) {
        return a + (b - a) * Mathf::Clamp01(t);
    }
}