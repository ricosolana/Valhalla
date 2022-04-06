#include <stdexcept>
#include "BinaryReader.hpp"

BinaryReader::BinaryReader(Stream &stream)
	: m_stream(stream) {}

void BinaryReader::Read(byte* out, int offset, int count) {
    m_stream.Read(out, offset, count);
}

void BinaryReader::Read(byte* out, int count) {
    Read(out, 0, count);
}

void BinaryReader::Read(std::vector<byte>& out, int count) {
    m_stream.Read(out, count);
}

//template <> std::string BinaryReader::Read() {
//    auto byteCount = Read7BitEncodedInt();
//
//    if (byteCount == 0)
//        return "";
//
//    std::string out;
//    out.resize(byteCount);
//
//    Read(reinterpret_cast<byte*>(out.data()), byteCount);
//}

//std::string BinaryReader::Read() {
//    //value.
//    auto byteCount = Read7BitEncodedInt();
//
//    if (byteCount == 0)
//        return "";
//
//    std::string out;
//    out.resize(byteCount);
//
//    Read(reinterpret_cast<byte*>(out.data()), byteCount);
//}

int BinaryReader::Read7BitEncodedInt() {
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
