#include "DataReader.h"
#include "DataWriter.h"

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

DataWriter DataReader::ToWriter() {
    return DataWriter(this->m_provider, this->m_pos);
}



sol::object DataReader::DeserializeOneLua(sol::state_view state, IModManager::Type type) {
    switch (type) {
    case IModManager::Type::BYTES:
        // Will be interpreted as sol container type
        // see https://sol2.readthedocs.io/en/latest/containers.html
        return sol::make_object(state, ReadBytes());
    case IModManager::Type::STRING:
        // Primitive: string
        return sol::make_object(state, ReadString());
    case IModManager::Type::ZDOID:
        // Userdata: ZDOID
        return sol::make_object(state, ReadZDOID());
    case IModManager::Type::VECTOR3f:
        // Userdata: Vector3f
        return sol::make_object(state, ReadVector3f());
    case IModManager::Type::VECTOR2i:
        // Userdata: Vector2i
        return sol::make_object(state, ReadVector2i());
    case IModManager::Type::QUATERNION:
        // Userdata: Quaternion
        return sol::make_object(state, ReadQuaternion());
    case IModManager::Type::STRINGS:
        // Container type of Primitive: string
        return sol::make_object(state, ReadStrings());
    case IModManager::Type::BOOL:
        // Primitive: boolean
        return sol::make_object(state, ReadBool());
    case IModManager::Type::INT8:
        // Primitive: number
        return sol::make_object(state, ReadInt8());
    case IModManager::Type::INT16:
        // Primitive: number
        return sol::make_object(state, ReadInt16());
    case IModManager::Type::INT32:
        // Primitive: number
        return sol::make_object(state, ReadInt32());
    case IModManager::Type::INT64:
        // Userdata: Int64Wrapper
        return sol::make_object(state, ReadInt64());
    case IModManager::Type::UINT8:
        // Primitive: number
        return sol::make_object(state, ReadUInt8());
    case IModManager::Type::UINT16:
        // Primitive: number
        return sol::make_object(state, ReadUInt16());
    case IModManager::Type::UINT32:
        // Primitive: number
        return sol::make_object(state, ReadUInt32());
    case IModManager::Type::UINT64:
        // Userdata: UInt64Wrapper
        return sol::make_object(state, ReadUInt64Wrapper());
    case IModManager::Type::FLOAT:
        // Primitive: number
        return sol::make_object(state, Read<float>());
    case IModManager::Type::DOUBLE:
        // Primitive: number
        return sol::make_object(state, Read<double>());
    default:
        throw std::runtime_error("invalid mod DataReader type");
    }
}

sol::variadic_results DataReader::DeserializeLua(sol::state_view state, const IModManager::Types& types) {
    sol::variadic_results results;
    
    for (auto&& type : types) {
        results.push_back(DeserializeOneLua(state, type));
    }

    return results;
}
