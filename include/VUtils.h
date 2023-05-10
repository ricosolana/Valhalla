#pragma once

//#include <WinSock2.h>
#include <chrono>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <utility>
#include <array>
#include <filesystem>
#include <span>

#include <zstd.h>
#include <zlib.h>
#include <ankerl/unordered_dense.h>
#include <dpp/dpp.h>
#include <tracy/Tracy.hpp>
#include <quill/Quill.h>

#include "CompileSettings.h"
#include "VUtilsEnum.h"

// global logger
//  I don't know where else to put it; this seems fine
extern quill::Logger* LOGGER;

namespace fs = std::filesystem;
using namespace std::chrono;
using namespace std::chrono_literals;

// https://stackoverflow.com/a/17350413
#define COLOR_RESET "\033[0m"
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_GOLD "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_PURPLE "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_GRAY "\033[90m"

using BYTE_t = char; // Unsigned 8 bit
using HASH_t = int32_t; // Used for RPC method hashing
using OWNER_t = int64_t; // Should rename to UID
using PLAYER_ID_t = int64_t; // Should rename to UID
using BYTES_t = std::vector<BYTE_t>; // Vector of bytes
using BYTE_VIEW_t = std::span<BYTE_t>;

using TICKS_t = duration<int64_t, std::ratio<1, 10000000>>;

//template<typename K, typename V>
//using UNORDERED_MAP_t = ankerl::unordered_dense::map<K, V>;

//template<typename K>
//using UNORDERED_SET_t = ankerl::unordered_dense::set<K>;

template<typename K, typename V, typename Hash = ankerl::unordered_dense::hash<K>>
using UNORDERED_MAP_t = ankerl::unordered_dense::map<K, V, Hash>;

template<typename K, typename Hash = ankerl::unordered_dense::hash<K>, typename Equal = std::equal_to<K>>
using UNORDERED_SET_t = ankerl::unordered_dense::set<K, Hash, Equal>;

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




template<typename T> requires (std::is_integral_v<T> && sizeof(T) == 8)
class IntegralWrapper {
    using Type = IntegralWrapper<T>;

private:
    T m_value;

public:
    constexpr IntegralWrapper() 
        : m_value(0) {}

    constexpr IntegralWrapper(T value)
        : m_value(value) {}
    
    constexpr IntegralWrapper(uint32_t high, uint32_t low)
        : m_value((static_cast<T>(high) << 32) | static_cast<T>(low)) {}

    constexpr IntegralWrapper(const std::string &hex) {
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



    IntegralWrapper<T> operator+(const IntegralWrapper<T>& rhs) const {
        return IntegralWrapper<T>(this->m_value + rhs.m_value);
    }

    IntegralWrapper<T> operator-(const IntegralWrapper<T>& rhs) const {
        return IntegralWrapper<T>(this->m_value - rhs.m_value);
    }

    IntegralWrapper<T> operator-() const {
        return IntegralWrapper<T>(-this->m_value);
    }

    IntegralWrapper<T> operator*(const IntegralWrapper<T>& rhs) const {
        return IntegralWrapper<T>(this->m_value * rhs.m_value);
    }

    IntegralWrapper<T> operator/(const IntegralWrapper<T>& rhs) const {
        return IntegralWrapper<T>(this->m_value / rhs.m_value);
    }

    bool operator==(const IntegralWrapper<T>& rhs) const {
        return this->m_value == rhs.m_value;
    }

    bool operator!=(const IntegralWrapper<T>& rhs) const {
        return this->m_value != rhs.m_value;
    }

    bool operator<(const IntegralWrapper<T>& rhs) const {
        return this->m_value < rhs.m_value;
    }

    bool operator<=(const IntegralWrapper<T>& rhs) const {
        return this->m_value <= rhs.m_value;
    }



    decltype(auto) __divi(const Type& other) {
        return this->m_value / other.m_value;
    }
};

using UInt64Wrapper = IntegralWrapper<uint64_t>;
using Int64Wrapper = IntegralWrapper<int64_t>;

std::ostream& operator<<(std::ostream& st, const UInt64Wrapper& val);
std::ostream& operator<<(std::ostream& st, const Int64Wrapper& val);



class ZStdCompressor {
    ZSTD_CCtx* m_ctx;
    ZSTD_CDict* m_dict;

public:
    ZStdCompressor(int level) {
        this->m_ctx = ZSTD_createCCtx();
        if (!this->m_ctx)
            throw std::runtime_error("failed to init zstd cctx");

        auto status = ZSTD_CCtx_setParameter(this->m_ctx, ZSTD_c_compressionLevel, level);
        if (ZSTD_isError(status)) {
            ZSTD_freeCCtx(m_ctx);
            throw std::runtime_error("failed to set zstd compression level parameter");
        }
        this->m_dict = nullptr;
    }

