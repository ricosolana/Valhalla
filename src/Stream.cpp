#include "Stream.h"

Stream::Stream(int count) {
    Reserve(count);
}

// Move Constructor
Stream::Stream(Stream&& old) noexcept
{
    m_bytes.swap(old.m_bytes);

    this->m_alloc = old.m_alloc;
    this->m_length = old.m_length;
    this->m_pos = old.m_pos;

    old.m_alloc = 0;
    old.m_length = 0;
    old.m_pos = 0;
}



void Stream::Read(byte* buffer, int offset, int count) {
    Read(buffer + offset, count);
}

void Stream::Read(byte* buffer, int count) {
    if (m_pos + count > m_length) throw std::runtime_error("reading garbage data");

    std::memcpy(buffer, m_bytes.get() + m_pos, count);
    // throw if invalid
    m_pos += count;
}

byte Stream::ReadByte() {
    byte b;
    Read(&b, 0, 1);
    return b;
}

void Stream::Read(std::vector<byte>& vec, int count) {
    if (m_pos + count > m_length) throw std::runtime_error("reading garbage data");

    vec.clear();
    vec.insert(vec.begin(), m_bytes.get() + m_pos, m_bytes.get() + m_pos + count);
    m_pos += count;
}



void Stream::Write(const byte* buffer, int offset, int count) {
    Write(buffer + offset, count);
}

void Stream::Write(const byte* buffer, int count) {
    EnsureCapacity(m_pos + count);
    std::memcpy(m_bytes.get() + m_pos, buffer, count);
    m_pos += count;
    m_length += count;
}

void Stream::WriteByte(const byte value) {
    Write(&value, 0, 1);
}

void Stream::Write(const std::vector<byte>& vec, int count) {
    Write(vec.data(), 0, count);
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
        m_bytes = std::unique_ptr<byte>(new byte[count]);
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
