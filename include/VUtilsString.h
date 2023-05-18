#pragma once

#include "VUtils.h"
#include "VUtilsTraits.h"

#define __H(str) VUtils::String::GetStableHashCodeCT(str)

namespace VUtils::String {
    // INTERNAL: Do not use unless you know what you are doing
    constexpr HASH_t GetStableHashCodeCT(const char *str, uint32_t num, uint32_t num2, uint32_t idx) { // NOLINT(misc-no-recursion)
        if (str[idx] != '\0') {
            num = ((num << 5) + num) ^ (uint32_t) str[idx];
            if (str[idx + 1] != '\0') {
                num2 = ((num2 << 5) + num2) ^ (uint32_t) str[idx + 1];
                idx += 2;
                return GetStableHashCodeCT(str, num, num2, idx);
            }
        }
        return static_cast<HASH_t>(num + num2 * 1566083941);
    }

    // Calculate the Valheim-hash of a string
    //  This is the compile time overload
    constexpr HASH_t GetStableHashCodeCT(const char *str) {
        uint32_t num = 5381;
        uint32_t num2 = num;
        uint32_t idx = 0;

        return GetStableHashCodeCT(str, num, num2, idx);
    }

    // Calculate the Valheim-hash of a string
    HASH_t GetStableHashCode(std::string_view s);


/*
    // Join a container consisting of strings separated by delimiter
    template<typename T> requires VUtils::Traits::is_iterable_v<T>
    std::string Join(std::string_view delimiter, T container) {
        std::string result;
        for (int i = 0; i < container.size() - 1; i++) {
            result += std::string(*(container.begin() + i)) + delimiter;
        }
        result += *(container.end() - 1);
        return result;
    }
    
    int LevenshteinDistance(std::string_view s, std::string_view t);*/

    std::vector<std::string_view> Split(std::string_view s, std::string_view delim);

    template<typename Iterable = std::vector<std::string_view>>
        requires (VUtils::Traits::is_iterable_v<Iterable>)
    Iterable Split(std::string_view s, char delim, bool includeBlanks = false) 
    {
        int64_t size = s.size();
        auto data = s.data();

        Iterable split{};

        int lineIdx = -1;
        int lineSize = 0;
        for (int i = 0; i < size; i++) {
            lineSize = i - lineIdx - 1;

            if (data[i] == delim) {
                if (lineSize || includeBlanks) {
                    split.insert(split.end(), Iterable::value_type(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            split.insert(split.end(), Iterable::value_type(data + lineIdx + 1, size - lineIdx - 1));
        }

        return split;
    }

    // C# Encoding.ASCII.GetString equivalent:
    // bytes greater than 127 get turned to literal '?' (63)
    // Returns whether any modification was done
    bool FormatAscii(std::string &in);

    // C# Encoding.ASCII.GetString equivalent:
    // bytes greater than 127 get turned to literal '?' (63)
    // Returns whether any modification was done
    bool FormatAscii(char* in, size_t inSize);

    // C# Encoding.ASCII.GetString equivalent:
    // bytes greater than 127 get turned to literal '?' (63)
    // Returns a transformed string
    std::string ToAscii(std::string_view in);

    // Gets the unicode code points in a UTF-8 encoded string
    // Return -1 on bad encoding
    int GetUTF8CodeCount(const BYTE_t *p);

    // Gets the unicode byte count needed to encode uint16_t or C# char 
    //  Returns 1, 2 or 3
    unsigned int GetUTF8ByteCount(uint16_t i);
}