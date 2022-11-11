#pragma once

#include <chrono>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <optick.h>
#include <robin_hood.h>
#include <easylogging++.h>

#include "CompileSettings.h"

using namespace std::chrono;
namespace fs = std::filesystem;

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



using namespace std::chrono_literals;

using BYTE_t = uint8_t;
using HASH_t = int32_t;
using OWNER_t = int64_t;
using BYTES_t = std::vector<BYTE_t>;

static constexpr float PI = 3.141592653589f;

class compress_error : public std::runtime_error {
    using runtime_error::runtime_error;
};

template<typename T>
struct has_const_iterator
{
private:
    typedef char                      yes;
    typedef struct { char array[2]; } no;

    template<typename C> static yes test(typename C::const_iterator*);
    template<typename C> static no  test(...);
public:
    static const bool value = sizeof(test<T>(0)) == sizeof(yes);
    typedef T type;
};

template <typename T>
struct has_begin_end
{
    template<typename C> static char (&f(typename std::enable_if<
            std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::begin)),
                    typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

    template<typename C> static char (&f(...))[2];

    template<typename C> static char (&g(typename std::enable_if<
            std::is_same<decltype(static_cast<typename C::const_iterator (C::*)() const>(&C::end)),
                    typename C::const_iterator(C::*)() const>::value, void>::type*))[1];

    template<typename C> static char (&g(...))[2];

    static bool const beg_value = sizeof(f<T>(0)) == 1;
    static bool const end_value = sizeof(g<T>(0)) == 1;
};

template<typename T>
struct is_container : std::integral_constant<bool, has_const_iterator<T>::value && has_begin_end<T>::beg_value && has_begin_end<T>::end_value>
{ };



namespace VUtils {

    // Compress a byte array with a specified length and compression level
    // Stores the compressed contents into 'out' array
    // Returns the compressed size otherwise a negative number on compression failure
    int CompressGz(const BYTE_t* in, unsigned int inSize, int level, BYTE_t* out, unsigned int outCapacity);
    // Compress a byte array with a specified length
    // Stores the compressed contents into 'out' array
    // Returns the compressed size otherwise a negative number on compression failure
    int CompressGz(const BYTE_t* in, unsigned int inSize, BYTE_t* out, unsigned int outCapacity);
    // Compress a byte array with a specified length and compression level
    // Stores the compressed contents into 'out' vector
    // Returns true otherwise false on compression failure
    bool CompressGz(const BYTE_t* in, unsigned int inSize, int level, BYTES_t& out);
    // Compress a byte array with a specified length
    // Stores the compressed contents into 'out' vector
    // Returns true otherwise false on compression failure
    bool CompressGz(const BYTE_t* in, unsigned int inSize, BYTES_t& out);

    // Compress a vector of bytes with a specified compression level
    // Stores the compressed contents into 'out' array
    // Returns the compressed size otherwise a negative number on compression failure
    int CompressGz(const BYTES_t& in, int level, BYTE_t* out, unsigned int outCapacity);
    // Compress a vector of bytes with a specified compression level
    // Stores the compressed contents into 'out' vector
    // Returns true otherwise false on compression failure
    bool CompressGz(const BYTES_t& in, int level, BYTES_t& out);
    // Compress a vector of bytes
    // Stores the compressed contents into 'out' vector
    // Returns true otherwise false on compression failure
    bool CompressGz(const BYTES_t& in, BYTES_t& out);



    // Compress a byte array with a specified length and compression level
    // Returns the compressed contents as a byte vector
    // Throws on compression failure
    BYTES_t CompressGz(const BYTE_t* in, unsigned int inSize, int level);
    // Compress a byte array with a specified length
    // Returns the compressed contents as a byte vector
    // Throws on compression failure
    BYTES_t CompressGz(const BYTE_t* in, unsigned int inSize);

    // Compress a vector with a specified compression level
    // Returns the compressed contents as a byte vector
    // Throws on compression failure
    BYTES_t CompressGz(const BYTES_t& in, int level);
    // Compress a vector with specified length
    // Returns the compressed contents as a byte vector
    // Throws on compression failure
    BYTES_t CompressGz(const BYTES_t& in);



    // Decompress a byte array with a specified length
    // Stores the decompressed contents into 'out' vector
    // Returns true otherwise false on decompresssion error
    bool Decompress(const BYTE_t* in, unsigned int inSize, BYTES_t &out);

    // Decompress a byte array with a specified length
    // Returns the decompressed contents as a byte vector
    // Throws on decompression error
    BYTES_t Decompress(const BYTE_t* in, unsigned int inSize);

    // Decompress a byte vector
    // Returns the decompressed contents as a byte vector
    // Throws on decompression error
    BYTES_t Decompress(const BYTES_t& in);

    OWNER_t GenerateUID();
}

//namespace VUtils = VUtility;
