#include "NetPackage.hpp"
#include <assert.h>

namespace Valhalla {
    namespace Net {

        Package::Package() 
            : m_writer(m_stream), m_reader(m_stream) {

        }

        Package::Package(byte* data, int count)
            : Package() {
            m_stream.Write(data, 0, count);
            m_stream.m_pos = 0;
        }

        Package::Package(std::vector<byte>& vec)
            : Package(vec.data(), vec.size()) {}



        void Package::Load(byte* data, int count) {
            Clear();
            m_stream.Write(data, 0, count);
            m_stream.m_pos = 0;
        }



        void Package::Write(Package& pkg) {
            Write(pkg.m_stream.Buffer());
        }

        void Package::WriteCompressed(Package& pkg) {
            throw std::runtime_error("not implemented");
        }

		void Package::Write(const byte* buf, int count) {
            m_writer.Write(count);
			m_writer.Write(buf, count);
		}

		void Package::Write(std::string& value) {
			m_writer.Write(value);
		}

        void Package::Write(std::vector<byte> &value) {
            Write(value.data(), static_cast<int>(value.size()));
        }



        std::string Package::ReadString() {
            return m_reader.ReadString();
        }

        std::unique_ptr<Package> Package::ReadPackage() {

            int count = Read<int>();
            std::vector<byte> data(count);
            m_reader.Read(data.data(), count);
            return std::make_unique<Package>(data);
        }

        void Package::ReadPackage(Package& pkg) {

        }

        std::vector<byte> Package::ReadByteArray() {
            std::vector<byte> ret(Read<int>());

            m_reader.Read(ret.data(), ret.size());

            return ret;
        }

        void Package::ReadByteArray(std::vector<byte>& vec) {
            vec.reserve(Read<int>());
            m_reader.Read(vec.data(), vec.size());
            m_reader.Read()

            return ret;
        }



    }
}
