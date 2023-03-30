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
    return DataReader(this->m_provider, this->m_pos);
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

void DataWriter::Write(const std::string& in) {
    auto length = in.length();
    //Assert31U(length);

    //auto byteCount = static_cast<int32_t>(VUtils::String::GetUTF8ByteCount());

    auto byteCount = static_cast<int32_t>(length);

    Write7BitEncodedInt(byteCount);
    if (byteCount == 0)
        return;

    WriteSomeBytes(reinterpret_cast<const BYTE_t*>(in.c_str()), byteCount);
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

void DataWriter::SerializeLuaImpl(DataWriter& params, const IModManager::Types&types, const sol::variadic_results &results) {
    for (int i = 0; i < results.size(); i++) {
        auto&& arg = results[i];
        auto argType = arg.get_type();
        auto&& expectType = types[i];

        // A vector of types and vector of values are used because Lua makes no distinction
        if (argType == sol::type::number) {
            switch (expectType) {
                // TODO add recent unsigned types
            case IModManager::Type::UINT8:
                params.Write(arg.as<uint8_t>());
                break;
            case IModManager::Type::UINT16:
                params.Write(arg.as<uint16_t>());
                break;
            case IModManager::Type::UINT32:
                params.Write(arg.as<uint32_t>());
                break;
            case IModManager::Type::UINT64:
                params.Write(arg.as<uint64_t>());
                break;
            case IModManager::Type::INT8:
                params.Write(arg.as<int8_t>());
                break;
            case IModManager::Type::INT16:
                params.Write(arg.as<int16_t>());
                break;
            case IModManager::Type::INT32:
                params.Write(arg.as<int32_t>());
                break;
            case IModManager::Type::INT64:
                params.Write(arg.as<int64_t>());
                break;
            case IModManager::Type::FLOAT:
                params.Write(arg.as<float>());
                break;
            case IModManager::Type::DOUBLE:
                params.Write(arg.as<double>());
                break;
            default:
                throw std::runtime_error("incorrect type at position (or bad DataFlag?)");
            }
        }
        else if (argType == sol::type::string && expectType == IModManager::Type::STRING) {
            params.Write(arg.as<std::string>());
        }
        else if (argType == sol::type::boolean && expectType == IModManager::Type::BOOL) {
            params.Write(arg.as<bool>());
        }
        // TODO can remove checks as sol safe mode already checks
        else if (arg.is<BYTES_t>() && expectType == IModManager::Type::BYTES) {
            params.Write(arg.as<BYTES_t>());
        }
        else if (arg.is<ZDOID>() && expectType == IModManager::Type::ZDOID) {
            params.Write(arg.as<ZDOID>());
        }
        else if (arg.is<Vector3f>() && expectType == IModManager::Type::VECTOR3f) {
            params.Write(arg.as<Vector3f>());
        }
        else if (arg.is<Vector2i>() && expectType == IModManager::Type::VECTOR2i) {
            params.Write(arg.as<Vector2i>());
        }
        else if (arg.is<Quaternion>() && expectType == IModManager::Type::QUATERNION) {
            params.Write(arg.as<Quaternion>());
        }
        else {
            throw std::runtime_error("unsupported type, or incorrect type at position");
        }
    }
}
