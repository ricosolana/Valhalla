#include "DataReader.h"

void DataReader::ReadBytes(BYTE_t* buffer, size_t count) {
    Assert31U(count);
    AssertOffset(count);

    // read into 'buffer'
    std::copy(m_provider.get().begin() + m_pos,
        m_provider.get().begin() + m_pos + count,
        buffer);

    m_pos += count;
}

//BYTE_t DataReader::ReadByte() {
//    BYTE_t b;
//    ReadBytes(&b, 1);
//    return b;
//}

void DataReader::ReadBytes(BYTES_t& vec, size_t count) {
    Assert31U(count);
    AssertOffset(count);

    vec = BYTES_t(m_provider.get().begin() + m_pos,
        m_provider.get().begin() + m_pos + count);

    m_pos += count;
}

uint16_t DataReader::ReadChar() {
    auto b1 = Read<BYTE_t>();

    // 3 byte
    if (b1 >= 0xE0) {
        auto b2 = Read<BYTE_t>() & 0x3F;
        auto b3 = Read<BYTE_t>() & 0x3F;
        return ((b1 & 0xF) << 12) | (b2 << 6) | b3;
    }
    // 2 byte
    else if (b1 >= 0xC0) {
        auto b2 = Read<BYTE_t>() & 0x3F;
        return ((b1 & 0x1F) << 6) | b2;
    }
    // 1 byte
    else {
        return b1 & 0x7F;
    }
}
