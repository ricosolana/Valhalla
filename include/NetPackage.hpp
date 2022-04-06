#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"

class Package {
    BinaryWriter m_writer;
    BinaryReader m_reader;
    Stream m_stream;

public:
    // Used when creating a packet for dispatch
    Package();
    // Used in inventory item reading
    //Package(std::string& base64);
            
    // Used for reading incoming data from packet
    Package(byte* data, int count);
    Package(std::vector<byte>& vec);
    Package(int reserve);

    void Load(byte* buf, int offset);

    void Write(Package &in);
    void WriteCompressed(Package& in);
    void Write(const byte* in, int count);
    template<typename T>
    void Write(const T &in) requires std::is_fundamental_v<T> {//std::is_trivially_copyable_v<T> {
        m_writer.Write(in);
    }
    void Write(const std::string& in);

    void Write(const std::vector<byte> &in);

    void Write(const std::vector<std::string>& in);



    template<typename T>
    T Read() {
        return m_reader.Read<T>();
    }

    // Zeta delta output maybe
    //ZDOID ReadZDOID();
    //bool ReadBool();
    //char ReadChar(); // weird impl in c#, since char is 2 bytes in c#
    //byte ReadByte();
    //signed char ReadSByte();
    //short ReadShort();
    //int ReadInt();
    //unsigned int ReadUInt();
    //long long ReadLong();
    //unsigned long long ReadULong();
    //float ReadSingle();
    //double ReadDouble();
    //std::string ReadString();
    //Vector3 ReadVector3();
    // ...
    //std::unique_ptr<Package> ReadPackage();
    void ReadPackage(Package& out);
    template<typename T>
    T Read() requires std::same_as<T, Package> {
        std::vector<byte> vec;
        ReadByteArray(vec);
        return Package(vec);
    }
    //std::vector<byte> ReadByteArray();
    void ReadByteArray(std::vector<byte> &out);
    void Read(std::vector<std::string>& out);



    void GetArray(std::vector<byte> &vec);

    std::vector<byte>& Buffer();
    int Size();
    void Clear();
    void SetPos(int pos);

};
    