    ZStdCompressor() : ZStdCompressor(ZSTD_CLEVEL_DEFAULT) {}

    ZStdCompressor(const BYTE_t* dict, size_t dictSize, int level) {
        this->m_ctx = ZSTD_createCCtx();
        if (!this->m_ctx)
            throw std::runtime_error("failed to init zstd cctx");

        this->m_dict = ZSTD_createCDict(dict, dictSize, level);
        if (!this->m_dict) {
            ZSTD_freeCCtx(m_ctx);
            throw std::runtime_error("failed to create zstd cdict");
        }
    }

    ZStdCompressor(const BYTES_t& dict, int level) 
        : ZStdCompressor(dict.data(), dict.size(), level) {}

    ZStdCompressor(const BYTES_t& dict) 
        : ZStdCompressor(dict, ZSTD_CLEVEL_DEFAULT) {}

    ZStdCompressor(const ZStdCompressor&) = delete;
    ZStdCompressor(ZStdCompressor&& other) {
        this->m_ctx = other.m_ctx;
        this->m_dict = other.m_dict;
        other.m_ctx = nullptr;
    }

    ~ZStdCompressor() {
        ZSTD_freeCCtx(this->m_ctx);
        ZSTD_freeCDict(this->m_dict);
    }

public:
    std::optional<BYTES_t> Compress(const BYTE_t* in, size_t inSize) {
        BYTES_t out;
        out.resize(ZSTD_compressBound(inSize));
        
        auto status = m_dict ?
            ZSTD_compress_usingCDict(this->m_ctx, out.data(), out.size(), in, inSize, this->m_dict) : 
            ZSTD_compress2(this->m_ctx, out.data(), out.size(), in, inSize);

        if (ZSTD_isError(status))
            return std::nullopt;

        out.resize(status);
        return out;
    }

    std::optional<BYTES_t> Compress(const BYTES_t& in) {
        return Compress(in.data(), in.size());
    }
};

class ZStdDecompressor {
    ZSTD_DCtx* m_ctx;
    ZSTD_DDict* m_dict;

public:
    ZStdDecompressor() {
        this->m_ctx = ZSTD_createDCtx();
        if (!this->m_ctx)
            throw std::runtime_error("failed to init zstd dctx");
        this->m_dict = nullptr;
    }

    ZStdDecompressor(const BYTE_t* dict, size_t dictSize) {
        this->m_ctx = ZSTD_createDCtx();
        if (!this->m_ctx)
            throw std::runtime_error("failed to init zstd dctx");

        this->m_dict = ZSTD_createDDict(dict, dictSize);
        if (!this->m_dict) {
            ZSTD_freeDCtx(m_ctx);
            throw std::runtime_error("failed to create zstd ddict");
        }
    }

    ZStdDecompressor(const BYTES_t& dict) 
        : ZStdDecompressor(dict.data(), dict.size()) {}

    ZStdDecompressor(const ZStdDecompressor&) = delete;
    ZStdDecompressor(ZStdDecompressor&& other) {
        this->m_ctx = other.m_ctx;
        this->m_dict = other.m_dict;
        other.m_ctx = nullptr;
    }

    ~ZStdDecompressor() {
        ZSTD_freeDCtx(this->m_ctx);
        ZSTD_freeDDict(this->m_dict);
    }

public:
    std::optional<BYTES_t> Decompress(const BYTE_t* in, size_t inSize) {
        auto size = ZSTD_getFrameContentSize(in, inSize);
        if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN)
            return std::nullopt;

        BYTES_t out;
        out.resize(size);

        if (this->m_dict) {
            // check that dictionaries match
            auto expect = ZSTD_getDictID_fromDDict(this->m_dict);
            auto actual = ZSTD_getDictID_fromFrame(in, inSize);
            if (expect != actual)
                return std::nullopt;
        }

        auto status = this->m_dict ? 
            ZSTD_decompress_usingDDict(this->m_ctx, out.data(), out.size(), in, inSize, this->m_dict) : 
            ZSTD_decompressDCtx(this->m_ctx, out.data(), out.size(), in, inSize);

