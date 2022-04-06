#pragma once

#include <vector>
#include "Utils.hpp"

class Stream {
	std::vector<byte> m_buf;
	
public:
	long long m_pos = 0;

	Stream() {}
	Stream(int reserve);
	
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

	void ensureCapacity(int extra);

};