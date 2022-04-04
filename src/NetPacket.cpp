#include "NetPacket.hpp"
#include <assert.h>

namespace Alchyme {
    namespace Net {
        bool Packet::Read(std::string& out) {
            std::uint_fast8_t size; if (!Read(size)) return false;

            if (size != 0) {

                if (size >= UINT8_MAX)
                    throw std::runtime_error("String too long");

                out.resize(size);
                return Read(reinterpret_cast<std::byte*>(out.data()), size);
            }

            return false;
        }

        bool Packet::Read(std::byte* out, std::size_t size) {
            if (offset + size > m_buf.size())
                return false;
            std::memcpy(out, m_buf.data() + offset, size);
            offset += size;
            return true;
        }



        void Packet::Write(const std::byte* in, std::size_t size) {
            m_buf.insert(m_buf.end(), in, in + size);

            offset += size;
        }

        void Packet::Write(const std::string_view str) {
            //assert(str.length() < UINT8_MAX);
            //if (str.length() >= UINT8_MAX)
                //return false;
                //throw std::runtime_error("String too long");

            // Write length
            //Write(static_cast<std::uint_fast8_t>(str.length()));
            Write(str.length());
            if (str.length() != 0)
                Write(reinterpret_cast<const std::byte*>(&str[0]), str.length());
            //return true;
        }
    }
}
