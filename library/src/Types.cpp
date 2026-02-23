#include "Types.h"
#include "ModManager.h"
#include "VUtilsMathf.h"
#include "ZDOID.h"

namespace avledet::util {

    Color Color::Lerp(Color const &a, Color const &b, float t)
    {
        //t = VUtils::Math::Clamp01(t);
        return Color(VUtils::Mathf::Lerp(a.r, b.r, t), VUtils::Mathf::Lerp(a.g, b.g, t),
                     VUtils::Mathf::Lerp(a.b, b.b, t), VUtils::Mathf::Lerp(a.a, b.a, t));
    }

    Color32 Color32::Lerp(Color32 const &a, Color32 const &b, float t)
    {
        return Color32((avledet::util::Byte) VUtils::Mathf::Lerp((float) a.r, (float) b.r, t),
                       (avledet::util::Byte) VUtils::Mathf::Lerp((float) a.g, (float) b.g, t),
                       (avledet::util::Byte) VUtils::Mathf::Lerp((float) a.b, (float) b.b, t),
                       (avledet::util::Byte) VUtils::Mathf::Lerp((float) a.a, (float) b.a, t));
    }

}// namespace avledet::util
