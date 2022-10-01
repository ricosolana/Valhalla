#pragma once

#include <vector>
#include <string>
#include "Stream.h"

// https://dotnetfiddle.net/wg4fSm
// C# BinaryWriter ported to C++
// Used to Write to the internal vector
class BinaryWriter {
	Stream *m_stream;

	void Write7BitEncodedInt(int value);

public:
	BinaryWriter(Stream* stream);
	BinaryWriter(BinaryWriter&) = default; // copy
	BinaryWriter(BinaryWriter&&) = delete; // move

	void Write(const byte_t* in, int offset, int count);
	void Write(const byte_t* in, int count);
	void Write(const std::vector<byte_t>& in, int count);

	template<typename T>
	void Write(const T &in) requires std::is_trivially_copyable_v<T> {
		Write(reinterpret_cast<const byte_t*>(&in), sizeof(T));
	}

	void Write(const std::string &in);
};
