#include "ZPackage.hpp"
#include <assert.h>


//std::deque<std::unique_ptr<ZPackage>> unusedPackages;

//ZPackage* ZPackage::NewPkg() {
//    if (unusedPackages.empty()) {
//        for (int i = 0; i < 10; i++) {
//            unusedPackages.push_back(std::make_unique<ZPackage>());
//        }
//    }
//    auto&& front = unusedPackages.front();
//    unusedPackages.pop_front();
//    auto ptr = front.get();
//    usedPackages.push_back(front);
//    return ptr;
//}
//
//void ZPackage::ReturnPkg(ZPackage* pkg) {
//    unusedPackages.push_back
//    auto&& front1 = unusedPackages.front();
//    unusedPackages.pop_front();
//    return std::move(front1);
//}

//std::unique_ptr<ZPackage> ZPackage::New() {
//    return std::make_unique<
//}

ZPackage::ZPackage()
    : m_writer(m_stream), m_reader(m_stream) {

}

ZPackage::ZPackage(byte* data, int32_t count)
    : ZPackage() {
    Load(data, count);
}

ZPackage::ZPackage(std::vector<byte>& vec)
    : ZPackage(vec.data(), vec.size()) {}

ZPackage::ZPackage(int32_t reserve)
    : m_stream(reserve), m_writer(m_stream), m_reader(m_stream) {}



void ZPackage::Load(byte* data, int32_t count) {
    m_stream.m_pos = 0;
    m_stream.Write(data, 0, count);
}



void ZPackage::Write(const ZPackage& in) {
    Write(in.Bytes(), in.Size());
}

void ZPackage::WriteCompressed(const ZPackage& in) {
    throw std::runtime_error("not implemented");
}

void ZPackage::Write(const byte* in, int32_t count) {
    m_writer.Write(count);
	m_writer.Write(in, count);
}

void ZPackage::Write(const std::string& in) {
	m_writer.Write(in);
}

void ZPackage::Write(const std::vector<byte> & in) {
    Write(in.data(), static_cast<int32_t>(in.size()));
}

void ZPackage::Write(const std::vector<std::string>& in) {
    Write(static_cast<int32_t>(in.size()));
    for (auto&& s : in) {
        Write(s);
    }
}

void ZPackage::Write(const ZDOID& id)
{
    m_writer.Write(id.m_userID);
    m_writer.Write(id.m_id);
}

void ZPackage::Write(const Vector3& v3)
{
    m_writer.Write(v3.x);
    m_writer.Write(v3.y);
    m_writer.Write(v3.z);
}

void ZPackage::Write(const Vector2i& v2)
{
    m_writer.Write(v2.x);
    m_writer.Write(v2.y);
}

void ZPackage::Write(const Quaternion& q)
{
    m_writer.Write(q.x);
    m_writer.Write(q.y);
    m_writer.Write(q.z);
    m_writer.Write(q.w);
}




//std::string Package::ReadString() {
//    return m_reader.Read<std::string>();
//}

//ZPackage* ZPackage::ReadPackage() {
//    std::vector<byte> vec;
//    ReadByteArray(vec);
//    return new ZPackage(vec);
//}

//void Package::ReadPackage(Package* out) {
//    ReadByteArray(out->m_stream.Buffer());
//}
//
//void Package::ReadPackage(Package& out) {
//    ReadByteArray(out.m_stream.Buffer());
//}

void ZPackage::ReadByteArray(std::vector<byte>& out) {
    m_stream.Read(out, Read<int32_t>());
}

//void ZPackage::GetArray(std::vector<byte>& vec) {
//    // https://onlinegdb.com/uCR3uyzin
//    vec = m_stream.Buffer();
//}

void ZPackage::Read(std::vector<std::string>& out) {
    auto count = Read<int32_t>();
    out.reserve(count);

    while (count--) {
        out.push_back(Read<std::string>());
    }
}



byte* ZPackage::Bytes() const {
    return m_stream.Bytes();
}

int ZPackage::Size() const {
    return m_stream.m_pos;
}

void ZPackage::Clear() {
    m_stream.m_pos = 0;
}

//void ZPackage::SetPos(int32_t pos) {
//    m_stream.m_pos = (int64_t)pos;
//}
