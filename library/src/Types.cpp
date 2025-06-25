#include "Types.h"
#include "ModManager.h"
#include "VUtilsMathf.h"
#include "ZDOID.h"

namespace avledet::util {

    Color Color::Lerp(Color const &other, float t)
    {
        //t = VUtils::Math::Clamp01(t);
        return Color(VUtils::Mathf::Lerp(r, other.r, t), VUtils::Mathf::Lerp(g, other.g, t),
                     VUtils::Mathf::Lerp(b, other.b, t), VUtils::Mathf::Lerp(a, other.a, t));
    }

    Color32 Color32::Lerp(Color32 const &other, float t)
    {
        return Color32((avledet::util::Byte) VUtils::Mathf::Lerp((float) r, (float) other.r, t),
                       (avledet::util::Byte) VUtils::Mathf::Lerp((float) g, (float) other.g, t),
                       (avledet::util::Byte) VUtils::Mathf::Lerp((float) b, (float) other.b, t),
                       (avledet::util::Byte) VUtils::Mathf::Lerp((float) a, (float) other.a, t));
    }

}// namespace avledet::util
