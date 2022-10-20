#include "Stream.h"

Stream::Stream(uint32_t count) {
    Reserve(count);
}

// Copy constructor
Stream::Stream(const Stream& other)
    : Stream(other.m_alloc)
{
    // Retrieving data from m_alloc bytes location is undefined, 
    // so only bytes from pos 0 to m_length is utilized
    Write(other.Bytes(), other.m_length);
}

//Stream::Stream(Stream&& other) 
//    : Stream(other.m_alloc) {
//    // essentially want to swap the elements
//#ifdef REALLOC_STREAM
//
//#else
//    this->m_buf = std::move(other.m_buf); //this->m_buf.swap(other.m_buf);
//    this->m_length
//#endif
//}


#ifdef REALLOC_STREAM
Stream::~Stream() {
    free(m_buf);
}
#endif


void Stream::Read(byte_t* buffer, uint32_t count) {
    if (m_marker + count > m_length) throw std::range_error("Stream::Read(byte_t* buffer, int count) length exceeded");

    std::memcpy(buffer, 
        Ptr() + m_marker,
        count);
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

//void Stream::Read(std::string& s, uint32_t count) {
//    if (m_marker + count > m_length) throw std::range_error("Stream::Read(std::string& s, int count) length exceeded");
//
//    s.insert(s.end(),
//        Ptr() + m_marker,
//        Ptr() + m_marker + count);
//    m_marker += count;
//}



void Stream::Write(const byte_t* buffer, uint32_t count) {
    EnsureCapacity(m_marker + count);

    //std::memcpy(Bytes() + m_marker, buffer, count);
    std::copy(buffer, buffer + count, Ptr() + m_marker);
    m_marker += count;

    // Make it so that the length is always at least marker
    // Also prevents length incrementing when marker is moved back
    m_length = max(m_marker, m_length);
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
#ifdef REALLOC_STREAM
        m_buf = (byte_t*) realloc(m_buf, sizeof(byte_t) * count);
        if (!m_buf)
            throw std::runtime_error("Stream failed to realloc; rare exception");
#else
        auto oldPtr = std::move(m_buf);
        m_buf = std::unique_ptr<byte_t>(new byte_t[count]);
        if (oldPtr) {
            std::memcpy(m_buf.get(), oldPtr.get(), static_cast<size_t>(count));
        }
#endif
        m_alloc = count;
    }
}
