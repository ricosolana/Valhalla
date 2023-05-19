#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "DataStream.h"
#include "DataReader.h"

class DataWriter : public virtual DataStream {
private:
    // Write count bytes from the specified buffer
    // Bytes are written in place, making space as necessary
    void WriteSomeBytes(const BYTE_t* buffer, size_t count) {
        std::visit(VUtils::Traits::overload{
            [&](std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) { 
                auto&& buf = pair.first.get();
                auto&& pos = pair.second;
                
                // It would be better if there was a vector<>::insert
                //  that overwrites elements instead of needing to
                //  allocate then copy in place

                // A vector is dynamic
                if (pos + count > buf.size())
                    buf.resize(pos + count);

                std::copy(buffer,
                    buffer + count, 
                    buf.data() + pos);

                pos += count;
            },
            [&](std::pair<BYTE_VIEW_t, size_t>& pair) { 
                auto&& buf = pair.first;
                auto&& pos = pair.second;

                // An array is fixed (error if exceeded)
                if (pos + count > buf.size())
                    throw std::runtime_error("write exceeds array bounds");

                std::copy(buffer,
                    buffer + count, 
                    buf.data() + pos);

                pos += count;
            },
            [&](std::FILE* file) {
                // just write file
                if (std::fwrite(buffer, count, 1, file) != 1) {
                    throw std::runtime_error("error while writing to file");
                }
            }
        }, this->m_data);
    }

    // Write count bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteSomeBytes(const BYTES_t& vec, size_t count) {
        return WriteSomeBytes(vec.data(), count);
    }

    // Write all bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteSomeBytes(const BYTES_t& vec) {
        return WriteSomeBytes(vec, vec.size());
    }

    void Write7BitEncodedInt(int32_t value) {
        auto num = static_cast<uint32_t>(value);
        for (; num >= 128U; num >>= 7)
            Write((BYTE_t)(num | 128U));

        Write((BYTE_t)num);
    }

public:
    explicit DataWriter(BYTE_VIEW_t buf) : DataStream(buf) {}
    explicit DataWriter(BYTES_t& buf) : DataStream(buf) {}
    //explicit DataWriter(std::string_view path) : DataStream(ScopedFile(path.data(), "wb")) {}
    explicit DataWriter(std::FILE* file) : DataStream(file) {}

    /*
    // Clears the underlying container and resets position
    // TODO standardize by renaming to 'clear'
    void Clear() {
        if (this->owned()) {
            this->m_pos = 0;
            this->m_ownedBuf.clear();
        }
        else {
            throw std::runtime_error("tried calling Clear() on unownedBuf DataWriter");
        }
    }*/

    // Sets the length of this stream
    //void SetLength(uint32_t length) {
    //    m_provider.get().resize(length);
    //}

