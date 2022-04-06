#include "Stream.hpp"

Stream::Stream(int reserve) {
    m_buf.reserve(reserve);
}

void Stream::Read(byte* buffer, int offset, int count) {
    std::memcpy(buffer + offset, m_buf.data() + m_pos, count);
    m_pos += count;
}

byte Stream::ReadByte() {
    byte b;
    Read(&b, 0, 1);
    return b;
}

void Stream::Read(std::vector<byte>& vec, int count) {
    vec.insert(vec.end(), m_buf.begin() + m_pos, m_buf.begin() + m_pos + count);
    m_pos += count;
}



void Stream::Write(const byte* buffer, int offset, int count) {
    m_buf.insert(m_buf.begin() + m_pos, buffer + offset, buffer + offset + count);
    m_pos += count;
}

void Stream::WriteByte(const byte value) {
    Write(&value, 0, 1);
}

void Stream::Write(const std::vector<byte>& vec, int count) {
    // buffer -> internal
    m_buf.insert(m_buf.begin() + m_pos, vec.begin(), vec.begin() + count);
    m_pos += count;
}

void Stream::ensureCapacity(int extra) {
    m_buf.reserve(m_buf.size() + extra);
}
