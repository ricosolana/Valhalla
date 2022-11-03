#include "Stream.h"

Stream::Stream() 
    : m_pos(0) {}

Stream::Stream(uint32_t count) 
    : Stream() {
    m_buf.reserve(count);
}

Stream::Stream(BYTES_t vec)
    : Stream() {
    m_buf = std::move(vec);
}

Stream::Stream(Stream &&other) {
    this->m_buf = std::move(other.m_buf);
    this->m_pos = other.m_pos;

    other.m_pos = 0;
}



void Stream::Read(BYTE_t* buffer, uint32_t count) {
    if (m_pos + count > Length()) throw std::range_error("Stream::Read(BYTE_t* buffer, int count) length exceeded");

    // read into 'buffer'
    std::copy(m_buf.begin() + m_pos, m_buf.begin() + m_pos + count, buffer);

    m_pos += count;
}

BYTE_t Stream::Read() {
    BYTE_t b;
    Read(&b, 1);
    return b;
}

void Stream::Read(std::vector<BYTE_t>& vec, uint32_t count) {
    if (m_pos + count > Length()) throw std::range_error("Stream::Read(std::vector<BYTE_t>& vec, int count) length exceeded");
    
    vec.clear();
    vec.insert(vec.begin(), 
        m_buf.begin() + m_pos, 
        m_buf.begin() + m_pos + count);
    m_pos += count;
}



void Stream::Write(const BYTE_t* buffer, uint32_t count) {
    // trim everything else away
    m_buf.resize(m_pos);

    // add elements
    m_buf.insert(m_buf.begin() + m_pos, buffer, buffer + count);
    m_pos += count;
}



void Stream::SetPos(uint32_t pos) {
    if (pos > Length()) throw std::runtime_error("Position exceeds length");

    m_pos = pos;
}
