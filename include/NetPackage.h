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

#define PKG(...) std::make_shared<ZPackage>(##__VA_ARGS__)

class ZPackage {
    //BinaryWriter m_writer;
    //BinaryReader m_reader;
    Stream m_stream;

    void Write7BitEncodedInt(int value);
    int Read7BitEncodedInt();

public:
    using Ptr = std::shared_ptr<ZPackage>;

    

    // Used when creating a packet for dispatch
    ZPackage() = default;
    ZPackage(const ZPackage&) = default; // copy
    ZPackage(ZPackage&&) = delete; // move
    // Used in inventory item reading
    //Package(std::string& base64);



    // Used for reading incoming data from packet
    ZPackage(byte_t* data, int32_t count);
    ZPackage(std::vector<byte_t>& vec);
    ZPackage(int32_t reserve);



    void Write(const byte_t* in, int32_t count);          // Write array
    template<typename T> void Write(const T &in) requires std::is_fundamental_v<T> { m_stream.Write(reinterpret_cast<const byte_t*>(&in), sizeof(T)); }
    void Write(const std::string& in);
    void Write(const std::vector<byte_t> &in);            // Write array
    void Write(const std::vector<std::string>& in);     // Write string array (ZRpc)
    void Write(const robin_hood::unordered_set<std::string>& in);
    void Write(const ZPackage::Ptr in);
    void Write(const ZDOID &id);
    void Write(const Vector3 &in);
    void Write(const Vector2i &in);
    void Write(const Quaternion& in);
    //void Write(const PlayerProfile& in);

    

    // https://stackoverflow.com/questions/60524480/how-to-force-a-template-parameter-to-be-an-enum-or-enum-class
    template<EnumType E>
    void Write(E v) {
        using T = std::underlying_type_t<E>;
        Write((T)v);
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

        std::string out;
        out.resize(byteCount);

        m_stream.Read(reinterpret_cast<byte_t*>(out.data()), byteCount);

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
            out.emplace_back(Read<std::string>());
        }
        return out;
    }

    template<typename T>
    T Read() requires std::same_as<T, ZPackage::Ptr> {
        auto count = Read<int32_t>();
        auto pkg(PKG(count));
        m_stream.Read(pkg->m_stream.Bytes(), count);
        pkg->GetStream().SetLength(count);
        pkg->GetStream().ResetPos();
        return pkg;
    }

    template<typename T>
    T Read() requires std::same_as<T, ZDOID> {
        return ZDOID(Read<int64_t>(), Read<uint32_t>());
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



    void Load(byte_t* buf, int32_t offset);

    ZPackage::Ptr ReadCompressed();
    void WriteCompressed(ZPackage::Ptr);

    void Read(std::vector<byte_t>& out);
    void Read(std::vector<std::string>& out);



    // Empty template accepter
    static void Serialize(ZPackage::Ptr pkg) {}

    
    // this wont compile when trying to use enums
    // see:
    // https://cplusplus.com/reference/type_traits/underlying_type/
    // this might allow compile when casting the enum to its underlying type
    template <typename T, typename... Types>
    static void Serialize(ZPackage::Ptr pkg, T var1, Types... var2) {
        pkg->Write(var1);

        Serialize(pkg, var2...);
    }



    template<class F>
    static auto Deserialize(ZPackage::Ptr pkg) {
        return std::tuple(pkg->Read<F>());
    }

    template<class F, class S, class...R>
    static auto Deserialize(ZPackage::Ptr pkg) {
        auto a(Deserialize<F>(pkg));
        std::tuple<S, R...> b = Deserialize<S, R...>(pkg);
        return std::tuple_cat(a, b);
    }



    Stream& GetStream() {
        return m_stream;
    }

};
    
