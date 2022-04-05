#pragma once

//#define WIN32_LEAN_AND_MEAN
#include <asio.hpp>

#include <chrono>
#include <easylogging++.h>
#include <iostream>
#include "AsyncDeque.hpp"

using namespace std::chrono_literals;

using byte = uint8_t;

class Stream {
	std::vector<byte> m_buf;

public:
	long long m_pos = 0;

	// internal -> buffer
	void Read(byte* buffer, int offset, int count);
	byte ReadByte();
	void Read(std::vector<byte>& vec, int count);

	// buffer -> internal
	void Write(const byte* buffer, int offset, int count);
	void WriteByte(const byte value);
	void Write(const std::vector<byte>& vec, int count);

	std::vector<byte>& Buffer() {
		return m_buf;
	}

};

namespace Valhalla {
	// Simple tag mark a field as never being null
	#define NOTNULL 
	// Simple tag mark a field as able to be null
	#define NULLABLE 

	// Player UID type
	using UID = size_t;

	struct TwoTupleHasher
	{
		template<typename A, typename B>
		std::size_t operator()(const std::tuple<A, B>& tup) const
		{
			return ((std::hash<A>{}(std::get<0>(tup))
				^ (std::hash<B>{}(std::get<1>(tup)) << 1) >> 1));
		}
	};

	namespace Utils {
		size_t constexpr StrHash(char const* input) {
			return *input ? static_cast<size_t>(*input) + 33 * StrHash(input + 1) : 5381;
		}

		void initLogger();

		int GetStableHashCode(const char* str);

		int GetUnicode8Count(const char* p);

		UID stringToUID(std::string_view sv);

		bool isAddress(std::string_view s);

		//std::string join(std::vector<std::string_view>& strings);

		//constexpr std::string join(std::initializer_list<std::string_view> strings);

		template<typename StrContainer>
		std::string join(std::string delimiter, StrContainer container) {
			std::string result;
			for (int i = 0; i < container.size() - 1; i++) {
				result += std::string(*(container.begin() + i)) + delimiter;
			}
			result += *(container.end() - 1);
			return result;
		}

		std::vector<std::string_view> split(std::string_view s, char ch);
	}
}
