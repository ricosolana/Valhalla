#include "DataReader.h"

void DataReader::ReadSomeBytes(BYTE_t* buffer, size_t count) {
    Assert31U(count);
    AssertOffset(count);

    // read into 'buffer'
    std::copy(m_provider.get().begin() + m_pos,
        m_provider.get().begin() + m_pos + count,
        buffer);

    m_pos += count;
}

void DataReader::ReadSomeBytes(BYTES_t& vec, size_t count) {
    Assert31U(count);
    AssertOffset(count);

    vec = BYTES_t(m_provider.get().begin() + m_pos,
        m_provider.get().begin() + m_pos + count);

    m_pos += count;
}

sol::variadic_results DataReader::DeserializeLua(sol::state_view state, DataReader& reader, const IModManager::Types& types) {
    sol::variadic_results results;
    
    for (auto&& type : types) {
        switch (type) {
        case IModManager::Type::BYTES:
            // Will be interpreted as sol container type
            // see https://sol2.readthedocs.io/en/latest/containers.html
            results.push_back(sol::make_object(state, reader.ReadBytes()));
            break;
        case IModManager::Type::STRING:
            // Primitive: string
            results.push_back(sol::make_object(state, reader.ReadString()));
            break;
        case IModManager::Type::ZDOID:
            // Userdata: ZDOID
            results.push_back(sol::make_object(state, reader.ReadZDOID()));
            break;
        case IModManager::Type::VECTOR3:
            // Userdata: Vector3
            results.push_back(sol::make_object(state, reader.ReadVector3()));
            break;
        case IModManager::Type::VECTOR2i:
            // Userdata: Vector2i
            results.push_back(sol::make_object(state, reader.ReadVector2i()));
            break;
        case IModManager::Type::QUATERNION:
            // Userdata: Quaternion
            results.push_back(sol::make_object(state, reader.ReadQuaternion()));
            break;
        case IModManager::Type::STRINGS:
            // Container type of Primitive: string
            results.push_back(sol::make_object(state, reader.ReadStrings()));
            break;
        case IModManager::Type::BOOL:
            // Primitive: boolean
            results.push_back(sol::make_object(state, reader.ReadBool()));
            break;
        case IModManager::Type::INT8:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.ReadInt8()));
            break;
        case IModManager::Type::INT16:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.ReadInt16()));
            break;
        case IModManager::Type::INT32:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.ReadInt32()));
            break;
        case IModManager::Type::INT64:
            // Userdata: Int64Wrapper
            results.push_back(sol::make_object(state, reader.ReadInt64()));
            break;
        case IModManager::Type::UINT8:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.ReadUInt8()));
            break;
        case IModManager::Type::UINT16:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.ReadUInt16()));
            break;
        case IModManager::Type::UINT32:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.ReadUInt32()));
            break;
        case IModManager::Type::UINT64:
            // Userdata: UInt64Wrapper
            results.push_back(sol::make_object(state, reader.ReadUInt64Wrapper()));
            break;
        case IModManager::Type::FLOAT:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.Read<float>()));
            break;
        case IModManager::Type::DOUBLE:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.Read<double>()));
            break;
        default:
            throw std::runtime_error("invalid mod DataReader type");
        }
    }

    return results;
}
