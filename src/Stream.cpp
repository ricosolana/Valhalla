#include "Stream.h"

Stream::Stream(int count) {
    Reserve(count);
}

// Copy constructor
Stream::Stream(const Stream& other)
    : Stream(other.m_alloc)
{
    // TODO should m_length or m_alloc bytes be copied
    //   technically, m_alloc bytes will be a true copy
    //   but m_length bytes leads to better defined behaviour

    // any bytes past m_length could be garbage (undefined behaviour)
    // so anyways, purpose is met, but not a true copy
    this->Write(other.Bytes(), other.Length());
}



void Stream::Read(byte_t* buffer, int offset, int count) {
    Read(buffer + offset, count);
}

void Stream::Read(byte_t* buffer, int count) {
    if (m_pos + count > m_length) throw std::range_error("Stream::Read(byte_t* buffer, int count) length exceeded");

    std::memcpy(buffer, m_bytes.get() + m_pos, count);
    // throw if invalid
    m_pos += count;
}

byte_t Stream::ReadByte() {
    byte_t b;
    Read(&b, 1);
    return b;
}

void Stream::Read(std::vector<byte_t>& vec, int count) {
    if (m_pos + count > m_length) throw std::range_error("Stream::Read(std::vector<byte_t>& vec, int count) length exceeded");

    vec.clear();
    vec.insert(vec.begin(), m_bytes.get() + m_pos, m_bytes.get() + m_pos + count);
    m_pos += count;
}

void Stream::Read(std::vector<byte_t>& vec) {
    return Read(vec, m_length);
}



void Stream::Write(const byte_t* buffer, int offset, int count) {
    Write(buffer + offset, count);
}

void Stream::Write(const byte_t* buffer, int count) {
    EnsureCapacity(m_pos + count);
    std::memcpy(m_bytes.get() + m_pos, buffer, count);
    m_pos += count;
    m_length += count;
}

void Stream::WriteByte(const byte_t value) {
    Write(&value, 0, 1);
}

void Stream::Write(const std::vector<byte_t>& vec, int count) {
    Write(vec.data(), count);
}

void Stream::Write(const std::vector<byte_t>& vec) {
    Write(vec.data(), m_length);
}



void Stream::Clear() {
    m_pos = 0;
    m_length = 0;
}

size_t Stream::Length() const {
    return m_length;
}

void Stream::SetLength(size_t length) {
    if (length > m_alloc) throw std::runtime_error("length exceeds allocated memory");

    m_length = length;
}

void Stream::ResetPos() {
    m_pos = 0;
}



void Stream::Reserve(int count) {
    if (m_alloc < count) {
        auto oldPtr = std::move(m_bytes);
        m_bytes = std::unique_ptr<byte_t>(new byte_t[count]);
        if (oldPtr) {
            std::memcpy(m_bytes.get(), oldPtr.get(), static_cast<size_t>(count));
        }
        m_alloc = count;
    }
}

void Stream::ReserveExtra(int extra) {
    Reserve(m_alloc + extra);
}

void Stream::EnsureCapacity(int count) {
    Reserve(count << 1);
}

void Stream::EnsureExtra(int extra) {
    EnsureCapacity(m_alloc + extra);
}
