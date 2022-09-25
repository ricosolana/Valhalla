#pragma once

#include <vector>
#include "Utils.h"

class Stream {
	std::unique_ptr<byte> m_bytes;
	size_t m_alloc = 0; // allocated bytes
	size_t m_length = 0; // written bytes
	size_t m_pos = 0; // read/write head offset from origin

	void EnsureCapacity(int count);
	void EnsureExtra(int extra);

public:
	Stream(int count = 0);
	Stream(const Stream&) = delete; // copy 
	Stream(Stream&&) noexcept;

	
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

	size_t Length() const;
	void SetLength(size_t length);
	void ResetPos();

	void Reserve(int count);
	void ReserveExtra(int extra);
};
