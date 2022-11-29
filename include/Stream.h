#pragma once

#include <vector>
#include "VUtils.h"

class NetPackage;

class Stream {
	friend NetPackage;

public: BYTES_t m_buf;
private: uint32_t m_pos; // read/write head offset from origin

public:
	Stream();
	Stream(const BYTE_t* data, uint32_t count);
	explicit Stream(uint32_t reserve);  // capacity
    explicit Stream(BYTES_t vec);       // assign
	Stream(const Stream& other);        // copy
	Stream(Stream&& other) noexcept ;   // move

	void operator=(const Stream& other);

	// Read count bytes into the specified buffer
	// Will throw if count exceeds end
	void Read(BYTE_t* buffer, uint32_t count);
	
	// Read a single byte from the buffer
	// Will throw if at end
	BYTE_t Read();

	// Reads count bytes overriding the specified vector
	// Will throw if count exceeds end
	void Read(std::vector<BYTE_t>& vec, uint32_t count);

	// Reads count bytes overriding the specified string
	// '\0' is not included in count (raw bytes only)
	// Will throw if count exceeds end
	//void Read(std::string& s, uint32_t count);

	

	// Write count bytes from the specified buffer
	void Write(const BYTE_t* buffer, uint32_t count);

	// Write a single byte from the specified buffer
	auto Write(const BYTE_t value) {
		return Write(&value, sizeof(value));
	}

	// Write count bytes from the specified vector
	auto Write(const std::vector<BYTE_t>& vec, uint32_t count) {
		return Write(vec.data(), count);
	}

	// Write all bytes from the specified vector
	auto Write(const std::vector<BYTE_t>& vec) {
		return Write(vec, static_cast<uint32_t>(vec.size()));
	}



	// Gets all the data in the package
	[[nodiscard]] BYTES_t Bytes() const {
		return m_buf;
	}

	// Gets all the data in the package
	void Bytes(BYTES_t &out) const {
		out = m_buf;
	}

    BYTES_t Remaining() {
        BYTES_t res;
        Read(res, m_buf.size() - m_pos);
        return res;
    }


	// Reset the marker and length members
	// No array shrinking is performed
	void Clear() {
		m_pos = 0;
		m_buf.clear();
	}



	// Returns the length of this stream
	uint32_t Length() const {
		return m_buf.size();
	}

	// Sets the length of this stream
	void SetLength(uint32_t length) {
		m_buf.resize(length);
	}
	


	// Returns the position of this stream
	uint32_t Position() const {
		return m_pos;
	}

	// Sets the positino of this stream
	void SetPos(uint32_t pos); 
};
