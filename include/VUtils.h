#pragma once

#include <chrono>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <utility>
#include <array>
#include <filesystem>

#include <optick.h>
#include <easylogging++.h>
#include <sol/sol.hpp>

#include "CompileSettings.h"
#include "VUtilsEnum.h"

namespace fs = std::filesystem;
using namespace std::chrono;
using namespace std::chrono_literals;

using BYTE_t = uint8_t; // Unsigned 8 bit
using HASH_t = int32_t; // Used for RPC method hashing
using OWNER_t = int64_t; // Should rename to UID
using PLAYER_ID_t = int64_t; // Should rename to UID
using BYTES_t = std::vector<BYTE_t>; // Vector of bytes

using TICKS_t = duration<int64_t, std::ratio<1, 10000000>>;

//constexpr TICKS_t operator"" t(unsigned long long _Val) noexcept {
//    return TICKS_t(_Val);
//}

// Runs a static periodic task later
#define PERIODIC_LATER(__period, __initial, ...) {\
    auto __now = steady_clock::now();\
    static auto __last_run = __now + __initial;\
    auto __elapsed = duration_cast<milliseconds>(__now - __last_run);\
    if (__elapsed > __period) {\
        __last_run = __now;\
        { __VA_ARGS__ }\
    }\
}

// Runs a static periodic task
#define PERIODIC_NOW(__period, ...) {\
    auto __now = steady_clock::now();\
    static auto __last_run = __now;\
    auto __elapsed = duration_cast<milliseconds>(__now - __last_run);\
    if (__elapsed > __period) {\
        __last_run = __now;\
        { __VA_ARGS__ }\
    }\
}

// Runs a static task later
#define DISPATCH_LATER(__initial, ...) {\
    auto __now = steady_clock::now();\
    static auto __last_run = __now;\
    auto __elapsed = duration_cast<milliseconds>(__now - __last_run);\
    if (__elapsed > __initial) {\
        __last_run = steady_clock::time_point::max();\
        { __VA_ARGS__ }\
    }\
}



struct Color {
    float r, g, b, a;

    constexpr Color() : r(0), g(0), b(0), a(1) {}
    constexpr Color(float r, float g, float b) : r(r), g(g), b(b), a(1) {}
    constexpr Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

    Color Lerp(const Color& other, float t);
};

struct Color32 {
    BYTE_t r, g, b, a;

    constexpr Color32() : r(0), g(0), b(0), a(1) {}
    constexpr Color32(BYTE_t r, BYTE_t g, BYTE_t b) : r(r), g(g), b(b), a(1) {}
    constexpr Color32(BYTE_t r, BYTE_t g, BYTE_t b, BYTE_t a) : r(r), g(g), b(b), a(a) {}

    //Color Lerp(const Color& other, float t);

    Color32 Lerp(const Color32& other, float t);
};

namespace Colors {
    static constexpr Color BLACK = Color(0, 0, 0);
    static constexpr Color RED = Color(1, 0, 0);
    static constexpr Color GREEN = Color(0, 1, 0);
    static constexpr Color BLUE = Color(0, 0, 1);
}



static constexpr float PI = 3.141592653589f;



#if FALSE
template<typename Enum> requires std::is_enum_v<Enum>
class BitMask {    
    using Type = std::underlying_type_t<Enum>;
    using Mask = BitMask<Enum>;

    // https://en.cppreference.com/w/cpp/utility/to_underlying
    Type value;

public:
    constexpr BitMask() : value(0) {}
    //BitMask(T value) : value(std::to_underlying(value)) {}

    constexpr BitMask(Enum value) : value(static_cast<Type>(value)) {}

    //BitMask(const BitMask& other) : value(other.value) {} // copy



    // assignment operator overload
    void operator=(const Mask& other) {
        this->value = other.value;
    }

    void operator=(const Type& other) {
        this->value = static_cast<Type>(other);
    }


    
    Mask operator&(const Mask& other) const {
        return BitMask<Enum>(static_cast<Enum>(this->value & other.value));
    }

    Mask operator&(const Enum& other) const {
        return *this & Mask(other);
    }



    Mask operator|(const Mask& other) const {
        return BitMask<Enum>(static_cast<Enum>(this->value | other.value));
    }

    Mask operator|(const Enum& other) const {
        return *this | Mask(other);
    }

    

    Mask& operator|=(const Mask& other) {
        this->value |= other.value;
        return *this;
    }

