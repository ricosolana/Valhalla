#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

namespace Alchyme {
    namespace Net {
        struct Packet {
            uint16_t offset = 0;
            std::vector<std::byte> m_buf;

            //uint16_t GetSize();

            /**
             * Readers
            */
            bool Read(std::byte* out, std::size_t size);

            bool Read(std::string& out);

            template<typename T>
            bool Read(std::vector<T> out) requires std::is_trivially_copyable_v<T> {
                std::size_t length;
                Read(length);
                if (length != 0) {
                    auto data = reinterpret_cast<T*>(m_buf.data());
                    out.insert(out.end(), data, data + length);
                }
            }

            template<typename T>
            bool Read(T& out) requires std::is_trivially_copyable_v<T> {
                return Read(reinterpret_cast<std::byte*>(&out), sizeof(out));
            }

            /*
            * Writers
            */
            void Write(const std::byte*, std::size_t size);

            void Write(const std::string_view str);

            template<typename T>
            void Write(const std::vector<T> in) requires std::is_trivially_copyable_v<T> {
                Write(in.size());
                if (in.size() != 0)
                    Write(reinterpret_cast<const std::byte*>(in.data()), in.size());
            }

            template<typename T>
            void Write(const T in) requires std::is_trivially_copyable_v<T> {
                Write(reinterpret_cast<const std::byte*>(&in), sizeof(in));
            }
        };
    }
}
