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

// TODO rename BinaryBuffer or DataBuffer or ByteBuffer
//  maybe rename Stream to BinaryStream
class NetPackage {
private:
    void Write7BitEncodedInt(int32_t value);
    int Read7BitEncodedInt();

public:
    Stream m_stream;

    NetPackage() = default;
    NetPackage(const BYTE_t* data, uint32_t count); // raw bytes constructor
    explicit NetPackage(BYTES_t &&vec);                      // assign vector constructor
    explicit NetPackage(const BYTES_t& vec);                 // copy vector constructor
    NetPackage(const BYTES_t& vec, uint32_t count); // vector count constructor
    explicit NetPackage(uint32_t reserve);                   // reserve initial memory
    NetPackage(const NetPackage& other) = default;  // copy
    NetPackage(NetPackage&& other) = default;       // move



    NetPackage& operator=(const NetPackage& other);



    // Write length-pointer data as byte array
    void Write(const BYTE_t* in, uint32_t count);
    // Write length-vector as byte array
    void Write(const BYTES_t &in, uint32_t count);            // Write array
    // Write vector as byte array
    void Write(const BYTES_t &in);
    // Write Package as byte array
    void Write(const NetPackage &in);
    // Write string
    void Write(const std::string& in);
    // Write NetID
    void Write(const NetID &id);
    // Write Vector3
    void Write(const Vector3 &in);
    // Write Vector2i
    void Write(const Vector2i &in);
    // Write Quaternion
    void Write(const Quaternion& in);
    
    //template<class Container>
    //typename std::enable_if<has_const_iterator<Container>::value,
    //    void>::type

    //template<class Container> requires is_container<Container>::value
    
    //template<class Container> requires is_container_of<Container, std::string>::value
    //void Write(const Container& in) {
    //    Write(static_cast<uint32_t>(in.size()));
    //    for (auto&& s : in) {
    //        //static_assert(std::is_same_v<decltype(s), std::string>);
    //        Write(s);
    //    }
    //}


    //template<template<class> class T>
    //void Write(const T<std::string> &in) {
    //    Write(static_cast<uint32_t>(in.size()));
    //    for (auto&& s : in) {
    //        Write(s);
    //    }
    //}
    
