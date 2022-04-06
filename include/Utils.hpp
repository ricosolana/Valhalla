#pragma once

#include <asio.hpp>

#include <chrono>
#include <easylogging++.h>
#include <iostream>
#include "AsyncDeque.hpp"

using namespace std::chrono_literals;

// Simple tag mark a field as never being null
#define NOTNULL 
// Simple tag mark a field as able to be null
#define NULLABLE 

using byte = uint8_t;
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
	int GetStableHashCode(const char* str);

	int GetUnicode8Count(const char* p);

	UID StringToUID(std::string_view sv);

	bool IsAddress(std::string_view s);

	//std::string join(std::vector<std::string_view>& strings);

	//constexpr std::string join(std::initializer_list<std::string_view> strings);

	template<typename StrContainer>
	std::string Join(std::string delimiter, StrContainer container) {
		std::string result;
		for (int i = 0; i < container.size() - 1; i++) {
			result += std::string(*(container.begin() + i)) + delimiter;
		}
		result += *(container.end() - 1);
		return result;
	}

	std::vector<std::string_view> Split(std::string_view s, char ch);
}
