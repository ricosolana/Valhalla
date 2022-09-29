#pragma once

#include <vector>
#include <string>
#include "Stream.h"

// https://dotnetfiddle.net/wg4fSm
// C# BinaryReader ported to C++
// Used to Read from the internal vector
class BinaryReader {
	Stream *m_stream;

	int Read7BitEncodedInt();

public:
	BinaryReader(Stream *stream);
	BinaryReader(BinaryReader&) = default; // copy
	BinaryReader(BinaryReader&&) = delete; // move

	void Read(byte* out, int offset, int count);
	void Read(byte* out, int count);
	void Read(std::vector<byte>& out, int count);

	template<typename T>
	T Read() requires std::is_fundamental_v<T> {
		T out;
		Read(reinterpret_cast<byte*>(&out), sizeof(T));
		return out;
	}

	template<typename T>
	T Read() requires std::same_as<T, std::string> {
		
		//static_assert(sizeof(wchar_t) == 2, "wchar_t is not 2 bytes wide!");

		// new fully functional method:
		auto byteCount = Read7BitEncodedInt(); // num2

		// could return a blank string?
		// if byteCount is malformed,
		// the user input might malicious?
		// so throw to panic
		if (byteCount < 0)
			throw std::runtime_error("Invalid string");
		else if (byteCount == 0)
			return "";
		
		std::string out;
		out.resize(byteCount);

		Read(reinterpret_cast<byte*>(out.data()), byteCount);

		// Do not worry about string copy when optimizations are disabled
		return out;
	}
};
