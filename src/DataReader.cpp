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
    
    // Pass the rpc always
    //results.push_back(sol::make_object(state, t));

    for (auto&& type : types) {
        switch (type) {
        case IModManager::Type::BYTES:
            // Will be interpreted as sol container type
            // see https://sol2.readthedocs.io/en/latest/containers.html
            results.push_back(sol::make_object(state, reader.Read<BYTES_t>()));
            break;
        case IModManager::Type::STRING:
            // Primitive: string
            results.push_back(sol::make_object(state, reader.Read<std::string>()));
            break;
        case IModManager::Type::ZDOID:
            results.push_back(sol::make_object(state, reader.Read<ZDOID>()));
            break;
        case IModManager::Type::VECTOR3:
            results.push_back(sol::make_object(state, reader.Read<Vector3>()));
            break;
        case IModManager::Type::VECTOR2i:
            results.push_back(sol::make_object(state, reader.Read<Vector2i>()));
            break;
        case IModManager::Type::QUATERNION:
            results.push_back(sol::make_object(state, reader.Read<Quaternion>()));
            break;
        case IModManager::Type::STRINGS:
            // Container type of Primitive: string
            results.push_back(sol::make_object(state, reader.Read<std::vector<std::string>>()));
            break;
        case IModManager::Type::BOOL:
            // Primitive: boolean
            results.push_back(sol::make_object(state, reader.Read<bool>()));
            break;
        case IModManager::Type::INT8:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.Read<int8_t>()));
            break;
        case IModManager::Type::INT16:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.Read<int16_t>()));
            break;
        case IModManager::Type::INT32:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.Read<int32_t>()));
            break;
        case IModManager::Type::INT64:
            // Primitive: number
            results.push_back(sol::make_object(state, reader.Read<int64_t>()));
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
            //LOG(FATAL) << "LUA MethodImpl bad DataType; this should have been impossible due to plugin register(handler) checking types for validity";
        }
    }

    return results;
}
