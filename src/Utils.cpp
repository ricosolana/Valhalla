#include "Utils.hpp"

namespace Utils {
    int GetStableHashCode(const char* str) {
        int num = 5381;
        int num2 = num;
        int num3 = 0;
        while (str[num3] != '\0')
        {
            num = ((num << 5) + num) ^ (int)str[num3];
            if (str[num3 + 1] == '\0')
            {
                break;
            }
            num2 = ((num2 << 5) + num2) ^ (int)str[num3 + 1];
            num3 += 2;
        }
        return num + num2 * 1566083941;
    }

    int GetUnicode8Count(const char* p) {
        int count = 0;
        for (p; *p != 0; ++p)
            count += ((*p & 0xc0) != 0x80);

        return count;
    }

    UID StringToUID(std::string_view sv) {
        std::string s(sv);
        std::stringstream ss(s);
        UID uid;
        ss >> uid;
        return uid;
    }

    bool IsAddress(std::string_view s) {
        //address make_address(const char* str,
        //    asio::error_code & ec) ASIO_NOEXCEPT
        asio::error_code ec;
        asio::ip::make_address(s, ec);
        return ec ? false : true;
    }

    std::string Join(std::vector<std::string_view>& strings) {
        std::string result;
        for (auto& s : strings) {
            result += s;
        }
        return result;
    }

    std::vector<std::string_view> Split(std::string_view s, char ch) {
        //std::string s = "scott>=tiger>=mushroom";

        // split in Java appears to be a recursive decay function (for the pattern)

        int off = 0;
        int next = 0;
        std::vector<std::string_view> list;
        while ((next = s.find(ch, off)) != std::string::npos) {
            list.push_back(s.substr(off, next));
            off = next + 1;
        }
        // If no match was found, return this
        if (off == 0)
            return { s };

        // Add remaining segment
        list.push_back(s.substr(off));

        // Construct result
        int resultSize = list.size();
        while (resultSize > 0 && list[resultSize - 1].empty()) {
            resultSize--;
        }

        return std::vector<std::string_view>(list.begin(), list.begin() + resultSize);






        //std::vector<std::string_view> res;
        //
        //size_t pos = 0;
        //while ((pos = s.find(delimiter)) != std::string::npos) {
        //    //std::cout << token << std::endl;
        //    res.push_back(s.substr(0, pos));
        //    //s.erase(0, pos + delimiter.length());
        //    if (pos + delimiter.length() == s.length())
        //        s = s.substr(pos + delimiter.length());
        //    else s = s.substr(pos + delimiter.length());
        //}
        //if (res.empty())
        //    res.push_back(s);
        ////std::cout << s << std::endl;
        //return res;
    }
}
