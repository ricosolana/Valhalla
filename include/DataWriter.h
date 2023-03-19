#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "DataStream.h"

class DataWriter : public DataStream {
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
    DataWriter(BYTES_t& bytes) : DataStream(bytes) {}

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
    void Write(const std::string& in);
    // Writes a ZDOID
    //  12 bytes total are written:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    void Write(const ZDOID& id);

    // Writes a Vector3
    //  12 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    void Write(const Vector3& in);

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
    template<typename Iterable> requires
        (VUtils::Traits::is_iterable_v<Iterable> && !std::is_same_v<Iterable, std::string> && !std::is_same_v<Iterable, BYTES_t>)
        void Write(const Iterable& in) {
        size_t size = in.size();
        Assert31U(size);
        Write(static_cast<int32_t>(size));
        for (auto&& v : in) {
            Write(v);
        }
    }

    // Writes a primitive type
    template<typename T> requires (std::is_fundamental_v<T> && !std::is_same_v<T, char16_t>)
    void Write(T in) { WriteSomeBytes(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    // Writes an enum
    //  Bytes written depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    void Write(Enum value) {
        Write(std::to_underlying(value));
    }

    template<typename T> requires std::is_same_v<T, char16_t>
    void Write(T in) {
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
        Write(in.m_name);
        Write(in.m_gamerTag);
        Write(in.m_networkUserId);
    }



    // Empty template
    static void _Serialize(DataWriter& pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static void _Serialize(DataWriter& pkg, const T &var1, const Types&... var2) {
        pkg.Write(var1);

        _Serialize(pkg, var2...);
    }

    // Serialize variadic types to an array
    template <typename T, typename... Types>
    static BYTES_t Serialize(const T &var1, const Types&... var2) {
        BYTES_t bytes; bytes.reserve((sizeof(var1) + ... + sizeof(Types)));
        DataWriter writer(bytes);

        _Serialize(writer, var1, var2...);
        return bytes;
    }

    // empty full template
    static BYTES_t Serialize() {
        BYTES_t bytes;
        return bytes;
    }

};
