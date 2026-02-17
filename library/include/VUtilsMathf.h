#pragma once

// TODO rename under CSU
namespace VUtils::Mathf {

    float Clamp01(float value);

    float SmoothStep(float from, float to, float t);

    //float Lerp(float a, float b, float t);

    // Bankers rounding
    float Round(float f);

}// namespace VUtils::Mathf

namespace avledet::util { namespace CSU {

    // return Mathf.Abs(b - a) < Mathf.Max(1E-06f * Mathf.Max(Mathf.Abs(a), Mathf.Abs(b)), Mathf.Epsilon * 8f);
    bool equal(float, float);

}}// namespace avledet::util::CSU
