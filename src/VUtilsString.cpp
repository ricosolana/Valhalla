
#include "VUtils.h"
#include "VUtilsString.h"

namespace VUtils::String {

    HASH_t GetStableHashCode(std::string_view s) {
        uint32_t num = 5381;
        uint32_t num2 = num;

        for (auto&& itr = s.begin(); itr != s.end(); ) {
            num = ((num << 5) + num) ^ (uint32_t) * (itr++);
            if (itr == s.end()) {
                break;
            }
            else {
                num2 = ((num2 << 5) + num2) ^ ((uint32_t) * (itr++));
            }
        }
        return static_cast<HASH_t>(num + num2 * 1566083941);
    }

    std::vector<std::string_view> Split(std::string_view s, std::string_view delim) {
        std::string_view remaining(s);
        std::vector<std::string_view> result;
        int pos = 0;
        //ABC DE FGHI JK
        while ((pos = remaining.find(delim)) != std::string::npos) {
            // If the delim was not at idx 0, then add everything from 0 to the pos
            if (pos) result.push_back(remaining.substr(0, pos));
            // Trim everything before pos
            remaining = remaining.substr(pos + 1);
        }
        // add final match to list after delim
        if (!remaining.empty())
            result.push_back(remaining);
        return result;
    }
    
    // Java code ported from Apache commons-lang-3.12.0
    int LevenshteinDistance(std::string_view s, std::string_view t) {
        int n = s.length();
        int m = t.length();

        if (n == 0) {
            return m;
        }
        else if (m == 0) {
            return n;
        }

        if (n > m) {
            std::swap(s, t);
            std::swap(m, n);
        }

        std::vector<int> p;
        
        //final int[] p = new int[n + 1];
        // indexes into strings s and t
        int i {}; // iterates through s
        int j {}; // iterates through t
        int upper_left {};
        int upper {};

        char t_j {}; // jth character of t
        int cost {};

        for (i = 0; i <= n; i++) {
            //p[i] = i;
            p.push_back(i);
        }

        for (j = 1; j <= m; j++) {
            upper_left = p[0];
            //t_j = t.charAt(j - 1);
            t_j = t[j - 1];
            p[0] = j;

            for (i = 1; i <= n; i++) {
                upper = p[i];
                //cost = s.charAt(i - 1) == t_j ? 0 : 1;
                cost = s[i - 1] == t_j ? 0 : 1;
                // minimum of cell to the left+1, to the top+1, diagonally left and up +cost
                //p[i] = Math.min(Math.min(p[i - 1] + 1, p[i] + 1), upper_left + cost);
                p[i] = std::min(std::min(p[i - 1] + 1, p[i] + 1), upper_left + cost);
                upper_left = upper;
            }
        }

        return p[n];
    }
    


    bool FormatAscii(std::string& in) {
        bool modif = false;
        auto data = reinterpret_cast<BYTE_t*>(in.data());
        for (int i = 0; i < in.size(); i++) {
            if (static_cast<uint8_t>(data[i]) > 127U) {
                modif = true;
                data[i] = 63;
            }
        }
        return modif;
    }

    bool FormatAscii(char* in, size_t inSize) {
        bool modif = false;
        for (size_t i = 0; i < inSize; i++) {
            auto&& ch = in + i;
            if (*ch < 0) {
                *ch = 63;
                modif = true;
            }
        }
        return modif;
    }

    std::string ToAscii(std::string_view in) {
        std::string ret = std::string(in);
        FormatAscii(ret); 
        return ret;
    }

    // https://en.wikipedia.org/wiki/UTF-8#Encoding
    int GetUTF8CodeCount(const BYTE_t* p) {
        // leading bits:
        //   0: total 1 byte
        //   110: total 2 bytes (trailing 10xxxxxx)
        //   1110: total 3 bytes (trailing 10xxxxxx)
        //   11110: total 4 bytes (trailing 10xxxxxx)
        int32_t count = 0;
        for (; *p != '\0'; ++p, count++) {
#define CHECK_TRAILING_BYTES(n) \
        { \
            for (p++; /*next byte*/ \
                *p != '\0', i < (n); /*min bounds check*/ \
                ++p, ++i) /*increment*/ \
            { \
                if (((*p) >> 6) != 0b10) { \
                    return -1; \
                } \
            } \
            /* if string ended prematurely, panic */ \
            if (i != (n)) \
                return -1; \
        }

            // 1-byte code point
            if (((*p) >> 7) == 0b0) {
                continue;
            }
            else {
                int i = 0;
                // 2-byte code point
                if (((*p) >> 5) == 0b110) {
                    CHECK_TRAILING_BYTES(1);
                }
                    // 3-byte code point
                else if (((*p) >> 4) == 0b1110) {
                    CHECK_TRAILING_BYTES(2);
                }
                    // 4-byte code point
                else if (((*p) >> 3) == 0b11110) {
                    CHECK_TRAILING_BYTES(3);
                }
                else
                    return -1;
            }
        }
        return count;
    }

    unsigned int GetUTF8ByteCount(uint16_t i) {
        if (i < 0x80) {
            return 1;
        }
        else if (i < 0x0800) {
            return 2;
        }
        //else if (i < 0x010000) {
            return 3;
        //}
    }
}