#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "ModManager.h"
#include "DataStream.h"

class DataReader : public DataStream {
private:
    void ReadSomeBytes(BYTE_t* buffer, size_t count) {
        this->AssertOffset(count);

        // read into 'buffer'
        std::copy(this->data() + this->m_pos,
            this->data() + this->m_pos + count,
            buffer);

        this->m_pos += count;
    }

    // Implementation of C#
    uint32_t Read7BitEncodedInt() {
        uint32_t out = 0;
        uint32_t num2 = 0;
        while (num2 != 35) {
            auto b = Read<uint8_t>();
            out |= static_cast<decltype(out)>(b & 127) << num2;
            num2 += 7;
            if ((b & 128) == 0)
            {
                return out;
            }
        }
        throw std::runtime_error("bad encoded int");
    }

public:
    explicit DataReader(BYTE_VIEW_t buf) : DataStream(buf) {}
    explicit DataReader(BYTES_t &buf) : DataStream(buf) {}
    explicit DataReader(BYTE_VIEW_t buf, size_t pos) : DataStream(buf, pos) {}
    explicit DataReader(BYTES_t &buf, size_t pos) : DataStream(buf, pos) {}

public:
    template<typename T>
        requires 
            (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>
                || std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
    decltype(auto) Read() {
        auto count = (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>) 
            ? Read<uint32_t>()
            : Read7BitEncodedInt();

        this->AssertOffset(count);

        auto result = T(reinterpret_cast<T::value_type*>(this->data()) + this->m_pos,
            reinterpret_cast<T::value_type*>(this->data()) + this->m_pos + count);

        this->SetPos(this->m_pos + count);

        return result;
    }

    
    // Reads a byte array as a Reader (more efficient than ReadBytes())
    //  int32_t:   size
    //  BYTES_t:    data
    template<typename T>
        requires std::is_same_v<T, DataReader>
    decltype(auto) Read() {
        return T(Read<BYTE_VIEW_t>());
    }



    //  Reads a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    decltype(auto) Read() {
        T out{};
        ReadSomeBytes(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    // Reads a container of supported types
    //  int32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
            && !std::is_arithmetic_v<typename Iterable::value_type>)
    decltype(auto) Read() {
        using Type = Iterable::value_type;

        const auto count = Read<int32_t>();

        Iterable out{};

        // TODO why is this here? should always reserve regardless
        //if constexpr (std::is_same_v<Type, std::string>)
        out.reserve(count);

        for (int32_t i=0; i < count; i++) {
            auto type = Read<Type>();
            out.insert(out.end(), type);
        }

        return out;
    }
    
    // Reads a container<T> and runs a function on each element
    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 1)
    void AsEach(F func) {
        using Type = std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>;

        const auto count = Read<int32_t>();

        for (int32_t i = 0; i < count; i++) {
            func(Read<Type>());
        }
    }

    // Reads a ZDOID
    //  12 bytes total are read:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    template<typename T> requires std::same_as<T, ZDOID>
    decltype(auto) Read() {
        auto a(Read<int64_t>());
        auto b(Read<uint32_t>());
        return ZDOID(a, b);
    }

