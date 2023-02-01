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
    Write(in.data(), static_cast<uint32_t>(in.size()));
}

void NetPackage::Write(const NetPackage& in) {
    Write(in.m_stream.m_buf);
}

void NetPackage::Write(const std::string& in) {
    int byteCount = static_cast<int>(in.length());

    assert(byteCount >= 0);

    Write7BitEncodedInt(byteCount);

    if (byteCount == 0)
        return;

    m_stream.Write(reinterpret_cast<const BYTE_t*>(in.c_str()), byteCount);
}

void NetPackage::Write(const NetID& in) {
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

void NetPackage::Write(const std::vector<std::string>& in) {
    Write(static_cast<int32_t>(in.size()));
    for (auto&& s : in) {
        Write(s);
    }
}

void NetPackage::Write(const robin_hood::unordered_set<std::string>& in) {
    Write(static_cast<int32_t>(in.size()));
    for (auto&& s : in) {
        Write(s);
    }
}



//void NetPackage::From(const BYTE_t* data, uint32_t count) {
//    m_stream.Clear();
//    m_stream.Write(data, count);
//    m_stream.SetPos(0);
//}



void NetPackage::Read(BYTES_t& out) {
    m_stream.Read(out, Read<uint32_t>());
}

/*
void NetPackage::Read(std::vector<std::string>& out) {
    auto count = Read<int32_t>();
    out.reserve(count);
    
    out.clear();
    while (count--) {
        out.push_back(Read<std::string>());
    }
}*/

void NetPackage::Read(NetPackage &out) {
    out = NetPackage(Read<BYTES_t>());
}



void NetPackage::Write7BitEncodedInt(int32_t in) {
    //m_stream.m_buf.reserve(m_stream.m_buf.size() + 4);

    uint32_t num;
    for (num = (uint32_t) in; num >= 128U; num >>= 7)
        Write((BYTE_t)(num | 128U));

    Write((BYTE_t) num);
}

int NetPackage::Read7BitEncodedInt() {
    int out = 0;
    int num2 = 0;
    while (num2 != 35) {
        auto b = Read<BYTE_t>();
        out |= (int)(b & 127) << num2;
        num2 += 7;
        if ((b & 128) == 0)
        {
            return out;
        }
    }
    throw std::runtime_error("bad encoded int");
}
