#include "Types.h"
#include "ModManager.h"
#include "VUtilsMathf.h"
#include "ZDOID.h"

namespace avledet::util {

    Color Color::Lerp(const Color& other, float t) {
        //t = VUtils::Math::Clamp01(t);
        return Color(
            VUtils::Mathf::Lerp(r, other.r, t),
            VUtils::Mathf::Lerp(g, other.g, t),
            VUtils::Mathf::Lerp(b, other.b, t),
            VUtils::Mathf::Lerp(a, other.a, t));
    }

    Color32 Color32::Lerp(const Color32 &other, float t) {
        return Color32(
            VUtils::Mathf::Lerp(r, other.r, t),
            VUtils::Mathf::Lerp(g, other.g, t),
            VUtils::Mathf::Lerp(b, other.b, t),
            VUtils::Mathf::Lerp(a, other.a, t));
    }

}// namespace avledet::util


