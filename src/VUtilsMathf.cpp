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

    float Round(float f)
    {
        const float r = round(f); // Result is round-half-away-from-zero
        const float d = r - f; // Difference

        // Result is not half, RHAFZ result same as RHTE
        if ((d != 0.5f) && (d != -0.5f))
        {
            return r;
        }

        // Check if RHAFZ result is even, then RHAFZ result same as RHTE
        if (fmod(r, 2.0f) == 0.0f)
        {
            return r;
        }

        // Switch to even value
        return f - d;
    }

}