        if (ZSTD_isError(status))
            return std::nullopt;

        assert(status == out.size());

        out.resize(status);
        return out;
    }

    std::optional<BYTES_t> Decompress(const BYTES_t& in) {
        return Decompress(in.data(), in.size());
    }
};



class Deflater {
    int m_level;
    int m_windowBits;

    Deflater(int level, int windowBits) : m_level(level), m_windowBits(windowBits) {}

public:
    static Deflater Gz(int level) {
        return Deflater(level, 15 | 16);
    };

    static Deflater Gz() {
        return Gz(Z_BEST_SPEED);
    };

    static Deflater ZLib(int level) {
        return Deflater(level, 15);
    }

    static Deflater ZLib() {
        return ZLib(Z_BEST_SPEED);
    }

    // Deflater with no header
    static Deflater Raw(int level) {
        return Deflater(level, -15);
    }

    // Deflater with no header
    static Deflater Raw() {
        return Raw(Z_BEST_SPEED);
    }

public:
    std::optional<BYTES_t> Compress(const BYTE_t* in, size_t inSize) {
        if (inSize == 0)
            return std::nullopt;

        BYTES_t out;

        z_stream zs{};

        // https://www.zlib.net/manual.html
        // default windowBits with deflateInit is 15

        // possible init errors are:
        //  - invalid param (can be fixed at compile time)
        //  - out of memory (unlikely)
        //  - incompatible version (should be fine if using the init macro)
        // https://stackoverflow.com/a/72499721
        if (deflateInit2(&zs, m_level, Z_DEFLATED, m_windowBits, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            return std::nullopt;

        // Set output buffer size to an upper bound compressed size
        // Might throw if out of memory (unlikely)
        out.resize(deflateBound(&zs, inSize));

        zs.avail_in = (uInt)inSize;
        zs.next_in = (Bytef*)in;
        zs.next_out = (Bytef*)out.data();
        zs.avail_out = out.size();

        auto r = deflate(&zs, Z_FINISH);
        if (r != Z_STREAM_END) {
            return std::nullopt;
        }

        out.resize(zs.total_out);

        return out;
    }

    std::optional<BYTES_t> Compress(const BYTES_t& in) {
        return Compress(in.data(), in.size());
    }
};

class Inflater {
private:
    int m_windowBits;

    Inflater(int windowBits) : m_windowBits(windowBits) {}

public:
    // Inflater that uses the header from the compressed data
    //static Inflater Any() {
    //    return Inflater(0);
    //}

    // Inflater expecting zlib header only
    static Inflater ZLib() {
        return Inflater(15);
    }

    // Inflater expecting gz header only 
    static Inflater Gz() {
        return Inflater(16);
    };

    // Inflater with automatic zlib/gz header detection
    static Inflater Auto() {
        return Inflater(15 | 16);
    }

    // Inflater with no header
    static Inflater Raw() {
        return Inflater(-15);
    }

public:
    std::optional<BYTES_t> Decompress(const BYTE_t* in, unsigned int inSize) {
        if (inSize == 0)
            return std::nullopt;

        BYTES_t out;
        out.resize(inSize * 2);

        z_stream stream;
        stream.next_in = (Bytef*)in;
        stream.avail_in = inSize;
        stream.total_out = 0;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;

        if (inflateInit2(&stream, m_windowBits) != Z_OK)
            return std::nullopt;

        while (true) {
            // If our output buffer is too small
            if (stream.total_out >= out.size()) {
                out.resize(stream.total_out + inSize / 2);
            }

            // Advance to the next chunk to decode
            stream.next_out = (Bytef*)(out.data() + stream.total_out);

            // Set the available output capacity
            stream.avail_out = out.size() - stream.total_out;

            // Inflate another chunk.
            int err = inflate(&stream, Z_SYNC_FLUSH);
            if (err == Z_STREAM_END)
                break;
            else if (err != Z_OK) {
                return std::nullopt;
            }
        }

        if (inflateEnd(&stream) != Z_OK)
            return std::nullopt;

        // Trim off the extra-capacity inflated bytes
        out.resize(stream.total_out);
        return out;
    }

    std::optional<BYTES_t> Decompress(const BYTES_t& in) {
        return Decompress(in.data(), in.size());
    }
};





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
    
}
