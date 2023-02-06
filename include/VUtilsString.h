#pragma once

#include "VUtils.h"
#include "VUtilsTraits.h"

#define __H(str) VUtils::String::GetStableHashCode(str)

namespace VUtils::String {
    constexpr HASH_t GetStableHashCode(const char *str, uint32_t num, uint32_t num2, uint32_t idx) { // NOLINT(misc-no-recursion)
        if (str[idx] != '\0') {
            num = ((num << 5) + num) ^ (unsigned) str[idx];
            if (str[idx + 1] != '\0') {
                num2 = ((num2 << 5) + num2) ^ (unsigned) str[idx + 1];
                idx += 2;
                return GetStableHashCode(str, num, num2, idx);
            }
        }
        return static_cast<HASH_t>(num + num2 * 1566083941);
    }

    constexpr HASH_t GetStableHashCode(const char *str) {
        uint32_t num = 5381;
        uint32_t num2 = num;
        uint32_t idx = 0;

        return GetStableHashCode(str, num, num2, idx);
    }

    // Join a container consisting of strings separated by delimiter
    template<typename T> requires VUtils::Traits::is_iterable_v<T>
    std::string Join(const char* delimiter, T container) {
        std::string result;
        for (int i = 0; i < container.size() - 1; i++) {
            result += std::string(*(container.begin() + i)) + delimiter;
        }
        result += *(container.end() - 1);
        return result;
    }

    // Join a container consisting of strings separated by delimiter
    template<typename T> requires VUtils::Traits::is_iterable_v<T>
    std::string Join(std::string &delimiter, T container) {
        return Join(delimiter.c_str(), container);
    }

    HASH_t GetStableHashCode(const std::string &s);

    HASH_t GetStableHashCode(const std::string_view& s);

    std::vector<std::string_view> Split(std::string_view s, const std::string &delim);

    // C# Encoding.ASCII.GetString equivalent:
    // bytes greater than 127 get turned to literal '?' (63)
    // Returns whether any modification was done
    bool FormatAscii(std::string &in);

    // C# Encoding.ASCII.GetString equivalent:
    // bytes greater than 127 get turned to literal '?' (63)
    // Returns a transformed string
    std::string ToAscii(const std::string& in);

    // Gets the unicode code points in a UTF-8 encoded string
    // Return -1 on bad encoding
    int GetUTF8CodeCount(const BYTE_t *p);

    // Gets the unicode byte count needed to encode uint16_t or C# char 
    //  Returns 1, 2 or 3
    unsigned int GetUTF8ByteCount(uint16_t i);
}