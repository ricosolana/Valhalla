#include "DataWriter.h"

void DataWriter::WriteBytes(const BYTE_t* buffer, size_t count) {
    Assert31U(count);

    Assert31U(m_pos + count);

    // this copies in place, without relocating bytes exceeding m_pos
    // resize, ensuring capacity for copy operation
    
    //vector::assign only works starting at beginning... so cant use it...
    if (CheckOffset(count))
        m_provider.get().resize(m_pos + count);

    std::copy(buffer, buffer + count, m_provider.get().begin() + m_pos);

    /*
    // Trash all bytes after position
    m_buf.resize(m_pos);

    // Effectively insert at m_pos (end of vector)
    m_buf.insert(m_buf.end(), buffer, buffer + count);
    */

    m_pos += count;
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
    else { // if (i < 0x010000) {
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



void DataWriter::Write(const BYTE_t* in, size_t count) {
    Write<int32_t>(count);
    WriteBytes(in, count);
}

void DataWriter::Write(const BYTES_t& in, size_t count) {
    Write(in.data(), count);
}

void DataWriter::Write(const BYTES_t& in) {
    Write(in.data(), in.size());
}

//void DataWriter::Write(const NetPackage& in) {
//    Write(in.m_stream.m_buf);
//}

void DataWriter::Write(const std::string& in) {
    auto length = in.length();
    //Assert31U(length);

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
