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
    //using Ptr = std::shared_ptr<NetPackage>;

    Stream m_stream;

    // Used when creating a packet for dispatch
    NetPackage() = default;
    NetPackage(const NetPackage& other); // copy
    NetPackage(NetPackage&& other) = default; // move
    // Used in container item reading
    //Package(std::string& base64);



    void operator=(const NetPackage& other);



    // Used for reading incoming data from packet
    NetPackage(const byte_t* data, uint32_t count);
    NetPackage(const BYTES_t& vec);
    NetPackage(uint32_t reserve);



    void Write(const byte_t* in, uint32_t count);
    template<typename T> void Write(const T &in) requires std::is_fundamental_v<T> { m_stream.Write(reinterpret_cast<const byte_t*>(&in), sizeof(T)); }
    void Write(const std::string& in);
    void Write(const BYTES_t &in);            // Write array
    void Write(const std::vector<std::string>& in);     // Write string array (NetRpc)
    void Write(const robin_hood::unordered_set<std::string>& in);
    void Write(const NetPackage &in);
    void Write(const NetID &id);
    void Write(const Vector3 &in);
    void Write(const Vector2i &in);
    void Write(const Quaternion& in);
    //void Write(const PlayerProfile& in);

    

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
        m_stream.Read(reinterpret_cast<byte_t*>(&out), sizeof(T));
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
        return NetID(Read<UUID_t>(), Read<uint32_t>());
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



    void From(const byte_t* buf, int32_t offset);

    void Read(std::vector<byte_t>& out);
    void Read(std::vector<std::string>& out);



    // Empty template recursor
    static void Serialize(NetPackage &pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static void Serialize(NetPackage& pkg, T var1, Types... var2) {
        pkg.Write(var1);

        Serialize(pkg, var2...);
    }

    template <typename T, typename... Types>
    static NetPackage Serialize(T var1, Types... var2) {
        NetPackage pkg;
        Serialize(pkg, var1, var2...);
        return pkg;
    }



    // Final variadic parameter
    template<class F>
    static auto Deserialize(NetPackage &pkg) {
        return std::tuple(pkg.Read<F>());
    }

    // Reads parameters from a package
    template<class F, class S, class...R>
    static auto Deserialize(NetPackage &pkg) {
        auto a(Deserialize<F>(pkg));
        std::tuple<S, R...> b = Deserialize<S, R...>(pkg);
        return std::tuple_cat(a, b);
    }
};
