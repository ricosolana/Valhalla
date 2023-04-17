#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "DataStream.h"

class DataWriter;

class DataReader {
public:
    BYTE_VIEW_t m_buf;
    size_t m_pos{};

private:
    static bool Check31U(size_t count) {
        return count > static_cast<size_t>(std::numeric_limits<int32_t>::max());
    }

    // Throws if the count exceeds int32_t::max signed size
    static void Assert31U(size_t count) {
        if (Check31U(count))
            throw std::runtime_error("count is negative or exceeds 2^32 - 1");
    }

    // Returns whether the specified position exceeds container length
    bool CheckPosition(size_t pos) const {
        return pos > Length();
    }

    // Throws if the specified position exceeds container length
    void AssertPosition(size_t pos) const {
        if (CheckPosition(pos))
            throw std::runtime_error("position exceeds length");
    }

    // Returns whether the specified offset from m_pos exceeds container length
    bool CheckOffset(size_t offset) const {
        return CheckPosition(m_pos + offset);
    }

    // Throws if the specified offset from m_pos exceeds container length
    void AssertOffset(size_t offset) const {
        if (CheckOffset(offset))
            throw std::runtime_error("offset from position exceeds length");
    }

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
    DataReader(BYTE_VIEW_t buf) : m_buf(buf) {}

    DataReader(BYTE_VIEW_t buf, size_t pos) : m_buf(buf) {
        SetPos(pos);
    }

public:
    // Returns the length of this stream
    size_t Length() const {
        return m_buf.size();
    }

    // Returns the position of this stream
    size_t Position() const {
        return m_pos;
    }

    // Sets the positino of this stream
    void SetPos(size_t pos) {
        Assert31U(pos);
        AssertPosition(pos);

        m_pos = pos;
    }

    const BYTE_t* Data() const {
        return m_buf.data();
    }

public:
    //DataWriter ToWriter();

    template<typename T>
        requires std::is_same_v<T, BYTE_VIEW_t>
    decltype(auto) Read() {
        auto count = Read<int32_t>();

        Assert31U(count);
        AssertOffset(count);

        auto result = BYTE_VIEW_t(m_buf.begin() + m_pos, count);

        this->SetPos(m_pos + count);

        return result;
    }

    // Reads a byte array as a Reader (more efficient than ReadBytes())
    //  int32_t:   size
    //  BYTES_t:    data
    template<typename T>
        requires std::is_same_v<T, DataReader>
    decltype(auto) Read() {
        return DataReader(Read<BYTE_VIEW_t>());
        /*
        auto count = Read<int32_t>();

        Assert31U(count);
        AssertOffset(count);

        auto other = DataReader(BYTE_VIEW_t(m_buf.begin() + m_pos, count));
        this->SetPos(m_pos + count);
        return other;*/
    }



    //  Reads a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    decltype(auto) Read() {
        T out;
        ReadSomeBytes(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    template<typename T> 
        requires (std::same_as<T, std::string>
            || std::same_as<T, std::string_view>)
    decltype(auto) Read() {
        auto count = Read7BitEncodedInt();
        if (count == 0) return T(m_buf.begin() + m_pos, m_buf.begin() + m_pos);

        Assert31U(count);
        AssertOffset(count);

        T s = T(m_buf.begin() + m_pos,
            m_buf.begin() + m_pos + count);

        m_pos += count;
        return s;
    }

    // Reads a byte array
    //  int32_t:   size
    //  BYTES_t:    data
    template<typename T> requires std::same_as<T, BYTES_t>
    decltype(auto) Read() {
        T out{};
        ReadSomeBytes(out, Read<int32_t>());
        return out;
    }

    // Reads a container of supported types
    //  int32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
            && !std::is_same_v<typename Iterable::value_type, BYTE_t>)
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
        return Vector3f{ a, b, c };
    }

    // Reads a Vector2i
    //  8 bytes total are read:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    template<typename T> requires std::same_as<T, Vector2i>
    decltype(auto) Read() {
        auto a(Read<int32_t>());
        auto b(Read<int32_t>());
        return Vector2i(a, b);
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
};