    // Reads a Vector3f
    //  12 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    template<typename T> requires std::same_as<T, Vector3f>
    decltype(auto) Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        return Vector3f(a, b, c);
    }

    // Reads a Vector2s
    //  8 bytes total are read:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    template<typename T> requires std::same_as<T, Vector2i>
    decltype(auto) Read() {
        auto a(Read<int32_t>());
        auto b(Read<int32_t>());
        return Vector2i(a, b);
    }


    // Reads a Vector2s
    //  4 bytes total are read:
    //  int16_t: x (2 bytes)
    //  int16_t: y (2 bytes)
    template<typename T> requires std::same_as<T, Vector2s>
    decltype(auto) Read() {
        auto a(Read<int16_t>());
        auto b(Read<int16_t>());
        return Vector2s(a, b);
    }



    // Reads a Quaternion
    //  16 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    template<typename T> requires std::same_as<T, Quaternion>
    decltype(auto) Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        auto d(Read<float>());
        return Quaternion{ a, b, c, d };
    }

    // Reads an enum type
    //  - Bytes read depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    decltype(auto) Read() {
        return static_cast<Enum>(Read<std::underlying_type_t<Enum>>());
    }

    // Read a UTF-8 encoded C# char
    //  Will advance 1 -> 3 bytes, depending on size of first char
    template<typename T> requires std::is_same_v<T, char16_t>
    decltype(auto) Read() {
        auto b1 = Read<uint8_t>();

        // 3 byte
        if (b1 >= 0xE0) {
            auto b2 = Read<uint8_t>() & 0x3F;
            auto b3 = Read<uint8_t>() & 0x3F;
            return ((b1 & 0xF) << 12) | (b2 << 6) | b3;
        }
        // 2 byte
        else if (b1 >= 0xC0) {
            auto b2 = Read<uint8_t>() & 0x3F;
            return ((b1 & 0x1F) << 6) | b2;
        }
        // 1 byte
        else {
            return b1 & 0x7F;
        }
    }

    template<typename T> requires std::is_same_v<T, UserProfile>
    decltype(auto) Read() {
        auto name = Read<std::string>();
        auto gamerTag = Read<std::string>();
        auto networkUserId = Read<std::string>();

        return UserProfile(std::move(name), std::move(gamerTag), std::move(networkUserId));
    }

    decltype(auto) ReadNumItems() {
        uint32_t num = Read<BYTE_t>();
        if ((num & 128) != 0) 
            num = ((num & 127) << 8) | (uint32_t)Read<BYTE_t>();
        return num;
    }



    // verbose extension methods
    //  I want these to actually all be in lua
    //  templates in c, wrappers for lua in modman

    decltype(auto) ReadBool() { return Read<bool>(); }

    decltype(auto) ReadString() { return Read<std::string>(); }
    decltype(auto) ReadStrings() { return Read<std::vector<std::string>>(); }

    decltype(auto) ReadBytes() { return Read<BYTES_t>(); }

    decltype(auto) ReadZDOID() { return Read<ZDOID>(); }
    decltype(auto) ReadVector3f() { return Read<Vector3f>(); }
    decltype(auto) ReadVector2i() { return Read<Vector2i>(); }
    decltype(auto) ReadQuaternion() { return Read<Quaternion>(); }
    decltype(auto) ReadProfile() { return Read<UserProfile>(); }

    decltype(auto) ReadInt8() { return Read<int8_t>(); }
    decltype(auto) ReadInt16() { return Read<int16_t>(); }
    decltype(auto) ReadInt32() { return Read<int32_t>(); }
    decltype(auto) ReadInt64() { return Read<int64_t>(); }
    decltype(auto) ReadInt64Wrapper() { return (Int64Wrapper) Read<int64_t>(); }

    decltype(auto) ReadUInt8() { return Read<uint8_t>(); }
    decltype(auto) ReadUInt16() { return Read<uint16_t>(); }
    decltype(auto) ReadUInt32() { return Read<uint32_t>(); }
    decltype(auto) ReadUInt64() { return Read<uint64_t>(); }
    decltype(auto) ReadUInt64Wrapper() { return (UInt64Wrapper) Read<uint64_t>(); }

    decltype(auto) ReadFloat() { return Read<float>(); }
    decltype(auto) ReadDouble() { return Read<double>(); }
    
    decltype(auto) ReadChar() { return Read<char16_t>(); }



    // Deserialize a reader to a tuple of types
    template<class...Ts, class RD>
    static std::tuple<Ts...> Deserialize(RD& reader) {
        return { reader.template Read<Ts>()... };
    }


#if VH_IS_ON(VH_USE_MODS)
    sol::object DeserializeOneLua(sol::state_view state, IModManager::Type type) {
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
            return sol::make_object(state, ReadFloat());
        case IModManager::Type::DOUBLE:
            // Primitive: number
            return sol::make_object(state, ReadDouble());
        default:
            throw std::runtime_error("invalid mod DataReader type");
        }
    }

    sol::variadic_results DeserializeLua(sol::state_view state, const IModManager::Types& types) {
        sol::variadic_results results;

        for (auto&& type : types) {
            results.push_back(DeserializeOneLua(state, type));
        }

        return results;
    }
#endif
};
