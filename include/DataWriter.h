#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "DataStream.h"
#include "ModManager.h"

class DataReader;

/*
template <class T>
struct Serializer;

template <class T>
struct Serializer
    : _Conditionally_enabled_hash<_Kty,
    !is_const_v<_Kty> && !is_volatile_v<_Kty> && (is_enum_v<_Kty> || is_integral_v<_Kty> || is_pointer_v<_Kty>)> {
    // hash functor primary template (handles enums, integrals, and pointers)
    static size_t _Do_hash(const _Kty& _Keyval) noexcept {
        return _Hash_representation(_Keyval);
    }
};*/

class DataWriter {
public:
    std::reference_wrapper<BYTES_t> m_provider;
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
    // Write count bytes from the specified buffer
    // Bytes are written in place, making space as necessary
    void WriteSomeBytes(const BYTE_t* buffer, size_t count);

    // Write count bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteSomeBytes(const BYTES_t& vec, size_t count) {
        return WriteSomeBytes(vec.data(), count);
    }

    // Write all bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteSomeBytes(const BYTES_t& vec) {
        return WriteSomeBytes(vec, vec.size());
    }

    void Write7BitEncodedInt(int32_t value) {
        auto num = static_cast<uint32_t>(value);
        for (; num >= 128U; num >>= 7)
            Write((BYTE_t)(num | 128U));

        Write((BYTE_t)num);
    }

public:
    DataWriter(BYTES_t& bytes) : m_provider(bytes) {}

    DataWriter(BYTES_t& bytes, size_t pos) : m_provider(bytes) {
        SetPos(pos);
    }

    // Clears the underlying container and resets position
    void Clear() {
        m_pos = 0;
        m_provider.get().clear();
    }

    // Sets the length of this stream
    //void SetLength(uint32_t length) {
    //    m_provider.get().resize(length);
    //}

public:
    virtual size_t Length() const {
        return m_provider.get().size();
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

public:
    DataReader ToReader();

    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 0)
    void SubWrite(F func) {
        const auto start = Position();
        int32_t count = 0;
        Write(count);

        // call func...
        func();

        const auto end = Position();
        SetPos(start);
        count = end - start - sizeof(count);
        assert(count >= 0);
        Write(count);
        SetPos(end);
    }

    // Writes a BYTE_t* as byte array of length
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTE_t* in, size_t count);

    // Writes a BYTES_t* as byte array of length
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTES_t& in, size_t count);

    // Writes a BYTE_t* as byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTES_t& in);

    // Writes a NetPackage as byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    //void Write(const NetPackage& in);

    // Writes a string
    void Write(std::string_view in);

    // Writes a ZDOID
    //  12 bytes total are written:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    void Write(const ZDOID& id);

    // Writes a Vector3f
    //  12 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    void Write(const Vector3f& in);

    // Writes a Vector2i
    //  8 bytes total are written:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    void Write(const Vector2i& in);

    // Writes a Quaternion
    //  16 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    void Write(const Quaternion& in);

    // Writes a container of supported types
    //  uint32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
                && !std::is_arithmetic_v<typename Iterable::value_type>)
    void Write(const Iterable& in) {
        size_t size = in.size();
        Assert31U(size);
        Write(static_cast<int32_t>(size));
        for (auto&& v : in) {
            if constexpr (std::is_same_v<typename Iterable::value_type, std::string>)
                Write(std::string_view(v));
            else
                Write(v);
        }
    }

    // Writes a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    void Write(T in) { WriteSomeBytes(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    // Writes an enum
    //  Bytes written depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    void Write(Enum value) {
        Write(std::to_underlying(value));
    }

    //template<typename T> requires std::is_same_v<T, char16_t>
    //void Write(T in) {
    void Write(char16_t in) {
        if (in < 0x80) {
            Write<BYTE_t>(in);
        }
        else if (in < 0x0800) {
            Write<BYTE_t>(((in >> 6) & 0x1F) | 0xC0);
            Write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
        else { // if (i < 0x010000) {
            Write<BYTE_t>(((in >> 12) & 0x0F) | 0xE0);
            Write<BYTE_t>(((in >> 6) & 0x3F) | 0x80);
            Write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
    }

    void Write(const UserProfile &in) {
        Write(std::string_view(in.m_name));
        Write(std::string_view(in.m_gamerTag));
        Write(std::string_view(in.m_networkUserId));
    }

    void Write(const UInt64Wrapper& in) {
        Write((uint64_t)in);
    }

    void Write(const Int64Wrapper& in) {
        Write((int64_t)in);
    }



    // Empty template
    static void SerializeImpl(DataWriter& pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static decltype(auto) SerializeImpl(DataWriter& pkg, const T &var1, const Types&... var2) {
        pkg.Write(var1);

        return SerializeImpl(pkg, var2...);
    }

    // Serialize variadic types to an array
    template <typename T, typename... Types>
    static decltype(auto) Serialize(const T &var1, const Types&... var2) {
        BYTES_t bytes;
        DataWriter writer(bytes);

        SerializeImpl(writer, var1, var2...);
        return bytes;
    }

    // empty full template
    static decltype(auto) Serialize() {
        return BYTES_t{};
    }



    void SerializeOneLua(IModManager::Type type, sol::object object);

    void SerializeLua(const IModManager::Types& types, const sol::variadic_results& results);

    static decltype(auto) SerializeExtLua(const IModManager::Types& types, const sol::variadic_results& results) {
        BYTES_t bytes;
        DataWriter params(bytes);

        params.SerializeLua(types, results);

        return bytes;
    }
};
