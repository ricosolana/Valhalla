#pragma once

#include <chrono>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <utility>
#include <array>
#include <filesystem>
#include <span>
#include <list>

#include <ankerl/unordered_dense.h>

#include "CompileSettings.h"

// Dummy loggers
#define LOGGER 0
#define LOG_INFO(logger, ...)
#define LOG_WARNING(logger, ...)
#define LOG_ERROR(logger, ...)

// Dummy webhook
#define VH_DISPATCH_WEBHOOK

using namespace std::chrono_literals;

using BYTE_t = char; // Unsigned 8 bit
using HASH_t = int32_t; // Used for RPC method hashing
using OWNER_t = int64_t; // Should rename to UID
using PLAYER_ID_t = int64_t; // Should rename to UID
using BYTES_t = std::vector<BYTE_t>; // Vector of bytes
using BYTE_VIEW_t = std::span<BYTE_t>;

using TICKS_t = std::chrono::duration<int64_t, std::ratio<1, 10000000>>;

template<typename K, typename V, typename Hash = ankerl::unordered_dense::hash<K>, typename Equal = std::equal_to<K>>
using UNORDERED_MAP_t = ankerl::unordered_dense::map<K, V, Hash, Equal>;

template<typename K, typename Hash = ankerl::unordered_dense::hash<K>, typename Equal = std::equal_to<K>>
using UNORDERED_SET_t = ankerl::unordered_dense::set<K, Hash, Equal>;

//constexpr TICKS_t operator"" t(unsigned long long _Val) noexcept {
//    return TICKS_t(_Val);
//}

// Runs a static periodic task later
#define PERIODIC_LATER(__period, __initial, ...) {\
    auto __now = std::chrono::steady_clock::now();\
    static auto __last_run = __now + __initial;\
    auto __elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(__now - __last_run);\
    if (__elapsed > __period) {\
        __last_run = __now;\
        { __VA_ARGS__ }\
    }\
}

// Runs a static periodic task
#define PERIODIC_NOW(__period, ...) {\
    auto __now = steady_clock::now();\
    static auto __last_run = __now;\
    auto __elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(__now - __last_run);\
    if (__elapsed > __period) {\
        __last_run = __now;\
        { __VA_ARGS__ }\
    }\
}

// Runs a static task later
#define DISPATCH_LATER(__initial, ...) {\
    auto __now = std::chrono::steady_clock::now();\
    static auto __last_run = __now;\
    auto __elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(__now - __last_run);\
    if (__elapsed > __initial) {\
        __last_run = std::chrono::steady_clock::time_point::max();\
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



static_assert(std::endian::native == std::endian::little, 
    "System must be little endian (big endian not supported for networking (who uses big endian anyways?)");

namespace VUtils {
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
    
    // https://github.com/Zunawe/md5-c/blob/main/md5.h
    //   borrowed from
    // MD5
    typedef struct {
        uint64_t size;        // Size of input in bytes
        uint32_t buffer[4];   // Current accumulation of hash
        uint8_t input[64];    // Input to be used in the next step
        uint8_t digest[16];   // Result of algorithm
    } MD5Context;

    void md5Init(MD5Context* ctx);

    void md5Update(MD5Context* ctx, uint8_t* input_buffer, size_t input_len);

    void md5Finalize(MD5Context* ctx);

    void md5Step(uint32_t* buffer, uint32_t* input);

    // Calculate the md5 hash of a char buffer
    void md5(const char* in, size_t inSize, uint8_t* out16);
}