public:
    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 1)
    void SubWrite(F func) {
        const auto start = this->Position();
        int32_t count = 0;
        Write(count);

        // call func...
        func(std::ref(*this));

        const auto end = this->Position();
        this->SetPos(start);
        count = end - start - sizeof(count);
        assert(count >= 0);
        Write(count);
        this->SetPos(end);
    }

    void Write(const BYTE_t* in, size_t length) {
        Write<int32_t>(length);
        WriteSomeBytes(in, length);
    }

    template<typename T>
        requires (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>)
    void Write(const T& in) {
        Write(in.data(), in.size());
        //Write<int32_t>(in.size());
        //WriteSomeBytes(in.data(), in.size());
    } // std::is_same_v<T, DataReader>

    void Write(const DataReader& in) {

        std::visit(VUtils::Traits::overload{
            [this](const std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) {
                Write(pair.first.get());
            },
            [this](const std::pair<BYTE_VIEW_t, size_t>& pair) {
                Write(pair.first);
            },
            [&](std::FILE* src) {
                // Extend underlying structure accordingly if vector
                
                /*std::visit([](auto&& pair1) {
                    using T = std::decay_t<decltype(pair1)>;

                    if constexpr (!std::is_same_v<T, std::pair<std::FILE*, Type>>) {

                        // Write the outer file to this vector
                        // extend the vector
                        BYTE_t* buf;

                        constexpr bool is_bytes = std::is_same_v<T, std::pair<std::reference_wrapper<BYTES_t>, size_t>>;
                        if constexpr (is_bytes) {
                            buf = pair1.first.get();
                        }

                        auto&& buf = pair.first.get();
                        auto&& pos = pair.second;

                        uint32_t count = 0;
                        if (std::fread(&count, sizeof(count), 1, src) != 1)
                            throw std::runtime_error("failed to read count from file");

                        if (pos + count > buf.size())
                            buf.resize(pos + count);

                        if (std::fread(buf.data(), count, 1, src) != 1)
                            throw std::runtime_error("failed to read payload data from file");

                        pos += count;

                        if constexpr (!std::is_same_v<T, std::pair<std::reference_wrapper<BYTES_t>, size_t>>) {

                        }
                    }
                    else {

                    }                    
                }, this->m_data);*/



                std::visit(VUtils::Traits::overload{
                    [&](std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) {
                        // Write the outer file to this vector
                        // extend the vector
                        auto&& buf = pair.first.get();
                        auto&& pos = pair.second;

                        uint32_t count = 0;
                        if (std::fread(&count, sizeof(count), 1, src) != 1)
                            throw std::runtime_error("failed to read count from file");

                        if (pos + count > buf.size())
                            buf.resize(pos + count);

                        if (std::fread(buf.data() + pos, count, 1, src) != 1)
                            throw std::runtime_error("failed to read payload data from file");

                        pos += count;
                    },
                    [&](std::pair<BYTE_VIEW_t, size_t>& pair) {
                        auto&& buf = pair.first;
                        auto&& pos = pair.second;

                        uint32_t count = 0;
                        if (std::fread(&count, sizeof(count), 1, src) != 1)
                            throw std::runtime_error("failed to read count from file");

                        if (pos + count > buf.size())
                            throw std::runtime_error("byte_view will be exceeded");

                        if (std::fread(buf.data() + pos, count, 1, src) != 1)
                            throw std::runtime_error("failed to read payload data from file");

                        pos += count;
                    },
                    [&](std::FILE* dst) {
                        // read from in file then write to this->file (dst)

                        uint32_t count = 0;
                        if (std::fread(&count, sizeof(count), 1, src) != 1)
                            throw std::runtime_error("failed to read count tf file");

                        int ch = 0;
                        while ((ch = std::getc(src)) != EOF) {
                            if (std::putc(ch, dst) == EOF && std::ferror(dst) == 0) {
                                throw std::runtime_error("error while writing to tf file");
                            }
                        }

                        if (std::ferror(src) == 0) {
                            throw std::runtime_error("error while reading from tf file");
                        }
                    }
                }, this->m_data);

                // Read from file

                // then read the 
            }
        }, in.m_data);

        //Write(in.data(), in.size());
        //Write<int32_t>(in.size());
        //WriteSomeBytes(in.data(), in.size());
    }

    // Writes a string
    void Write(std::string_view in) {
        auto length = in.length();

        auto byteCount = static_cast<int32_t>(length);

        Write7BitEncodedInt(byteCount);
        if (byteCount == 0)
            return;

        WriteSomeBytes(reinterpret_cast<const BYTE_t*>(in.data()), byteCount);
    }

    // Writes a ZDOID
    //  12 bytes total are written:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    void Write(ZDOID in) {
        Write(in.GetOwner());
        Write(in.GetUID());
    }

    // Writes a Vector3f
    //  12 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    void Write(Vector3f in) {
        Write(in.x);
        Write(in.y);
        Write(in.z);
    }

    // Writes a Vector2i
    //  8 bytes total are written:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    void Write(Vector2i in) {
        Write(in.x);
        Write(in.y);
    }

    // Writes a Quaternion
    //  16 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    void Write(Quaternion in) {
        Write(in.x);
        Write(in.y);
        Write(in.z);
        Write(in.w);
    }

    // Writes a container of supported types
    //  uint32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
                && !std::is_arithmetic_v<typename Iterable::value_type>)
    void Write(const Iterable& in) {
        size_t size = in.size();
        Write(static_cast<int32_t>(size));
        for (auto&& v : in) {
            Write(v);
        }
    }

    // Writes a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    void Write(T in) { WriteSomeBytes(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    // Writes an enum
    //  Bytes written depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    void Write(Enum value) {
        Write(std::to_underlying(value));
    }

    //template<typename T> requires std::is_same_v<T, char16_t>
    //void Write(T in) {
    void Write(char16_t in) {
        if (in < 0x80) {
            Write<BYTE_t>(in);
        }
        else if (in < 0x0800) {
            Write<BYTE_t>(((in >> 6) & 0x1F) | 0xC0);
            Write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
        else { // if (i < 0x010000) {
            Write<BYTE_t>(((in >> 12) & 0x0F) | 0xE0);
            Write<BYTE_t>(((in >> 6) & 0x3F) | 0x80);
            Write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
    }

    void Write(const UserProfile &in) {
        Write(std::string_view(in.m_name));
        Write(std::string_view(in.m_gamerTag));
        Write(std::string_view(in.m_networkUserId));
    }



    // Empty template
    static void SerializeImpl(DataWriter& pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static decltype(auto) SerializeImpl(DataWriter& pkg, const T &var1, const Types&... var2) {
        pkg.Write(var1);

        return SerializeImpl(pkg, var2...);
    }

    // Serialize variadic types to an array
    template <typename T, typename... Types>
    static decltype(auto) Serialize(const T &var1, const Types&... var2) {
        BYTES_t bytes;
        DataWriter writer(bytes);

        SerializeImpl(writer, var1, var2...);
        return bytes;
    }

    // empty full template
    static decltype(auto) Serialize() {
        return BYTES_t{};
    }
};
