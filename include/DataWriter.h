#pragma once

#include "VUtils.h"
#include "NetID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "VUtilsTraits.h"

class DataWriter {
public:
    std::reference_wrapper<BYTES_t> m_provider;
    int32_t m_pos;

private:
    // Write count bytes from the specified buffer
    // Bytes are written in place, making space as necessary
    void WriteBytes(const BYTE_t* buffer, int32_t count);

    // Write a single byte from the specified buffer
    // Bytes are written in place, making space as necessary
    //auto Write(const BYTE_t value) {
    //    return Write(&value, sizeof(value));
    //}

    // Write count bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteBytes(const BYTES_t& vec, int32_t count) {
        return WriteBytes(vec.data(), count);
    }

    // Write all bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteBytes(const BYTES_t& vec) {
        return WriteBytes(vec, static_cast<int32_t>(vec.size()));
    }

    void Write7BitEncodedInt(int32_t value) {
        auto num = static_cast<uint32_t>(value);
        for (; num >= 128U; num >>= 7)
            Write((BYTE_t)(num | 128U));

        Write((BYTE_t)num);
    }

public:
    DataWriter(BYTES_t& bytes) : m_provider(bytes), m_pos(0) {}

    // Gets all the data in the package
    //BYTES_t Bytes() const {
    //    return *m_provider;
    //}

    // Gets all the data in the package
    //void Bytes(BYTES_t &out) const {
    //    out = *m_buf;
    //}

    //BYTES_t Remaining() {
    //    BYTES_t res;
    //    Read(res, m_provider->size() - m_pos);
    //    return res;
    //}



    // Reset the marker and length members
    // No array shrinking is performed
    void Clear() {
        m_pos = 0;
        m_provider.get().clear();
    }



    // Returns the length of this stream
    int32_t Length() const {
        return static_cast<int32_t>(m_provider.get().size());
    }

    // Sets the length of this stream
    void SetLength(uint32_t length) {
        m_provider.get().resize(length);
    }



    // Returns the position of this stream
    int32_t Position() const {
        return m_pos;
    }

    // Sets the positino of this stream
    void SetPos(int32_t pos);

public:
    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 0)
    //requires std::is_lvalue_reference_v<std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>> 
        //std::same_as<NetPackage, std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>>
    void SubWrite(F func) {
        //using args_type = typename VUtils::Traits::func_traits<F>::args_type;

        //static_assert(std::tuple_size<args_type>{} == 2, "Lambda must contain 2 parameters");

        //static_assert(std::same_as<decltype(NetPackage&), std::tuple_element_t<0, args_type>>::value, "First arg must be NetPackage&");

        //static_assert(std::same_as<decltype(NetPackage&), std::tuple_element_t<1, args_type>>::value, "First arg must be NetPackage&");

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
    void Write(const BYTE_t* in, int32_t count);

    // Writes a BYTES_t* as byte array of length
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTES_t& in, int32_t count);

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
        Write<int32_t>(in.size());
        for (auto&& v : in) {
            Write(v);
        }
    }

    // Writes a primitive type
    template<typename T> requires std::is_fundamental_v<T>
    void Write(const T& in) { WriteBytes(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    // Writes an enum
    //  Bytes written depend on the underlying value
    template<typename E> requires std::is_enum_v<E>
    void Write(E v) {
        using T = std::underlying_type_t<E>;
        Write(static_cast<T>(v));
    }

    void WriteChar(uint16_t i);



    // Empty template
    static void _Serialize(DataWriter& pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static void _Serialize(DataWriter& pkg, T var1, const Types&... var2) {
        pkg.Write(var1);

        _Serialize(pkg, var2...);
    }

    // Serialize variadic types to a NetPackage
    template <typename T, typename... Types>
    static BYTES_t Serialize(T var1, const Types&... var2) {
        BYTES_t bytes; bytes.reserve((sizeof(var1) + ... + sizeof(Types)));
        DataWriter writer(bytes);

        _Serialize(writer, var1, var2...);
        return bytes;
    }
};
