#include <limits>
#include <random>
#include <stdexcept>
#include <unistd.h>
#include <zlib.h>

#include "VUtils.h"
#include "VUtilsMath.h"
#include "VUtilsMathf.h"

//const Color Color::BLACK = Color();
//const Color Color::RED = Color(1, 0, 0);
//const Color Color::GREEN = Color(0, 1, 0);
//const Color Color::BLUE = Color(0, 0, 1);

namespace VUtils {

    bool SetEnv(std::string_view key, std::string_view value)
    {
        (void) key;
        (void) value;
        //return setenv((key.data(), value.data()) == 0;
        throw std::runtime_error("SetEnv nyi");
        //return putenv((key.data() + std::string("=") + value.data()).c_str()) == 0;
    }

    std::string GetEnv(std::string_view key)
    {
        //environ
        auto &&env = getenv(key.data());
        if (env)
            return env;
        return "";
    }
}// namespace VUtils

namespace avledet::util {


}
