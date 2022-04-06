#include "NetPackage.hpp"
#include <assert.h>

Package::Package() 
    : m_writer(m_stream), m_reader(m_stream) {

}

Package::Package(byte* data, int count)
    : Package() {
    m_stream.Write(data, 0, count);
    m_stream.m_pos = 0;
}

Package::Package(std::vector<byte>& vec)
    : Package(vec.data(), vec.size()) {}

Package::Package(int reserve)
    : m_stream(reserve), m_writer(m_stream), m_reader(m_stream) {}



void Package::Load(byte* data, int count) {
    Clear();
    m_stream.Write(data, 0, count);
    m_stream.m_pos = 0;
}



void Package::Write(Package& in) {
    Write(in.m_stream.Buffer());
}

void Package::WriteCompressed(Package& in) {
    throw std::runtime_error("not implemented");
}

void Package::Write(const byte* in, int count) {
    m_writer.Write(count);
	m_writer.Write(in, count);
}

void Package::Write(const std::string& in) {
	m_writer.Write(in);
}

void Package::Write(const std::vector<byte> & in) {
    Write(in.data(), static_cast<int>(in.size()));
}

void Package::Write(const std::vector<std::string>& in) {
    Write(static_cast<int>(in.size()));
    for (auto&& s : in) {
        Write(s);
    }
}



//std::string Package::ReadString() {
//    return m_reader.Read<std::string>();
//}

//std::unique_ptr<Package> Package::ReadPackage() {
//    std::vector<byte> vec;
//    ReadByteArray(vec);
//    return std::make_unique<Package>(vec);
//}

void Package::ReadPackage(Package& out) {
    ReadByteArray(out.m_stream.Buffer());
}

void Package::ReadByteArray(std::vector<byte>& out) {
    m_stream.Read(out, Read<int>());
}

void Package::GetArray(std::vector<byte>& vec) {
    // https://onlinegdb.com/uCR3uyzin
    vec = m_stream.Buffer();
}

void Package::Read(std::vector<std::string>& out) {
    auto count = Read<int>();
    out.reserve(count);

    while (count--) {
        out.push_back(Read<std::string>());
    }
}



std::vector<byte>& Package::Buffer() {
    return m_stream.Buffer();
}

int Package::Size() {
    return m_stream.Buffer().size();
}

void Package::Clear() {
    m_stream.Buffer().clear();
}

void Package::SetPos(int pos) {
    m_stream.m_pos = (long long)pos;
}
