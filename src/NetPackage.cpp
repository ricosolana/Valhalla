#include "NetPackage.h"
#include "NetSync.h"


NetPackage::NetPackage(byte_t* data, uint32_t count) {
    From(data, count);
}

NetPackage::NetPackage(std::vector<byte_t>& vec)
    : NetPackage(vec.data(), static_cast<uint32_t>(vec.size())) {}

NetPackage::NetPackage(uint32_t reserve)
    : m_stream(reserve) {}



void NetPackage::Write(const byte_t* in, uint32_t count) {
    Write(count);
	m_stream.Write(in, count);
}

void NetPackage::Write(const std::string& in) {
    int byteCount = static_cast<int>(in.length());

    m_stream.ReserveExtra(1 + in.length());

    Write7BitEncodedInt(byteCount);

    if (byteCount == 0)
        return;

    m_stream.Write(reinterpret_cast<const byte_t*>(in.c_str()), byteCount);
}

void NetPackage::Write(const std::vector<byte_t>& in) {
    Write(in.data(), static_cast<int32_t>(in.size()));
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

void NetPackage::Write(const NetPackage::Ptr in) {
    Write(in->m_stream.Bytes(), in->m_stream.Length());
}

void NetPackage::Write(const NetSync::ID& in) {
    Write(in.m_userID);
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



void NetPackage::From(byte_t* data, int32_t count) {
    m_stream.Clear();
    m_stream.Write(data, count);
}



void NetPackage::Read(std::vector<byte_t>& out) {
    m_stream.Read(out, Read<int32_t>());
}

void NetPackage::Read(std::vector<std::string>& out) {
    auto count = Read<int32_t>();
    out.reserve(count);
    
    while (count--) {
        out.push_back(Read<std::string>());
    }
}



void NetPackage::Write7BitEncodedInt(int in) {
    m_stream.ReserveExtra(4);
    unsigned int num;
    for (num = (unsigned int)in; num >= 128U; num >>= 7)
        Write((unsigned char)(num | 128U));

    Write((unsigned char)num);
}

int NetPackage::Read7BitEncodedInt() {
    int out = 0;
    int num2 = 0;
    while (num2 != 35)
    {
        auto b = Read<byte_t>();
        out |= (int)(b & 127) << num2;
        num2 += 7;
        if ((b & 128) == 0)
        {
            return out;
        }
    }
    throw std::runtime_error("bad encoded int");
}

