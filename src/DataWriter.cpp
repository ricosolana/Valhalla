#include "DataWriter.h"

void DataWriter::WriteBytes(const BYTE_t* buffer, int32_t count) {

    // this copies in place, without relocating bytes exceeding m_pos
    // resize, ensuring capacity for copy operation
    if (count < 0) 
        throw VUtils::data_error("negative count");

    if (static_cast<size_t>(m_pos) + static_cast<size_t>(count) > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw VUtils::data_error("int32_t size exceeded");

    if (Length() < m_pos + count)
        m_provider.get().resize(m_pos + count);

    std::copy(buffer, buffer + count, m_provider.get().begin() + m_pos);

    /*
    // This old way made m_pos the end of the buffer, trashing all bytes after it
    // trim everything else away
    m_buf.resize(m_pos);

    // push elements
    //m_buf.insert(m_buf.begin() + m_pos, buffer, buffer + count);
    m_buf.insert(m_buf.end(), buffer, buffer + count);
    */

    m_pos += count;
}

void DataWriter::SetPos(int32_t pos) {
    if (pos < 0)
        throw VUtils::data_error("negative pos");

    if (pos > Length() + 1)
        throw VUtils::data_error("position exceeds length");

    m_pos = pos;
}

void DataWriter::WriteChar(uint16_t i) {
    // simplest case
    if (i < 0x80) {
        Write<BYTE_t>(i);
    }
    else if (i < 0x0800) {
        Write<BYTE_t>(((i >> 6) & 0x1F) | 0xC0);
        Write<BYTE_t>(((i >> 0) & 0x3F) | 0x80);
    }
    else if (i < 0x010000) {
        Write<BYTE_t>(((i >> 12) & 0x0F) | 0xE0);
        Write<BYTE_t>(((i >> 6) & 0x3F) | 0x80);
        Write<BYTE_t>(((i >> 0) & 0x3F) | 0x80);
    }
    //else if (i < 0x110000) {
    //    Write<BYTE_t>(((i >> 18) & 0x07) | 0xF0);
    //    Write<BYTE_t>(((i >> 12) & 0x3F) | 0x80);
    //    Write<BYTE_t>(((i >> 6) & 0x3F) | 0x80);
    //    Write<BYTE_t>(((i >> 0) & 0x3F) | 0x80);
    //}
}



void DataWriter::Write(const BYTE_t* in, int32_t count) {
    Write(count);
    WriteBytes(in, count);
}

void DataWriter::Write(const BYTES_t& in, int32_t count) {
    Write(in.data(), count);
}

void DataWriter::Write(const BYTES_t& in) {
    if (in.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw VUtils::data_error("int32_t size exceeded");

    Write(in.data(), static_cast<int32_t>(in.size()));
}

//void DataWriter::Write(const NetPackage& in) {
//    Write(in.m_stream.m_buf);
//}

void DataWriter::Write(const std::string& in) {
    auto length = in.length();
    if (length > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw VUtils::data_error("int32_t size exceeded");

    auto byteCount = static_cast<int32_t>(length);

    Write7BitEncodedInt(byteCount);
    if (byteCount == 0)
        return;

    WriteBytes(reinterpret_cast<const BYTE_t*>(in.c_str()), byteCount);
}

void DataWriter::Write(const ZDOID& in) {
    Write(in.m_uuid);
    Write(in.m_id);
}

void DataWriter::Write(const Vector3& in) {
    Write(in.x);
    Write(in.y);
    Write(in.z);
}

void DataWriter::Write(const Vector2i& in) {
    Write(in.x);
    Write(in.y);
}

void DataWriter::Write(const Quaternion& in) {
    Write(in.x);
    Write(in.y);
    Write(in.z);
    Write(in.w);
}
