#include "DataWriter.h"
#include "VUtilsString.h"
#include "DataReader.h"

void DataWriter::WriteSomeBytes(const BYTE_t* buffer, size_t count) {
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



DataReader DataWriter::ToReader() {
    return DataReader(this->m_provider.get(), this->m_pos);
}

void DataWriter::Write(const BYTE_t* in, size_t count) {
    Write<int32_t>(count);
    WriteSomeBytes(in, count);
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

void DataWriter::Write(std::string_view in) {
    auto length = in.length();
    //Assert31U(length);

    //auto byteCount = static_cast<int32_t>(VUtils::String::GetUTF8ByteCount());

    auto byteCount = static_cast<int32_t>(length);

    Write7BitEncodedInt(byteCount);
    if (byteCount == 0)
        return;

    WriteSomeBytes(reinterpret_cast<const BYTE_t*>(in.data()), byteCount);
}

void DataWriter::Write(const ZDOID& in) {
    Write(in.m_uuid);
    Write(in.m_id);
}

void DataWriter::Write(const Vector3f& in) {
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


void DataWriter::SerializeOneLua(IModManager::Type type, sol::object arg) {
    switch (type) {
        // TODO add recent unsigned types
    case IModManager::Type::UINT8:
        Write(arg.as<uint8_t>());
        break;
    case IModManager::Type::UINT16:
        Write(arg.as<uint16_t>());
        break;
    case IModManager::Type::UINT32:
        Write(arg.as<uint32_t>());
        break;
    case IModManager::Type::UINT64:
        Write(arg.as<uint64_t>());
        break;
    case IModManager::Type::INT8:
        Write(arg.as<int8_t>());
        break;
    case IModManager::Type::INT16:
        Write(arg.as<int16_t>());
        break;
    case IModManager::Type::INT32:
        Write(arg.as<int32_t>());
        break;
    case IModManager::Type::INT64:
        Write(arg.as<int64_t>());
        break;
    case IModManager::Type::FLOAT:
        Write(arg.as<float>());
        break;
    case IModManager::Type::DOUBLE:
        Write(arg.as<double>());
        break;
    case IModManager::Type::STRING:
        Write(arg.as<std::string_view>());
        break;
    case IModManager::Type::BOOL:
        Write(arg.as<bool>());
        break;
    case IModManager::Type::BYTES:
        Write(arg.as<BYTES_t>());
        break;
    case IModManager::Type::ZDOID:
        Write(arg.as<ZDOID>());
        break;
    case IModManager::Type::VECTOR3f:
        Write(arg.as<Vector3f>());
        break;
    case IModManager::Type::VECTOR2i:
        Write(arg.as<Vector2i>());
        break;
    case IModManager::Type::QUATERNION:
        Write(arg.as<Quaternion>());
        break;
    default:
        throw std::runtime_error("Invalid data type");
    }
}

void DataWriter::SerializeLua(const IModManager::Types& types, const sol::variadic_results &results) {
    for (int i = 0; i < results.size(); i++) {
        SerializeOneLua(types[i], results[i]);
    }
}
