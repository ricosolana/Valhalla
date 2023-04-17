#include "DataReader.h"
#include "DataWriter.h"

void DataReader::ReadSomeBytes(BYTE_t* buffer, size_t count) {
    Assert31U(count);
    AssertOffset(count);

    // read into 'buffer'
    std::copy(m_buf.begin() + m_pos,
        m_buf.begin() + m_pos + count,
        buffer);

    m_pos += count;
}

void DataReader::ReadSomeBytes(BYTES_t& vec, size_t count) {
    Assert31U(count);
    AssertOffset(count);

    vec = BYTES_t(m_buf.begin() + m_pos,
        m_buf.begin() + m_pos + count);

    m_pos += count;
}

