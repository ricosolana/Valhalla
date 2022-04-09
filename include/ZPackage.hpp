#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"
#include "ZDOID.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Quaternion.hpp"

class ZPackage {
    BinaryWriter m_writer;
    BinaryReader m_reader;
    Stream m_stream;

public:
    // Used when creating a packet for dispatch
    ZPackage();
    // Used in inventory item reading
    //Package(std::string& base64);


    // Used for reading incoming data from packet
    ZPackage(byte* data, int32_t count);
    ZPackage(std::vector<byte>& vec);
    ZPackage(int32_t reserve);


    //static std::unique_ptr<ZPackage> New();
    //static void ReturnPkg(ZPackage* pkg);

    void Load(byte* buf, int32_t offset);



    void Write(const ZPackage& in);
    void WriteCompressed(const ZPackage& in);
    void Write(const byte* in, int32_t count);
    template<typename T>
    void Write(const T &in) requires std::is_fundamental_v<T> {
        m_writer.Write(in);
    }
    void Write(const std::string& in);

    void Write(const std::vector<byte> &in);

    void Write(const std::vector<std::string>& in);

    void Write(const ZDOID &id);
    void Write(const Vector3 &v3);
    void Write(const Vector2i &v2);
    void Write(const Quaternion &q);



    template<typename T>
    T Read() {
        return m_reader.Read<T>();
    }

    template<typename T>
    T Read() requires std::same_as<T, ZPackage> {
        int32_t count = Read<int32_t>();
        ZPackage pkg(count);
        m_stream.Read(pkg.Bytes(), count);
        return pkg; // Might have to use std::move
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
        return Vector2i{ Read<float>(), Read<float>(), Read<float>(), Read<float>() };
    }

    void ReadByteArray(std::vector<byte> &out);

    void Read(std::vector<std::string>& out);



    //void GetArray(std::vector<byte> &vec);

    byte* Bytes() const;
    int32_t Size() const;
    void Clear();
    //void SetPos(int32_t pos);

};
    
