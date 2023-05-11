#include <random>
#include <limits>
#include <zlib.h>

#include "VUtils.h"
#include "VUtilsMath.h"
#include "VUtilsMathf.h"

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

//const Color Color::BLACK = Color();
//const Color Color::RED = Color(1, 0, 0);
//const Color Color::GREEN = Color(0, 1, 0);
//const Color Color::BLUE = Color(0, 0, 1);

std::ostream& operator<<(std::ostream& st, const UInt64Wrapper& val) {
    return st << (uint64_t)val;
}

std::ostream& operator<<(std::ostream& st, const Int64Wrapper& val) {
    return st << (int64_t)val;
}

namespace VUtils {
    bool SetEnv(const std::string &key, const std::string &value) {
        return putenv((key + "=" + value).c_str()) == 0;
    }

    std::string GetEnv(const std::string& key) {
        auto&& env = getenv(key.c_str());
        if (env) return env;
        return "";
    }
}
