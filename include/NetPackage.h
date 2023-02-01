#pragma once

#include "HashUtils.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
#include <concepts>
#include <type_traits>
#include <robin_hood.h>
#include <bitset>

#include "Stream.h"
#include "Vector.h"
#include "Quaternion.h"
#include "NetID.h"
#include "VUtilsTraits.h"

// TODO rename BinaryBuffer or DataBuffer or ByteBuffer
//  maybe rename Stream to BinaryStream
class NetPackage {
private:
    void Write7BitEncodedInt(uint32_t value);
    uint32_t Read7BitEncodedInt();

public:
    Stream m_stream;

    NetPackage() = default;
    NetPackage(const BYTE_t* data, uint32_t count); // raw bytes constructor
    explicit NetPackage(BYTES_t &&vec);             // assign vector constructor
    explicit NetPackage(const BYTES_t& vec);        // copy vector constructor
    NetPackage(const BYTES_t& vec, uint32_t count); // vector count constructor
    explicit NetPackage(uint32_t reserve);          // reserve initial memory
    NetPackage(const NetPackage& other) = default;  // copy construct
    NetPackage(NetPackage&& other) = default;       // move construct
    NetPackage& operator=(const NetPackage& other); // copy assign



    // More efficient way to write a sub array/package organized type
    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 0)
    //requires std::is_lvalue_reference_v<std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>> 
        //std::same_as<NetPackage, std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>>
    void NestedWrite(F func) {
        //using args_type = typename VUtils::Traits::func_traits<F>::args_type;

        //static_assert(std::tuple_size<args_type>{} == 2, "Lambda must contain 2 parameters");

        //static_assert(std::same_as<decltype(NetPackage&), std::tuple_element_t<0, args_type>>::value, "First arg must be NetPackage&");

        //static_assert(std::same_as<decltype(NetPackage&), std::tuple_element_t<1, args_type>>::value, "First arg must be NetPackage&");
        
        const auto start = m_stream.Position();
        int32_t count = 0;
        Write(count);

        // call func...
        func();

        const auto end = m_stream.Position();
        m_stream.SetPos(start);
        count = end - start - sizeof(count);
        assert(count >= 0);
        Write(count);
        m_stream.SetPos(end);
    }



    // Writes a BYTE_t* as byte array of length
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTE_t* in, uint32_t count);

    // Writes a BYTES_t* as byte array of length
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTES_t &in, uint32_t count);

    // Writes a BYTE_t* as byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTES_t &in);

    // Writes a NetPackage as byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const NetPackage &in);

    // Writes a string
    void Write(const std::string& in);
    // Writes a ZDOID
    //  12 bytes total are written:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    void Write(const ZDOID &id);

    // Writes a Vector3
    //  12 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    void Write(const Vector3 &in);

    // Writes a Vector2i
    //  8 bytes total are written:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    void Write(const Vector2i &in);

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
        (is_iterable_v<Iterable> && !std::is_same_v<Iterable, std::string> && !std::is_same_v<Iterable, BYTES_t>)
    void Write(const Iterable& in) {
        Write<int32_t>(in.size());
        for (auto&& v : in) {
            Write(v);
        }
    }
    
    // Writes a primitive type
    template<typename T> requires std::is_fundamental_v<T>
    void Write(const T &in) { m_stream.Write(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }
    
    // Writes an enum
    //  Bytes written depend on the underlying value
    template<typename E> requires std::is_enum_v<E>
    void Write(E v) {
        using T = std::underlying_type_t<E>;
        Write(static_cast<T>(v));
    }



    //  Reads a primitive type
    template<typename T> requires std::is_fundamental_v<T>
    T Read() {
        T out;
        m_stream.Read(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    // Reads a string
    template<typename T> requires std::same_as<T, std::string>
    T Read() {
        auto count = Read7BitEncodedInt();
        if (count == 0)
            return "";

        if (m_stream.Position() + count > m_stream.Length()) 
            throw std::range_error("NetPackage::Read<std::string>() length exceeded");
        
        std::string s;

        s.insert(s.end(),
            m_stream.m_buf.begin() + m_stream.m_pos,
            m_stream.m_buf.begin() + m_stream.m_pos + count);

        m_stream.m_pos += count;

        return s;
    }

    // Reads a byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    template<typename T> requires std::same_as<T, BYTES_t>
    T Read() {
        T out;
        m_stream.Read(out, Read<uint32_t>());
        return out;
    }

    // Reads a container of supported types
    //  uint32_t:   size
    //  T...:       value_type
    template<typename Iterable> requires (is_iterable_v<Iterable> && !std::is_same_v<Iterable, std::string> && !std::is_same_v<Iterable, BYTES_t>)
    Iterable Read() {
        Iterable out;
        auto count = Read<uint32_t>();
    
        while (count--) {
            out.insert(Read<Iterable::value_type>());
        }
        return out;
    }

    // Reads a NetPackage
    //  uint32_t:   size
    //  BYTES_t:    data
    template<typename T> requires std::same_as<T, NetPackage>
    T Read() {
        auto count = Read<uint32_t>();

        NetPackage pkg(count);
        m_stream.Read(pkg.m_stream.m_buf, count);
        assert(pkg.m_stream.Length() == count);
        assert(pkg.m_stream.Position() == 0);
        return pkg;
    }

    // Reads a ZDOID
    //  12 bytes total are read:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    template<typename T> requires std::same_as<T, ZDOID>
    T Read() {
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
    T Read() {
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
    T Read() {
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
    T Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        auto d(Read<float>());
        return Quaternion{ a, b, c, d };
    }

    // Reads an enum type
    //  Bytes read depend on the underlying value
    template<typename E> requires std::is_enum_v<E>
    E Read() {
        return static_cast<E>(Read<std::underlying_type_t<E>>());
    }

    // Reads a byte array 
    // The target vector be overwritten
    void Read(BYTES_t& out);

    // Reads a NetPackage from package
    // The target package will be entirely overwritten
    void Read(NetPackage &out);



public:
    // Empty template
    static void _Serialize(NetPackage &pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static void _Serialize(NetPackage& pkg, T var1, const Types&... var2) {
        pkg.Write(var1);

        _Serialize(pkg, var2...);
    }

    // Serialize variadic types to a NetPackage
    template <typename T, typename... Types>
    static NetPackage Serialize(T var1, const Types&... var2) {
        NetPackage pkg((sizeof(var1) + ... + sizeof(Types)));

        _Serialize(pkg, var1, var2...);
        return pkg;
    }

    // Deserialize a NetPackage to a tuple of types
    template<class...Ts, class NP>
    static std::tuple<Ts...> Deserialize(NP& pkg) {
        return {pkg.template Read<Ts>()...};
    }
};
