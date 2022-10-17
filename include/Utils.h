#pragma once

#include <asio.hpp>

//#ifdef _WIN32
//#  ifdef ASIO_STANDALONE
////     Set the proper SDK version before including boost/Asio
//#      include <SDKDDKVer.h>
////     Note boost/ASIO includes Windows.h. 
//#      include <asio.hpp>
//#   else //  USE_ASIO
//#      include <Windows.h>
//#   endif //  ASIO_STANDALONE
//#else // _WIN32
//#  ifdef ASIO_STANDALONE
//#     include <asio.hpp>
//#  endif // ASIO_STANDALONE
//#endif //_WIN32

#include <chrono>
#include <iostream>
#include <zlib.h>
#include <robin_hood.h>
#include <easylogging++.h>
#include <type_traits>
#include <concepts>
#include <bitset>
#include "CompileSettings.h"
#include "AsyncDeque.h"

using namespace std::chrono_literals;

using byte_t = uint8_t;
using uuid_t = int64_t;
using hash_t = int32_t;
using bytes_t = std::vector<byte_t>;
//std::bitset<8> b;

//using Dictionary = robin_hood::unordered_map;

static constexpr float PI = 3.141592653589f;

//float FISQRT(float)

template<typename T>
class BitMask {
	//static_assert(std::is_integral_v<std::underlying_type_t<T>>, "Must be an integral enum");
	static_assert(std::is_enum<T>::value, "Must be an enum");

	// https://en.cppreference.com/w/cpp/utility/to_underlying
	std::underlying_type_t<T> value;

public:
	//BitMask()
	//BitMask(T value) : value(std::to_underlying(value)) {}

	BitMask(T value) : value(static_cast<std::underlying_type_t<T>>(value)) {}

	T operator()() {
		return static_cast<T>(value);
	}

	bool operator()(T otherEnum) {
		auto otherValue = static_cast<std::underlying_type_t<T>>(otherEnum);

		return (value & otherValue) == otherValue;
	}

	//operator |=
};

//struct TwoTupleHasher
//{
//	template<typename A, typename B>
//	std::size_t operator()(const std::tuple<A, B>& tup) const
//	{
//		return ((std::hash<A>{}(std::get<0>(tup))
//			^ (std::hash<B>{}(std::get<1>(tup)) << 1) >> 1));
//	}
//};

namespace Utils {

	template <class F, class C, class Pass, class Tuple, size_t... Is>
	constexpr auto InvokeTupleImpl(F f, C& c, Pass pass, Tuple& t, std::index_sequence<Is...>) {
		return std::invoke(f, c, pass, std::move(std::get<Is>(t))...);
	}

	template <class F, class C, class Pass, class Tuple>
	constexpr void InvokeTuple(F f, C& c, Pass pass, Tuple& t) {
		InvokeTupleImpl(f, c, pass, t,
			std::make_index_sequence < std::tuple_size<Tuple>{} > {}); // last arg is for template only
	}

	// for lua
	template <class F, class Pass, class Tuple, size_t... Is>
	constexpr auto InvokeTupleImplS(F f, Pass pass, Tuple& t, std::index_sequence<Is...>) {
		return std::invoke(f, pass, std::move(std::get<Is>(t))...);
		//return std::forward()
	}

	template <class F, class Pass, class Tuple>
	constexpr void InvokeTupleS(F f, Pass pass, Tuple& t) {
		InvokeTupleImplS(f, pass, t,
			std::make_index_sequence < std::tuple_size<Tuple>{} > {}); // last arg is for template only
	}

	bool Compress(const byte_t* buf, unsigned int bufSize, int level, byte_t* out, unsigned int& outSize);
	void Compress(const bytes_t& buf, int level, bytes_t& out);
	std::vector<byte_t> Compress(const byte_t* buf, unsigned int bufSize, int level);


	std::vector<byte_t> Decompress(const byte_t* compressedBytes, int count);

	//byte* Compress(const byte* uncompressedBytes, int count, int *outCount, int level = Z_DEFAULT_COMPRESSION);
	//byte* Decompress(const byte* compressedBytes, int count, int *outCount);

	uuid_t GenerateUID();

	hash_t GetStableHashCode(const std::string &s);

	// Gets the unicode code points in a UTF-8 encoded string
	// Return -1 on bad encoding
	int32_t GetUTF8Count(const byte_t* p);

	uuid_t StringToUID(const std::string &s);

	bool IsAddress(const std::string& s);

	//std::string join(std::vector<std::string_view>& strings);

	//constexpr std::string join(std::initializer_list<std::string_view> strings);

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

	std::vector<std::string_view> Split(std::string_view s, char ch);
}
