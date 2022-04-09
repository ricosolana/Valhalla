#pragma once

#include <vector>
#include "Utils.hpp"

class Stream {
	std::unique_ptr<byte> m_bytes;
	size_t m_alloc = 0; // allocated bytes
	size_t m_size; // written bytes
	
	void ensureCapacity(int count);
	void ensureExtra(int extra);

public:
	size_t m_pos = 0; // read/write head offset from origin

	Stream(int count = 0);

	// Move Constructor
	//Stream(Stream&& other);

	
	// internal -> buffer
	void Read(byte* buffer, int offset, int count);
	void Read(byte* buffer, int count);
	byte ReadByte();
	void Read(std::vector<byte>& vec, int count);

	// buffer -> internal
	void Write(const byte* buffer, int offset, int count);
	void Write(const byte* buffer, int count);
	void WriteByte(const byte value);
	void Write(const std::vector<byte>& vec, int count);

	byte* Bytes() const {
		return m_bytes.get();
	}

	void Clear();

	size_t Size();

	void reserve(int count);
	void reserveExtra(int extra);

};