    Mask& operator|=(const Enum& other) {
        *this |= Mask(other);
        return *this;
    }

    Mask& operator|=(const Type& other) {
        *this |= Mask(other);
        return *this;
    }



    Mask operator<<(const Mask& other) {
        //return *this |= M
    }





    //BitMask

    

    operator Enum() const {
        return static_cast<Enum>(this->value);
    }

    operator Type() const {
        return static_cast<Type>(this->value);
    }



    bool operator()(const Mask& other) const {
        return (value & other.value) == other.value;
    }

    bool operator()(const Enum &other) const {
        return (*this)(Mask(other));
    }

    bool operator==(const Mask& other) const {
        return *this == other.value;
    }



    // Returns the least significant bit shift or 0 if none
    Type Shift() const {
        unsigned int shift = 0;

        unsigned int bits = static_cast<unsigned int>(this->value);
        for (; bits; shift++) {
            bits >>= 1;
        }

        return shift;
    }

    //operator |=
};
#endif






class Int64Wrapper {
private:
    int64_t m_value;

public:
    Int64Wrapper() {
        m_value = 0;
    }

    Int64Wrapper(int64_t value) 
        : m_value(value) {

    }
    
    Int64Wrapper(uint32_t high, uint32_t low) {
        // high and low should be bounded around 2^32
        m_value = ((uint64_t)high << 32) | (uint64_t)low;
    }

    Int64Wrapper(const std::string &hex) {
        // high and low should be bounded around 2^32

        // number accepts '0x...', 'bases...'
        // when radix is 0, radix will be deduced from the initial base '0x...'
        this->m_value = std::strtoull(hex.c_str() + 2, nullptr, 0);
    }



    operator int64_t() const {
        return this->m_value;
    }
    
    operator uint64_t() const {
        return this->m_value;
    }

    operator int32_t() const {
        return static_cast<int32_t>(this->m_value);
    }

    operator uint32_t() const {
        return static_cast<uint32_t>(this->m_value);
    }



    decltype(auto) __add(const Int64Wrapper& other) {
        return this->m_value + other.m_value;
    }

    decltype(auto) __sub(const Int64Wrapper& other) {
        return this->m_value - other.m_value;
    }

    decltype(auto) __mul(const Int64Wrapper& other) {
        return this->m_value * other.m_value;
    }

    decltype(auto) __div(const Int64Wrapper& other) {
        return this->m_value / other.m_value;
    }

    decltype(auto) __divi(const Int64Wrapper& other) {
        return this->m_value / other.m_value;
    }

    decltype(auto) __unm() {
        return -this->m_value;
    }

    decltype(auto) __eq(const Int64Wrapper& other) {
        return this->m_value == other.m_value;
    }

    decltype(auto) __lt(const Int64Wrapper& other) {
        return this->m_value < other.m_value;
    }

    decltype(auto) __le(const Int64Wrapper& other) {
        return this->m_value <= other.m_value;
    }
};



namespace VUtils {

    //class compress_error : public std::runtime_error {
    //    using runtime_error::runtime_error;
    //};
    //
    //class data_error : public std::runtime_error {
    //    using runtime_error::runtime_error;
    //};

    // Compress a byte array with a specified length and compression level
    // Stores the compressed contents into 'out' array
    // Returns the compressed size otherwise a negative number on compression failure
    std::optional<BYTES_t> CompressGz(const BYTE_t* in, unsigned int inSize, int level);

    // Compress a vector with a specified compression level
    // Returns nullopt on compress failure
    std::optional<BYTES_t> CompressGz(const BYTES_t& in, int level);
    // Compress a vector with specified length
    // Returns nullopt on compress failure
    std::optional<BYTES_t> CompressGz(const BYTES_t& in);



    // Decompress a byte array with a specified length
    // Returns nullopt on decompress failure
    std::optional<BYTES_t> Decompress(const BYTE_t* in, unsigned int inSize);

    // Decompress a byte vector
    // Returns nullopt on decompress failure
    std::optional<BYTES_t> Decompress(const BYTES_t& in);

    // Returns the smallest 1-value bitshift
    template<typename Enum> requires std::is_enum_v<Enum>
    constexpr uint8_t GetShift(Enum value) {
        uint8_t shift = -1;

        auto bits = std::to_underlying(value);
        for (; bits; shift++) {
            bits >>= 1;
        }

        return shift;
    }
    
}
