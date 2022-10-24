#include "Stream.h"

Stream::Stream() 
    : m_alloc(0), m_length(0), m_marker(0) {}

Stream::Stream(uint32_t count) 
    : Stream() {
    Reserve(count);
}

// Copy constructor
Stream::Stream(const Stream& other) {
    this->m_alloc = 0;
    this->m_length = other.m_length;
    this->m_marker = other.m_marker;

    Reserve(other.m_alloc);
    std::copy(other.Ptr(), other.Ptr() + other.m_alloc, Ptr());
}

Stream::Stream(Stream&& other) noexcept
    : Stream(other.m_alloc) {
    // move from other
    this->m_buf = std::move(other.m_buf);
    this->m_alloc = other.m_alloc;
    this->m_length = other.m_length;
    this->m_marker = other.m_marker;

    // invalidate other
    other.m_alloc = 0;
    other.m_length = 0;
    other.m_marker = 0;
}

// Data stream guaranteed to contain length data and marker pos
void Stream::operator=(const Stream& other) {    
    Reserve(other.m_alloc);
    std::copy(other.Ptr(), other.Ptr() + other.m_alloc, this->Ptr());

    this->m_length = other.m_length;
    this->m_marker = other.m_marker;
}



void Stream::Read(byte_t* buffer, uint32_t count) {
    if (m_marker + count > m_length) throw std::range_error("Stream::Read(byte_t* buffer, int count) length exceeded");

    std::copy(Ptr() + m_marker, Ptr() + m_marker + count, buffer);

    m_marker += count;
}

byte_t Stream::Read() {
    byte_t b;
    Read(&b, 1);
    return b;
}

void Stream::Read(std::vector<byte_t>& vec, uint32_t count) {
    if (m_marker + count > m_length) throw std::range_error("Stream::Read(std::vector<byte_t>& vec, int count) length exceeded");
    
    vec.clear();
    vec.insert(vec.begin(), 
        Ptr() + m_marker, 
        Ptr() + m_marker + count);
    m_marker += count;
}



void Stream::Write(const byte_t* buffer, uint32_t count) {
    EnsureCapacity(m_marker + count);

    //std::memcpy(Bytes() + m_marker, buffer, count);
    std::copy(buffer, buffer + count, Ptr() + m_marker);
    m_marker += count;

    // Make it so that the length is always at least marker
    // Also prevents length incrementing when marker is moved back
    m_length = std::max(m_marker, m_length);
    //m_length = 0;
}



void Stream::SetLength(uint32_t length) {
    if (length > m_alloc) throw std::runtime_error("length exceeds allocated memory");

    m_length = length;
}



void Stream::SetMarker(uint32_t marker) {
    if (marker > m_length) throw std::runtime_error("marks exceeds remaining length");

    m_marker = marker;
}



void Stream::Reserve(uint32_t count) {
    if (m_alloc < count) {
#ifdef SAFE_STREAM
#error not implemented
        m_buf = (byte_t*) realloc(m_buf, sizeof(byte_t) * count);
        if (!m_buf)
            throw std::runtime_error("Stream failed to realloc; rare exception");
#else
        auto oldPtr = std::move(m_buf);
        m_buf = std::unique_ptr<byte_t>(new byte_t[count]);
        if (oldPtr) {
            std::copy(oldPtr.get(), oldPtr.get() + count, m_buf.get());
            //std::memcpy(m_buf.get(), oldPtr.get(), static_cast<size_t>(count));
        }
#endif
        m_alloc = count;
    }
}
