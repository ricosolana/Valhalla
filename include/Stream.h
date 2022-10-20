#pragma once

#include <vector>
#include "Utils.h"

class NetPackage;

class Stream {
	friend NetPackage;

#ifdef REALLOC_STREAM
	byte_t* m_buf = nullptr;
#else
	std::unique_ptr<byte_t> m_buf;
#endif
	uint32_t m_alloc = 0; // allocated bytes
	uint32_t m_length = 0; // written bytes
	uint32_t m_marker = 0; // read/write head offset from origin

	// Reserves in the next power of two
	void EnsureCapacity(uint32_t count) {
		Reserve(count << 1);
	}

	void EnsureExtra(uint32_t extra) {
		EnsureCapacity(m_alloc + extra);
	}

public:
	Stream(uint32_t count = 0);
	Stream(const Stream&); // copy 
	Stream(Stream&&) = default; // move removed

#ifdef REALLOC_STREAM
	~Stream();
#endif
	
	// Read count bytes into the specified buffer
	// Will throw if count exceeds end
	void Read(byte_t* buffer, uint32_t count);
	
	// Read a single byte from the buffer
	// Will throw if at end
	byte_t Read();

	// Reads count bytes overriding the specified vector
	// Will throw if count exceeds end
	void Read(std::vector<byte_t>& vec, uint32_t count);

	// Reads count bytes overriding the specified string
	// '\0' is not included in count (raw bytes only)
	// Will throw if count exceeds end
	//void Read(std::string& s, uint32_t count);

	

	// Write count bytes from the specified buffer
	void Write(const byte_t* buffer, uint32_t count);

	// Write a single byte from the specified buffer
	auto Write(const byte_t value) {
		return Write(&value, sizeof(value));
	}

	// Write count bytes from the specified vector
	auto Write(const std::vector<byte_t>& vec, uint32_t count) {
		return Write(vec.data(), count);
	}

	// Write all bytes from the specified vector
	auto Write(const std::vector<byte_t>& vec) {
		return Write(vec, static_cast<uint32_t>(vec.size()));
	}



	// Gets all the data in the package
	BYTES_t Bytes() const {
		//out.insert(out.begin(), Bytes(), Bytes() + m_length);
		return BYTES_t(Ptr(), Ptr() + m_length);
	}

	// Gets all the data in the package
	void Bytes(BYTES_t &out) const {
		out.clear();
		out.insert(out.begin(), Ptr(), Ptr() + m_length);
	}

	// https://stackoverflow.com/a/9370717
	
	// Returns the raw bytes pointer
	// implicitly hinted inline or
	byte_t* Ptr() const {
#ifdef REALLOC_STREAM
		return m_buf;
#else
		return m_buf.get();
#endif
	}



	// Reset the marker and length members
	// No array shrinking is performed
	void Clear() {
		m_marker = 0;
		m_length = 0;
	}



	// Returns the length of this stream
	uint32_t Length() const {
		return m_length;
	}

	// Sets the length of this stream
	// Will not realloc
	// May throw
	void SetLength(uint32_t length);
	


	// Returns the internal offset of this stream
	uint32_t Marker() const {
		return m_marker;
	}

	// Skip the internal marker ahead by marks bytes
	// Memory may be allocated and initialized
	//auto Skip(uint32_t marks) {
	//	//Write//
	//	//ReserveExtra(marks);
	//	return SetMarker(m_marker + marks);
	//}

	void SetMarker(uint32_t marker); 



	// Reserves count bytes if m_alloc is less than count
	void Reserve(uint32_t count);
	auto ReserveExtra(uint32_t extra) {
		return Reserve(m_alloc + extra);
	}
};
