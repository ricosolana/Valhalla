#pragma once

#include "VUtils.h"
#include "VUtilsTraits.h"

#define __H(str) VUtils::String::GetStableHashCodeCT(str)

template<typename T, size_t N, typename C = typename T::value_type>
    requires (N >= 1)
class ConcatPack {
    std::array<std::basic_string_view<C>, N> m_strings{};
    size_t m_length{};

public:
    using value_type = C;
    using traits_type = std::char_traits<C>;

public:
    template<typename C>
    struct Iterator {
        friend class ConcatPack;

    private:
        Iterator(const std::basic_string_view<C>* begin) {
            m_str = begin;
            m_ptr = begin->data();
            SkipEmpty();
        }

        // Some strings might be empty
        //  Skip those
        bool SkipEmpty() {
            bool skipped = false;
            while (m_ptr >= m_str->data() + m_str->length() - 1) {
                ++m_str;
                m_ptr = m_str->data();
                skipped = true;
            }
            return skipped;
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = C;
        using pointer = const C*;
        using reference = C;

        reference operator*() const { return *m_ptr; }
        pointer operator->() const { return m_ptr; }
        Iterator& operator++() {
            // If at end of string
            //  Increment and reset pos
            if (!SkipEmpty()) {
                ++m_ptr;
            }

            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_str == b.m_str && a.m_ptr == b.m_ptr; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); }

    private:
        const std::basic_string_view<C>* m_str;
        pointer m_ptr;
    };

public:
    ConcatPack(const std::array<T, N>& strings) : m_strings(strings) {
        for (auto&& s : m_strings) {
            m_length += s.length();
        }
    }

    bool empty() const {
        return m_length == 0;
    }

    size_t size() const {
        return m_length;
    }

    size_t length() const {
        return m_length;
    }

    Iterator<C> begin() const { return Iterator(m_strings.data()); }
    Iterator<C> end() const { return Iterator(m_strings.data() + m_strings.size()); }
};

template<typename T>
concept StringLike = 
       VUtils::Traits::has_traits_type_v<T> 
    && std::is_same_v<typename T::value_type, char>;

namespace VUtils::String {
    // Get the compile-time Valheim hash of the string
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

    // Get the compile-time Valheim hash of the string
    constexpr HASH_t GetStableHashCodeCT(const char *str) {
        uint32_t num = 5381;
        uint32_t num2 = num;
        uint32_t idx = 0;

        return GetStableHashCodeCT(str, num, num2, idx);
    }

    //template<typename Strings> requires
    //HASH_t GetStableHashCode()

    //HASH_t GetStableHashCode(std::string_view s);

    //template<std::size_t N>
    template<StringLike S>
    HASH_t GetStableHashCode(const S& s) {
        uint32_t num = 5381;
        uint32_t num2 = num;

        for (auto&& itr = s.begin(); itr != s.end(); ) {
            num = ((num << 5) + num) ^ (uint32_t)*(itr++);
            if (itr == s.end()) {
                break;
            }
            else {
                num2 = ((num2 << 5) + num2) ^ ((uint32_t) * (itr++));
            }                
        }
        return static_cast<HASH_t>(num + num2 * 1566083941);
    }

    /*
    constexpr HASH_t GetStableHashCode(std::string_view str, uint32_t num, uint32_t num2, uint32_t idx) { // NOLINT(misc-no-recursion)
        if (str[idx] != '\0') {
            num = ((num << 5) + num) ^ (unsigned)str[idx];
            if (str[idx + 1] != '\0') {
                num2 = ((num2 << 5) + num2) ^ (unsigned)str[idx + 1];
                idx += 2;
                return GetStableHashCode(str, num, num2, idx);
            }
        }
        return static_cast<HASH_t>(num + num2 * 1566083941);
    }

    constexpr HASH_t GetStableHashCode(std::string_view str) {
        uint32_t num = 5381;
        uint32_t num2 = num;
        uint32_t idx = 0;

        return GetStableHashCode(str, num, num2, idx);
    }*/

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

    

    int LevenshteinDistance(std::string_view s, std::string_view t);

    std::vector<std::string_view> Split(std::string_view s, const std::string &delim);

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
    // Returns a transformed string
    std::string ToAscii(const std::string& in);

    // Gets the unicode code points in a UTF-8 encoded string
    // Return -1 on bad encoding
    int GetUTF8CodeCount(const BYTE_t *p);

    // Gets the unicode byte count needed to encode uint16_t or C# char 
    //  Returns 1, 2 or 3
    unsigned int GetUTF8ByteCount(uint16_t i);

    //unsigned int GetUTF8ByteCount(const std::string& s);
}