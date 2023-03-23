#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "DataStream.h"
#include "ModManager.h"

class DataReader : public DataStream {
private:
    void ReadSomeBytes(BYTE_t* buffer, size_t count);

    // Read a single byte from the buffer (slow if used to read a bunch of bytes)
    // Throws if end of buffer is reached
    //BYTE_t ReadByte();

    // Reads count bytes overriding the specified vector
    // Throws if not enough bytes to read
    void ReadSomeBytes(BYTES_t& vec, size_t count);

    // Reads count bytes overriding the specified string
    // '\0' is not included in count (raw bytes only)
    // Will throw if count exceeds end
    //void Read(std::string& s, uint32_t count);

    int32_t Read7BitEncodedInt() {
        int32_t out = 0;
        int32_t num2 = 0;
        while (num2 != 35) {
            auto b = Read<BYTE_t>();
            out |= (int32_t)(b & 127) << num2;
            num2 += 7;
            if ((b & 128) == 0)
            {
                return out;
            }
        }
        throw std::runtime_error("bad encoded int");
    }

public:
    DataReader(BYTES_t& bytes) : DataStream(bytes) {}

    DataReader(BYTES_t& bytes, size_t pos) : DataStream(bytes) {
        SetPos(pos);
    }

public:
    // Reads a byte array as a Reader (more efficient than ReadBytes())
    //  int32_t:   size
    //  BYTES_t:    data
    DataReader SubRead() {
        auto count = Read<int32_t>();

        Assert31U(count);

        auto other = DataReader(m_provider.get(), Position());
        this->SetPos(m_pos + count);
        return other;
    }

    //  Reads a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    decltype(auto) Read() {
        T out;
        ReadSomeBytes(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    // Reads a string
    template<typename T> requires std::same_as<T, std::string>
    decltype(auto) Read() {
        auto count = Read7BitEncodedInt();
        if (count == 0) return std::string();

        Assert31U(count);
        AssertOffset(count);

        std::string s = std::string(m_provider.get().begin() + m_pos,
            m_provider.get().begin() + m_pos + count);

        m_pos += count;
        return s;
    }

    // Reads a byte array
    //  int32_t:   size
    //  BYTES_t:    data
    template<typename T> requires std::same_as<T, BYTES_t>
    decltype(auto) Read() {
        T out;
        ReadSomeBytes(out, Read<int32_t>());
        return out;
    }

    // Reads a container of supported types
    //  int32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> && !std::is_same_v<Iterable, std::string> && !std::is_same_v<Iterable, BYTES_t>)
    decltype(auto) Read() {
        const auto count = Read<int32_t>();
        Assert31U(count);

        using Type = Iterable::value_type;

        Iterable out;

        if constexpr (std::is_same_v<Type, std::string>)
            out.reserve(count);

        for (int32_t i=0; i < count; i++) {
            auto type = Read<Type>();
            out.insert(out.end(), type);
        }

        return out;
    }

    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 1)
        //requires (std::is_same_typename VUtils::Traits::func_traits<F>::args_type)
    void AsEach(F func) {
        using Type = std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>;

        const auto count = Read<int32_t>();
        Assert31U(count);

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

    // Reads a Vector3
    //  12 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    template<typename T> requires std::same_as<T, Vector3>
    decltype(auto) Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        return Vector3{ a, b, c };
    }

    // Reads a Vector2i
    //  8 bytes total are read:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    template<typename T> requires std::same_as<T, Vector2i>
    decltype(auto) Read() {
        auto a(Read<int32_t>());
        auto b(Read<int32_t>());
        return Vector2i{ a, b };
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
        auto b1 = Read<BYTE_t>();

        // 3 byte
        if (b1 >= 0xE0) {
            auto b2 = Read<BYTE_t>() & 0x3F;
            auto b3 = Read<BYTE_t>() & 0x3F;
            return ((b1 & 0xF) << 12) | (b2 << 6) | b3;
        }
        // 2 byte
        else if (b1 >= 0xC0) {
            auto b2 = Read<BYTE_t>() & 0x3F;
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

        return UserProfile(name, gamerTag, networkUserId);
    }



    // Deserialize a reader to a tuple of types
    template<class...Ts, class RD>
    static std::tuple<Ts...> Deserialize(RD& reader) {
        return { reader.template Read<Ts>()... };
    }

    static sol::variadic_results DeserializeLua(sol::state_view state, DataReader& reader, const IModManager::Types& types);

    // verbose extension methods
    //  I want these to actually all be in lua
    //  templates in c, wrappers for lua in modman

    decltype(auto) ReadBool() { return Read<bool>(); }

    decltype(auto) ReadString() { return Read<std::string>(); }
    decltype(auto) ReadStrings() { return Read<std::vector<std::string>>(); }

    decltype(auto) ReadBytes() { return Read<BYTES_t>(); }

    decltype(auto) ReadZDOID() { return Read<ZDOID>(); }
    decltype(auto) ReadVector3() { return Read<Vector3>(); }
    decltype(auto) ReadVector2i() { return Read<Vector2i>(); }
    decltype(auto) ReadQuaternion() { return Read<Quaternion>(); }

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
};
