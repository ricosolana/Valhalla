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
#include <zstd.h>

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




template<typename T> requires std::is_integral_v<T>
class IntegralWrapper {
    using Type = IntegralWrapper<T>;

private:
    T m_value;

public:
    IntegralWrapper() {
        m_value = 0;
    }

    IntegralWrapper(T value)
        : m_value(value) {}
    
    IntegralWrapper(uint32_t high, uint32_t low) {
        // high and low should be bounded around 2^32
        m_value = (static_cast<T>(high) << 32) | static_cast<T>(low);
    }

    IntegralWrapper(const std::string &hex) {
        // high and low should be bounded around 2^32

        // number accepts '0x...', 'bases...'
        // when radix is 0, radix will be deduced from the initial base '0x...'
        if constexpr (std::is_unsigned_v<T>)
            this->m_value = std::strtoull(hex.c_str() + 2, nullptr, 0);
        else 
            this->m_value = std::strtoll(hex.c_str() + 2, nullptr, 0);
    }



    operator int64_t() const {
        return static_cast<int64_t>(this->m_value);
    }
    
    operator uint64_t() const {
        return static_cast<uint64_t>(this->m_value);
    }

    operator int32_t() const {
        return static_cast<int32_t>(this->m_value);
    }

    operator uint32_t() const {
        return static_cast<uint32_t>(this->m_value);
    }



    decltype(auto) __add(const Type& other) {
        return this->m_value + other.m_value;
    }

    decltype(auto) __sub(const Type& other) {
        return this->m_value - other.m_value;
    }

    decltype(auto) __mul(const Type& other) {
        return this->m_value * other.m_value;
    }

    decltype(auto) __div(const Type& other) {
        return this->m_value / other.m_value;
    }

    decltype(auto) __divi(const Type& other) {
        return this->m_value / other.m_value;
    }

    decltype(auto) __unm() {
        return -this->m_value;
    }

    decltype(auto) __eq(const Type& other) {
        return this->m_value == other.m_value;
    }

    decltype(auto) __lt(const Type& other) {
        return this->m_value < other.m_value;
    }

    decltype(auto) __le(const Type& other) {
        return this->m_value <= other.m_value;
    }
};

using UInt64Wrapper = IntegralWrapper<uint64_t>;
using Int64Wrapper = IntegralWrapper<int64_t>;

std::ostream& operator<<(std::ostream& st, const UInt64Wrapper& val);
std::ostream& operator<<(std::ostream& st, const Int64Wrapper& val);



class Compressor {
    ZSTD_CCtx* m_ctx = nullptr;
    ZSTD_CDict* m_dict = nullptr;

public:
    Compressor(const BYTES_t& dict) {
        this->m_ctx = ZSTD_createCCtx();
        if (!this->m_ctx)
            throw std::runtime_error("failed to init zstd cctx");

        this->m_dict = ZSTD_createCDict(dict.data(), dict.size(), 1);
        if (!this->m_dict) {
            ZSTD_freeCCtx(m_ctx);
            throw std::runtime_error("failed to create zstd cdict");
        }
    }

    Compressor(const Compressor&) = delete;
    Compressor(Compressor&& other) {
        this->m_ctx = other.m_ctx;
        this->m_dict = other.m_dict;
        other.m_ctx = nullptr;
    }

    ~Compressor() {
        ZSTD_freeCCtx(this->m_ctx);
        ZSTD_freeCDict(this->m_dict);
    }

    std::optional<BYTES_t> Compress(const BYTES_t& in) {
        BYTES_t out;
        out.resize(ZSTD_compressBound(in.size()));

        auto status = ZSTD_compress_usingCDict(this->m_ctx, out.data(), out.size(), in.data(), in.size(), this->m_dict);
        if (ZSTD_isError(status))
            return std::nullopt;
        
        out.resize(status);
        return out;
    }
};

class Decompressor {
    ZSTD_DCtx* m_ctx = nullptr;
    ZSTD_DDict* m_dict = nullptr;

public:
    Decompressor(const BYTES_t& dict) {
        this->m_ctx = ZSTD_createDCtx();
        if (!this->m_ctx)
            throw std::runtime_error("failed to init zstd dctx");

        this->m_dict = ZSTD_createDDict(dict.data(), dict.size());
        if (!this->m_dict) {
            ZSTD_freeDCtx(m_ctx);
            throw std::runtime_error("failed to create zstd ddict");
        }
    }

    Decompressor(const Decompressor&) = delete;
    Decompressor(Decompressor&& other) {
        this->m_ctx = other.m_ctx;
        this->m_dict = other.m_dict;
        other.m_ctx = nullptr;
    }

    ~Decompressor() {
        ZSTD_freeDCtx(this->m_ctx);
        ZSTD_freeDDict(this->m_dict);
    }

    std::optional<BYTES_t> Decompress(const BYTES_t& in) {
        auto size = ZSTD_getFrameContentSize(in.data(), in.size());
        if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN)
            return std::nullopt;

        BYTES_t out;
        out.resize(size);

        // check that dictionaries match
        auto expect = ZSTD_getDictID_fromDDict(this->m_dict);
        auto actual = ZSTD_getDictID_fromFrame(in.data(), in.size());
        if (expect != actual)
            return std::nullopt;

        auto status = ZSTD_decompress_usingDDict(this->m_ctx, out.data(), out.size(), in.data(), in.size(), this->m_dict);
        if (ZSTD_isError(status))
            return std::nullopt;

        assert(status == out.size());

        out.resize(status);
        return out;
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

    //std::optional<BYTES_t> CompressZStd(const BYTES_t& in);



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
