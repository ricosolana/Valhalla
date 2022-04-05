#pragma once

#include <vector>
#include <string>
#include "Utils.hpp"

// https://dotnetfiddle.net/wg4fSm
// C# BinaryReader ported to C++
// Used to Read from the internal vector
class BinaryReader {
	Stream& m_stream;

	int Read7BitEncodedInt();

public:
	BinaryReader(Stream& stream);

	void Read(byte* out, int offset, int count);
	void Read(byte* out, int count);
	void Read(std::vector<byte>& out, int count);

	template<typename T>
	T Read() requires std::is_trivially_copyable_v<T> {
		T out;
		Read(reinterpret_cast<byte*>(&out), sizeof(T));
		return out;
	}

	std::string ReadString();
};
