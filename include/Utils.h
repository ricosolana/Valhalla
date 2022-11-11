#pragma once

//#include <WinSock2.h>
//#include <Windows.h>

#include <chrono>
#include <iostream>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <optick.h>
#include <zlib.h>
#include <robin_hood.h>
#include <easylogging++.h>

#include "CompileSettings.h"

using namespace std::chrono;
namespace fs = std::filesystem;

#define __H(str) Utils::GetStableHashCode(str)

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




//bytes
using namespace std::chrono_literals;

using BYTE_t = uint8_t;
using OWNER_t = int64_t;
using HASH_t = int32_t;
using BYTES_t = std::vector<BYTE_t>;

static constexpr float PI = 3.141592653589f;

class compress_error : public std::runtime_error {
    using runtime_error::runtime_error;
};

namespace Utils {


    /*
	template <class F, class C, class Pass, class Tuple, size_t... Is>
	constexpr auto InvokeTupleImpl(F f, C& c, Pass pass, Tuple t, std::index_sequence<Is...>) {
		return std::invoke(f, c, pass, std::move(std::get<Is>(t))...);
		// the move causes the params to get invalidated when passing twice
		//return std::invoke(f, c, pass, std::get<Is>(t)...);
	}

	template <class F, class C, class Pass, class Tuple>
	constexpr void InvokeTuple(F f, C& c, Pass pass, Tuple t) {
		InvokeTupleImpl(f, c, pass, std::move(t),
			std::make_index_sequence < std::tuple_size<Tuple>{} > {}); // last arg is for template only
	}

	// Static method invoke
	template <class F, class Pass, class Tuple, size_t... Is>
	constexpr auto InvokeTupleImplS(F f, Pass pass, Tuple t, std::index_sequence<Is...>) {
		return std::invoke(f, pass, std::move(std::get<Is>(t))...);
		//return std::invoke(f, pass, std::get<Is>(t)...);
	}

	template <class F, class Pass, class Tuple>
	constexpr void InvokeTupleS(F f, Pass pass, Tuple t) {
		InvokeTupleImplS(f, pass, std::move(t),
			std::make_index_sequence < std::tuple_size<Tuple>{} > {}); // last arg is for template only
	}*/

    // https://stackoverflow.com/questions/44776927/call-member-function-with-expanded-tuple-parms-stdinvoke-vs-stdapply
    //template<typename F, typename T, typename U>
    //decltype(auto) apply_invoke(F&& func, T&& first, U&& tuple) {
    //    return std::apply(std::forward<F>(func), std::tuple_cat(std::forward_as_tuple(std::forward<T>(first)), std::forward<U>(tuple)));
    //}

    

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
    // Stores the compressed contents into 'out' array
    // Returns true otherwise false on decompresssion error
    bool Decompress(const BYTE_t* in, unsigned int inSize, BYTES_t &out);

    BYTES_t Decompress(const BYTE_t* in, unsigned int inSize);

    BYTES_t Decompress(const BYTES_t& in);



	// Encoding.ASCII.GetString equivalent:
	// bytes greater than 127 get turned to literal '?' (63)
	void FormatAscii(std::string& in);

	OWNER_t GenerateUID();

	HASH_t GetStableHashCode(const std::string &s);

	constexpr HASH_t GetStableHashCode(const char* str, uint32_t num, uint32_t num2, uint32_t idx) { // NOLINT(misc-no-recursion)
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

	constexpr HASH_t GetStableHashCode(const char* str) {
		uint32_t num = 5381;
		uint32_t num2 = num;
		uint32_t idx = 0;

		return GetStableHashCode(str, num, num2, idx);
	}

	// Gets the unicode code points in a UTF-8 encoded string
	// Return -1 on bad encoding
	int32_t GetUTF8Count(const BYTE_t* p);

	//OWNER_t StringToUID(const std::string &s);

	// ugly
	template<typename StrContainer>
	std::string Join(const std::string& delimiter, StrContainer container) {
		std::string result;
		for (int i = 0; i < container.size() - 1; i++) {
			result += std::string(*(container.begin() + i)) + delimiter;
		}
		result += *(container.end() - 1);
		return result;
	}

    std::vector<std::string_view> Split(std::string& s, const std::string &delim);
}
