#include "DataReader.h"

void DataReader::ReadBytes(BYTE_t* buffer, int32_t count) {
    if (count < 0) 
        throw std::runtime_error("negative count");

    if (static_cast<size_t>(m_pos) + static_cast<size_t>(count) > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw std::runtime_error("int32_t size exceeded");

    if (m_pos + count > Length()) 
        throw std::runtime_error("read exceeds length");

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

void DataReader::ReadBytes(std::vector<BYTE_t>& vec, int32_t count) {
    if (count < 0) 
        throw std::runtime_error("negative count");

    if (static_cast<size_t>(m_pos) + static_cast<size_t>(count) > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw std::runtime_error("int32_t size exceeded");

    if (m_pos + count > Length()) 
        throw std::runtime_error("read exceeds length");

    vec.clear();
    vec.insert(vec.begin(),
        m_provider.get().begin() + m_pos,
        m_provider.get().begin() + m_pos + count);

    m_pos += count;
}

void DataReader::SetPos(int32_t pos) {
    if (pos < 0) 
        throw std::runtime_error("negative pos");

    if (pos > Length() + 1) 
        throw std::runtime_error("position exceeds length");

    m_pos = pos;
}
