#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"

namespace Valhalla {
    namespace Net {
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

            void Load(byte* buf, int offset);

            void Write(Package& pkg);
            void WriteCompressed(Package& pkg);
            void Write(const byte* buf, int count);
            template<typename T>
            void Write(const T value) requires std::is_trivially_copyable_v<T> {
                m_writer.Write(value);
            }
            void Write(std::string& value);

            void Write(std::vector<byte> &value);



            template<typename T>
            T Read() requires std::is_trivially_copyable_v<T> {
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
            std::string ReadString();
            //Vector3 ReadVector3();
            // ...
            std::unique_ptr<Package> ReadPackage();
            void ReadPackage(Package& pkg);
            std::vector<byte> ReadByteArray();
            void ReadByteArray(std::vector<byte> &vec);

            std::vector<byte> GetArray();

            void Clear();

        };
    }
}
