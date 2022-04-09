#include "Stream.hpp"

Stream::Stream(int count) {
    reserve(count);
}

// Move Constructor
//Stream::Stream(Stream&& other)
//    : m_bytes{ other.m_bytes }
//{
//
//    cout << "Move Constructor for "
//        << *source.data << endl;
//    source.data = nullptr;
//}



void Stream::Read(byte* buffer, int offset, int count) {
    Read(buffer + offset, count);
}

void Stream::Read(byte* buffer, int count) {
    std::memcpy(buffer, m_bytes.get() + m_pos, count);
    m_pos += count;
}

byte Stream::ReadByte() {
    byte b;
    Read(&b, 0, 1);
    return b;
}

void Stream::Read(std::vector<byte>& vec, int count) {
    vec.clear();
    vec.insert(vec.begin(), m_bytes.get() + m_pos, m_bytes.get() + m_pos + count);
    m_pos += count;
}



void Stream::Write(const byte* buffer, int offset, int count) {
    Write(buffer + offset, count);
}

void Stream::Write(const byte* buffer, int count) {
    ensureCapacity(m_pos + count);
    std::memcpy(m_bytes.get() + m_pos, buffer, count);
    m_pos += count;
    m_size += count;
}

void Stream::WriteByte(const byte value) {
    Write(&value, 0, 1);
}

void Stream::Write(const std::vector<byte>& vec, int count) {
    Write(vec.data(), 0, count);
}



void Stream::Clear() {
    m_pos = 0;
    m_size = 0;
}

size_t Stream::Size() {
    return m_size;
}




void Stream::reserve(int count) {
    if (m_alloc < count) {
        auto oldPtr = m_bytes.release();
        auto newPtr = new byte[count];
        std::memcpy(newPtr, oldPtr, static_cast<size_t>(count));
        m_bytes = std::unique_ptr<byte>(newPtr);
        delete[] oldPtr;
        m_alloc = count;
    }
}

void Stream::reserveExtra(int extra) {
    reserve(m_alloc + extra);
}

void Stream::ensureCapacity(int count) {
    reserve(count << 1);
}

void Stream::ensureExtra(int extra) {
    ensureCapacity(m_alloc + extra);
}