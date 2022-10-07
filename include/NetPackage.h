#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
#include <locale>
#include <codecvt>
#include <concepts>
#include <type_traits>
#include <robin_hood.h>

#include "Stream.h"
#include "ZDOID.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "PlayerProfile.h"

template<typename E>
concept EnumType = std::is_enum_v<E>;

// how to handle circular dependency with double includes

#define PKG(...) std::make_shared<NetPackage>(##__VA_ARGS__)

class NetPackage {
    void Write7BitEncodedInt(int value);
    int Read7BitEncodedInt();

public:
    using Ptr = std::shared_ptr<NetPackage>;

    Stream m_stream;

    // Used when creating a packet for dispatch
    NetPackage() = default;
    NetPackage(const NetPackage&) = default; // copy
    NetPackage(NetPackage&&) = delete; // move
    // Used in container item reading
    //Package(std::string& base64);



    // Used for reading incoming data from packet
    NetPackage(byte_t* data, uint32_t count);
    NetPackage(std::vector<byte_t>& vec);
    NetPackage(uint32_t reserve);



    void Write(const byte_t* in, uint32_t count);
    template<typename T> void Write(const T &in) requires std::is_fundamental_v<T> { m_stream.Write(reinterpret_cast<const byte_t*>(&in), sizeof(T)); }
    void Write(const std::string& in);
    void Write(const std::vector<byte_t> &in);            // Write array
    void Write(const std::vector<std::string>& in);     // Write string array (NetRpc)
    void Write(const robin_hood::unordered_set<std::string>& in);
    void Write(const NetPackage::Ptr in);
    void Write(const ZDOID &id);
    void Write(const Vector3 &in);
    void Write(const Vector2i &in);
    void Write(const Quaternion& in);
    //void Write(const PlayerProfile& in);

    

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
        auto byteCount = Read7BitEncodedInt();

        if (byteCount < 0)
            throw std::runtime_error("invalid string length");

        if (byteCount == 0)
            return "";

        // this is a bit slower because the resize then copy again
        //std::string out;
        //out.resize(byteCount);
        //
        //m_stream.Read(reinterpret_cast<byte_t*>(out.data()), byteCount);

        std::string out;
        m_stream.Read(out, byteCount);

        return out;
    }

    template<typename T>
    T Read() requires std::same_as<T, std::vector<byte_t>> {
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
    T Read() requires std::same_as<T, NetPackage::Ptr> {
        auto count = Read<int32_t>();
        auto pkg(PKG(count));
        m_stream.Read(pkg->m_stream.Bytes(), count);
        pkg->m_stream.SetLength(count);
        pkg->m_stream.SetMarker(0);
        return pkg;
    }

    template<typename T>
    T Read() requires std::same_as<T, ZDOID> {
        return ZDOID(Read<uuid_t>(), Read<uint32_t>());
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

    //template<typename T>
    //T Read() requires std::same_as<T, Player> {
    //
    //
    //
    //
    //    return PlayerProfile{ Read<float>(), Read<float>(), Read<float>(), Read<float>() };
    //}



    void From(byte_t* buf, int32_t offset);

    void Read(std::vector<byte_t>& out);
    void Read(std::vector<std::string>& out);



    Stream& GetStream() {
        return m_stream;
    }



    // Empty template accepter
    static void Serialize(NetPackage::Ptr pkg) {}

    template <typename T, typename... Types>
    static void Serialize(NetPackage::Ptr pkg, T var1, Types... var2) {
        pkg->Write(var1);

        Serialize(pkg, var2...);
    }



    template<class F>
    static auto Deserialize(NetPackage::Ptr pkg) {
        return std::tuple(pkg->Read<F>());
    }

    template<class F, class S, class...R>
    static auto Deserialize(NetPackage::Ptr pkg) {
        auto a(Deserialize<F>(pkg));
        std::tuple<S, R...> b = Deserialize<S, R...>(pkg);
        return std::tuple_cat(a, b);
    }

};
    
