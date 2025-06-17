#pragma once

#include <string>
#include <string_view>

namespace avledet::crypto {

    // Hashes an input string and returns a 128-bit string (16 characters)
    std::string md5(std::string_view in);

}// namespace avledet::crypto
