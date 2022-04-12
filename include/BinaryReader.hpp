#pragma once

#include <vector>
#include <string>
#include "Stream.hpp"

// https://dotnetfiddle.net/wg4fSm
// C# BinaryReader ported to C++
// Used to Read from the internal vector
class BinaryReader {
	Stream *m_stream;

	int Read7BitEncodedInt();

public:
	BinaryReader(Stream *stream);

	BinaryReader(BinaryReader&) = delete; // copy

	BinaryReader(BinaryReader&&); // move

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
		//value.
		auto byteCount = Read7BitEncodedInt();

		if (byteCount == 0)
			return "";

		std::string out;
		out.resize(byteCount);

		Read(reinterpret_cast<byte*>(out.data()), byteCount);

		// Do not worry about string copy when optimizations are disabled
		return out;
	}
};
