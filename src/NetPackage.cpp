#include "NetPackage.h"

NetPackage::NetPackage(const BYTE_t* data, uint32_t count) 
    : m_stream(data, count) {}

NetPackage::NetPackage(BYTES_t &&vec)
    : m_stream(std::move(vec)) {}

NetPackage::NetPackage(const BYTES_t& vec)
    : NetPackage(vec.data(), static_cast<uint32_t>(vec.size())) {}

NetPackage::NetPackage(const BYTES_t& vec, uint32_t count)
    : NetPackage(vec.data(), count) {}

NetPackage::NetPackage(uint32_t reserve)
    : m_stream(reserve) {}



NetPackage& NetPackage::operator=(const NetPackage& other) {
    this->m_stream = other.m_stream;
    return *this;
}



void NetPackage::Write(const BYTE_t* in, uint32_t count) {
    Write(count);
    m_stream.Write(in, count);
}

void NetPackage::Write(const BYTES_t &in, uint32_t count) {
    Write(in.data(), count);
}

void NetPackage::Write(const BYTES_t& in) {
    assert(in.size() < std::numeric_limits<int32_t>::max());
    Write(in.data(), in.size());
}

void NetPackage::Write(const NetPackage& in) {
    Write(in.m_stream.m_buf);
}

void NetPackage::Write(const std::string& in) {
    assert(in.size() < std::numeric_limits<int32_t>::max());

    uint32_t byteCount = in.length();
    Write7BitEncodedInt(byteCount);
    if (byteCount == 0)
        return;

    m_stream.Write(reinterpret_cast<const BYTE_t*>(in.c_str()), byteCount);
}

void NetPackage::Write(const ZDOID& in) {
    Write(in.m_uuid);
    Write(in.m_id);
}

void NetPackage::Write(const Vector3& in) {
    Write(in.x);
    Write(in.y);
    Write(in.z);
}

void NetPackage::Write(const Vector2i& in) {
     Write(in.x);
     Write(in.y);
}

void NetPackage::Write(const Quaternion& in) {
     Write(in.x);
     Write(in.y);
     Write(in.z);
     Write(in.w);
}



void NetPackage::Read(BYTES_t& out) {
    m_stream.Read(out, Read<uint32_t>());
}

void NetPackage::Read(NetPackage &out) {
    out = NetPackage(Read<BYTES_t>());
}



void NetPackage::Write7BitEncodedInt(uint32_t in) {
    for (; in >= 128U; in >>= 7)
        Write((BYTE_t)(in | 128U));

    Write((BYTE_t) in);
}

uint32_t NetPackage::Read7BitEncodedInt() {
    uint32_t out = 0;
    uint32_t num2 = 0;
    while (num2 != 35) {
        auto b = Read<BYTE_t>();
        out |= (uint32_t)(b & 127) << num2;
        num2 += 7;
        if ((b & 128) == 0)
        {
            return out;
        }
    }
    throw std::runtime_error("bad encoded int");
}
