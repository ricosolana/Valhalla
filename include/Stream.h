#pragma once

#include <vector>
#include "Utils.h"

class Stream {
#ifdef REALLOC_STREAM
	byte_t* m_bytes = nullptr;
#else
	std::unique_ptr<byte_t> m_bytes;
#endif
	uint32_t m_alloc = 0; // allocated bytes
	uint32_t m_length = 0; // written bytes
	uint32_t m_marker = 0; // read/write head offset from origin

	// Reserves in the next power of two
	void EnsureCapacity(uint32_t count);
	void EnsureExtra(uint32_t extra);

public:
	Stream(uint32_t count = 0);
	Stream(const Stream&); // copy 
	Stream(Stream&&) = delete; // move removed

#ifdef REALLOC_STREAM
	~Stream();
#endif

	
	// Read count bytes into the specified buffer
	// May throw
	void Read(byte_t* buffer, uint32_t count);
	
	// Read a single byte from the buffer
	// May throw
	byte_t Read();

	// Reads count bytes into the specified vector
	// The vector is solely inserted into
	// May throw
	void Read(std::vector<byte_t>& vec, uint32_t count);

	// Reads the remaining bytes into the specified vector
	// The vector is solely inserted into
	// Should never throw
	//void Read(std::vector<byte_t>& vec);

	void Read(std::string& s, uint32_t count);

	//template<typename C, typename T = typename C::value_type>
	//void Read(C& container)
	//{
	//	for (int v : container) { ... }
	//}

	//template<typename C,
	//	typename T = std::decay_t<
	//	decltype(*Read(std::declval<C>()))>>
	//void Read(C& container, uint32_t count) {
	//	if (m_marker + count > m_length) throw std::range_error("Stream::Read(CONTAINER, int count) length exceeded");
	//
	//	container.insert(container.end(),
	//		Bytes() + m_marker,
	//		Bytes() + m_marker + count);
	//
	//	m_marker += count;
	//}

	

	// Write count bytes from the specified buffer
	void Write(const byte_t* buffer, uint32_t count);

	// Write a single byte from the specified buffer
	void Write(const byte_t value);

	// Write count bytes from the specified vector
	void Write(const std::vector<byte_t>& vec, uint32_t count);

	// Write all bytes from the specified vector
	void Write(const std::vector<byte_t>& vec);



	// Gets all the data in the package
	void Bytes(std::vector<byte_t>& out);

	// Returns the raw bytes pointer
	byte_t* Bytes() const;



	// Reset the marker and length members
	// No array shrinking is performed
	void Clear();



	// Returns the length of this stream
	uint32_t Length() const;

	// Sets the length of this stream
	// Will not realloc
	// May throw
	void SetLength(uint32_t length);
	


	// Returns the internal offset of this stream
	uint32_t Marker() const;

	// Skip the internal marker ahead by marks bytes
	// May throw
	void Skip(uint32_t marks);

	void SetMarker(uint32_t marker);



	// Reserves count bytes if m_alloc is less than count
	void Reserve(uint32_t count);
	void ReserveExtra(uint32_t extra);
};