    // Write string vector as string array
    void Write(const std::vector<std::string>& in);     // Write string array (NetRpc)
    // Write string set as string array
    void Write(const robin_hood::unordered_set<std::string>& in);
    // Write primitive
    template<typename T> void Write(const T &in) requires std::is_fundamental_v<T> { m_stream.Write(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    

    // Enum overload; writes the enum underlying value
    // https://stackoverflow.com/questions/60524480/how-to-force-a-template-parameter-to-be-an-enum-or-enum-class
    template<typename E>
    void Write(E v) requires std::is_enum_v<E> {
        using T = std::underlying_type_t<E>;
        Write(static_cast<T>(v));
    }



    template<typename T>
    T Read() requires std::is_fundamental_v<T> {
        T out;
        m_stream.Read(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    template<typename T>
    T Read() requires std::same_as<T, std::string> {
        auto count = Read7BitEncodedInt();

        if (count < 0)
            throw std::runtime_error("negative count");

        if (count == 0)
            return "";
        
        std::string s;
        //m_stream.Read(s, byteCount);



        if (m_stream.Position() + count > m_stream.Length()) throw std::range_error("NetPackage::Read<std::string>() length exceeded");
        s.insert(s.end(),
            m_stream.m_buf.begin() + m_stream.m_pos,
            m_stream.m_buf.begin() + m_stream.m_pos + count);
        m_stream.m_pos += count;



        return s;
    }

    template<typename T>
    T Read() requires std::same_as<T, BYTES_t> {
        T out;
        m_stream.Read(out, Read<uint32_t>());
        return out;
    }

    //template<class Container>
    //typename std::enable_if<has_const_iterator<Container>::value,
    //    Container>::type
    //template<class Container> requires is_container<Container>::value
    //Container Read() {
    //    Container out;
    //    auto count = Read<uint32_t>();
    //
    //    while (count--) {
    //        out.push_back(Read<std::string>());
    //    }
    //    return out;
    //}

    //template<template<class> class T>
    //T<std::string> Read() requires is_container<T<std::string>>::value {
    //    T out;
    //    auto count = Read<uint32_t>();
    //
    //    while (count--) {
    //        out.push_back(Read<std::string>());
    //    }
    //    return out;
    //}

    template<typename T>
    T Read() requires std::same_as<T, NetPackage> {
        auto count = Read<uint32_t>();

        NetPackage pkg(count);
        m_stream.Read(pkg.m_stream.m_buf, count);
        assert(pkg.m_stream.Length() == count);
        assert(pkg.m_stream.Position() == 0);
        return pkg;
    }

    template<typename T>
    T Read() requires std::same_as<T, NetID> {
        auto a(Read<OWNER_t>());
        auto b(Read<uint32_t>());
        return NetID(a, b);
    }

    template<typename T>
    T Read() requires std::same_as<T, Vector3> {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        return Vector3{ a, b, c };
    }

    template<typename T>
    T Read() requires std::same_as<T, Vector2i> {
        auto a(Read<int32_t>());
        auto b(Read<int32_t>());
        return Vector2i{ a, b };
    }

    template<typename T>
    T Read() requires std::same_as<T, Quaternion> {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        auto d(Read<float>());
        return Quaternion{ a, b, c, d };
    }

    template<typename E>
    E Read() requires std::is_enum_v<E> {
        return static_cast<E>(Read<std::underlying_type_t<E>>());
    }



    //template<typename T>
    //T Read() requires std::same_as<T, Player> {
    //
    //
    //
    //
    //    return PlayerProfile{ Read<float>(), Read<float>(), Read<float>(), Read<float>() };
    //}



    // Reads a byte array from package
    // The target vector be overwritten
    void Read(BYTES_t& out);

    // Reads a string array from package
    // The target vector will be overwritten
    ///template<class Container> requires is_container<Container>::value
    //typename std::enable_if<has_const_iterator<Container>::value,
        //void>::type
    ///void Read(Container& out) {
    ///    auto count = Read<uint32_t>();
    ///
    ///    out.clear();
    ///    out.reserve(count);
    ///
    ///    while (count--) {
    ///        out.push_back(Read<std::string>());
    ///    }
    ///}

    template<typename T>
    T Read() requires std::same_as<T, std::vector<std::string>> {
        T out;
        auto count = Read<uint32_t>();

        while (count--) {
            out.push_back(Read<std::string>());
        }
        return out;
    }

    //template<template<class> class T>
    //void Read(T<std::string>& out) requires is_container<T<std::string>>::value{
    //    auto count = Read<uint32_t>();
    //
    //    out.clear();
    //    out.reserve(count);
    //
    //    while (count--) {
    //        out.push_back(Read<std::string>());
    //    }
    //}

    // Reads a NetPackage from package
    // The target package will be entirely overwritten
    void Read(NetPackage &out);



    // Empty template
    static void _Serialize(NetPackage &pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static void _Serialize(NetPackage& pkg, T var1, const Types&... var2) {
        pkg.Write(var1);

        _Serialize(pkg, var2...);
    }

    template <typename T, typename... Types>
    static NetPackage Serialize(T var1, const Types&... var2) {
        NetPackage pkg((sizeof(var1) + ... + sizeof(Types)));

        _Serialize(pkg, var1, var2...);
        return pkg;
    }


    // https://stackoverflow.com/questions/21180346/variadic-template-unpacking-arguments-to-typename
    // empty

    // this is better
    template<class...Ts, class NP>
    static std::tuple<Ts...> Deserialize(NP& pkg)
    {
        return {pkg.template Read<Ts>()...};
    }

    //template<class Tuple, size_t ...Is> // class Tuple>
    //static Tuple Deserialize(NetPackage& pkg, std::index_sequence<Is...>)
    //{
    //    //std::tuple<int, char> tup;
    //    //std::get<0>(tup)
    //    //Tuple tuple;
    //    //return {pkg.template Read<std::get<Is>(tuple)>()...};
    //    //std::tuple_element_t<0, decltype(tup)> e;
    //
    //    return { pkg.template Read<std::tuple_element_t<Is, Tuple>>()...};
    //}

};
