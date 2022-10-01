#pragma once

#include <vector>
#include "Utils.h"

class Stream {
	std::unique_ptr<byte_t> m_bytes;
	size_t m_alloc = 0; // allocated bytes
	size_t m_length = 0; // written bytes
	size_t m_pos = 0; // read/write head offset from origin

	// Reserves in the next power of two
	void EnsureCapacity(int count);
	void EnsureExtra(int extra);

public:
	Stream(int count = 0);
	Stream(const Stream&); // copy 
	Stream(Stream&&) = delete; //noexcept;

	
	// internal -> buffer
	void Read(byte_t* buffer, int offset, int count);
	void Read(byte_t* buffer, int count);
	byte_t ReadByte();
	void Read(std::vector<byte_t>& vec, int count);

	// buffer -> internal
	void Write(const byte_t* buffer, int offset, int count);
	void Write(const byte_t* buffer, int count);
	void WriteByte(const byte_t value);
	void Write(const std::vector<byte_t>& vec, int count);

	byte_t* Bytes() const {
		return m_bytes.get();
	}

	void Clear();

	size_t Length() const;
	void SetLength(size_t length);
	void ResetPos();

	// Reserves count bytes if m_alloc is less than count
	void Reserve(int count);
	void ReserveExtra(int extra);
};
