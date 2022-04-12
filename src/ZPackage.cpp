#include "ZPackage.hpp"
#include <zlib.h>

ZPackage::ZPackage(byte* data, int32_t count) {
    Load(data, count);
}

ZPackage::ZPackage(std::vector<byte>& vec)
    : ZPackage(vec.data(), vec.size()) {}

ZPackage::ZPackage(int32_t reserve)
    : m_stream(reserve) {}



void ZPackage::Write(const byte* in, int32_t count) {
    Write(count);
	m_stream.Write(in, count);
}

void ZPackage::Write(const std::string& in) {
    int byteCount = Utils::GetUnicode8Count(in.c_str());
    if (byteCount > 256)
        throw std::runtime_error("Writing big string not yet supported");

    // slight optimization
    m_stream.ReserveExtra(1 + in.length());

    Write7BitEncodedInt(byteCount);

    if (byteCount == 0)
        return;

    m_stream.Write(reinterpret_cast<const byte*>(in.c_str()), byteCount);
}

void ZPackage::Write(const std::vector<byte>& in) {
    Write(in.data(), static_cast<int32_t>(in.size()));
}

void ZPackage::Write(const std::vector<std::string>& in) {
    Write(static_cast<int32_t>(in.size()));
    for (auto&& s : in) {
        Write(s);
    }
}

void ZPackage::Write(const ZPackage& in) {
    Write(in.m_stream.Bytes(), in.m_stream.Length());
}

void ZPackage::Write(const ZDOID& in) {
    Write(in.m_userID);
    Write(in.m_id);
}

void ZPackage::Write(const Vector3& in) {
    Write(in.x);
    Write(in.y);
    Write(in.z);
}

void ZPackage::Write(const Vector2i& in) {
     Write(in.x);
     Write(in.y);
}

void ZPackage::Write(const Quaternion& in)
{
     Write(in.x);
     Write(in.y);
     Write(in.z);
     Write(in.w);
}



void ZPackage::Load(byte* data, int32_t count) {
    m_stream.Clear();
    m_stream.Write(data, 0, count);
}



ZPackage ZPackage::ReadCompressed() {
    int count = Read<int32_t>();

    //return ZPackage(Utils::Decompress(Read(count)));
    throw std::runtime_error("not implemented");
}

void ZPackage::WriteCompressed(const ZPackage& in) {
    throw std::runtime_error("not implemented");
}

void ZPackage::Read(std::vector<byte>& out) {
    m_stream.Read(out, Read<int32_t>());
}

void ZPackage::Read(std::vector<std::string>& out) {
    auto count = Read<int32_t>();
    out.reserve(count);

    while (count--) {
        out.push_back(Read<std::string>());
    }
}



void ZPackage::Write7BitEncodedInt(int in) {
    m_stream.ReserveExtra(4);
    unsigned int num;
    for (num = (unsigned int)in; num >= 128U; num >>= 7)
        Write((unsigned char)(num | 128U));

    Write((unsigned char)num);
}

int ZPackage::Read7BitEncodedInt() {
    int out = 0;
    int num2 = 0;
    while (num2 != 35)
    {
        auto b = Read<byte>();
        out |= (int)(b & 127) << num2;
        num2 += 7;
        if ((b & 128) == 0)
        {
            return out;
        }
    }
    throw std::runtime_error("bad encoded Int32");
}

