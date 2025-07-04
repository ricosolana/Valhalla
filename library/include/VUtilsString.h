#pragma once

#include "VUtils.h"
#include "VUtilsTraits.h"

namespace avledet::lexicon {

    int levenshtein_distance(std::string_view s, std::string_view t);

    std::vector<std::string_view> split(std::string_view s, std::string_view delim);

    std::string to_lower(std::string_view in);

    // Join a container consisting of strings separated by delimiter
    template<typename T>
        requires VUtils::Traits::is_iterable<T>
    std::string join(std::string_view delimiter, T container)
    {
        std::string result;
        for (int i = 0; i < container.size() - 1; i++) {
            result += std::string(*(container.begin() + i)) + std::string(delimiter);
        }
        result += *(container.end() - 1);
        return result;
    }

    template<typename Iterable = std::vector<std::string_view>>
        requires(VUtils::Traits::is_iterable<Iterable>)
    Iterable split(std::string_view s, char delim, bool includeBlanks = false)
    {
        std::int64_t size = s.size();
        auto data         = s.data();

        Iterable split {};

        std::int64_t lineIdx  = -1;
        std::int64_t lineSize = 0;
        for (decltype(size) i = 0; i < size; i++) {
            lineSize = i - lineIdx - 1;

            if (data[i] == delim) {
                if (lineSize || includeBlanks) {
                    split.insert(split.end(), typename Iterable::value_type(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            split.insert(split.end(), typename Iterable::value_type(data + lineIdx + 1, size - lineIdx - 1));
        }

        return split;
    }

    namespace CSU {

        // C# Encoding.ASCII.GetString equivalent:
        // bytes greater than 127 get turned to literal '?' (63)
        // Returns whether any modification was done
        //std::string ascii(std::string in);

        // C# Encoding.ASCII.GetString equivalent:
        // bytes greater than 127 get turned to literal '?' (63)
        // Returns whether any modification was done
        //bool FormatAscii(char* in, std::size_t inSize);

        // C# Encoding.ASCII.GetString equivalent:
        // bytes greater than 127 get turned to literal '?' (63)
        // Returns a transformed string
        std::string ascii(std::string_view in);

        // Gets the unicode code points in a UTF-8 encoded string
        // Return -1 on bad encoding
        int get_utf8_code_count(avledet::util::Byte const *p);

        // Gets the unicode byte count needed to encode std::uint16_t or C# char
        //  Returns 1, 2 or 3
        unsigned int get_utf8_byte_count(std::uint16_t i);

    }// namespace CSU

}// namespace avledet::lexicon
