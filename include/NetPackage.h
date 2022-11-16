#pragma once

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

template<typename E>
concept EnumType = std::is_enum_v<E>;

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
    //Package(std::string& base64);



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
    // Write string vector as string array
    void Write(const std::vector<std::string>& in);     // Write string array (NetRpc)
    // Write string set as string array
    void Write(const robin_hood::unordered_set<std::string>& in);
    // Write primitive
    template<typename T> void Write(const T &in) requires std::is_fundamental_v<T> { m_stream.Write(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    

    // Enum overload; writes the enum underlying value
    // https://stackoverflow.com/questions/60524480/how-to-force-a-template-parameter-to-be-an-enum-or-enum-class
    template<EnumType E>
    void Write(E v) {
        //std::to_underlying
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
            throw std::runtime_error("invalid string length");

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
        m_stream.Read(out, Read<int32_t>());
        return out;
    }

    template<typename T>
    T Read() requires std::same_as<T, std::vector<std::string>> {
        T out;
        auto count = Read<int32_t>();
        while (count--) {
            out.push_back(Read<std::string>());
        }
        return out;
    }

    template<typename T>
    T Read() requires std::same_as<T, NetPackage> {
        auto count = Read<int32_t>();
        NetPackage pkg(count);
        m_stream.Read(pkg.m_stream.m_buf, count);
        assert(pkg.m_stream.Length() == count);
        assert(pkg.m_stream.Position() == 0);
        return pkg;
    }

    template<typename T>
    T Read() requires std::same_as<T, NetID> {
        return NetID(Read<OWNER_t>(), Read<uint32_t>());
    }

    template<typename T>
    T Read() requires std::same_as<T, Vector3> {
        return Vector3{ Read<float>(), Read<float>(), Read<float>() };
    }

    template<typename T>
    T Read() requires std::same_as<T, Vector2i> {
        return Vector2i{ Read<int32_t>(), Read<int32_t>()};
    }

    template<typename T>
    T Read() requires std::same_as<T, Quaternion> {
        return Quaternion{ Read<float>(), Read<float>(), Read<float>(), Read<float>() };
    }

    template<EnumType E>
    E Read() {
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
    void Read(std::vector<std::string>& out);

    // Reads a NetPackage from package
    // The target package will be overwritten
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
        NetPackage pkg;
        _Serialize(pkg, var1, var2...);
        return pkg;
    }



    /*
    // Final variadic parameter
    template<class F>
    static auto Deserialize(NetPackage &pkg) {
        return std::tuple(pkg.Read<F>());
    }

    // Reads parameters from a package
    // The marker will be modified
    template<class F, class S, class...R>
    static auto Deserialize(NetPackage &pkg) {
        auto a(Deserialize<F>(pkg));
        std::tuple<S, R...> b = Deserialize<S, R...>(pkg);
        return std::tuple_cat(a, b);
    }*/



    // TODO this is new
    // blank
    //template<class... T>
    //static std::tuple<T...> Deserialize(NetPackage& pkg);

    // https://stackoverflow.com/questions/21180346/variadic-template-unpacking-arguments-to-typename
    // empty
    /*
    static auto Deserialize(NetPackage&) {
        //return std::make_tuple(); //std::tuple{};
        return std::tuple{};
    }

    template<class ... Types> std::enable_if_t<sizeof...(Types) == 0>
    static Deserialize(NetPackage &pkg) {}

    // must unpack
    // how to unpack types in place without a, b unpacking
    //template<class T, class... Types>
    template<class T, class... Types>
    static auto Deserialize(NetPackage& pkg) {
        auto t = std::tuple(pkg.Read<T>());
        if constexpr (sizeof...(Types)) {
            return std::tuple_cat(t, Deserialize<Types...>(pkg));
        }
        return t;
    }*/



    template <class... T>
    struct DeserializeImpl;

    template<>
    struct DeserializeImpl<> {
        auto operator()(NetPackage& pkg) const {
            return std::tuple{};
        }
    };

    template<class T, class ... Types>
    struct DeserializeImpl<T, Types...> {
        auto operator()(NetPackage& pkg) const {
            return std::tuple_cat(
                    std::make_tuple(pkg.Read<T>()), DeserializeImpl<Types...>()(pkg));
        }
    };

    template<class... Types>
    static auto Deserialize(NetPackage& pkg) {
        return DeserializeImpl<Types...>()(pkg);
    }
};
