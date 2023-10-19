#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "ModManager.h"
#include "DataStream.h"
#include "DataReader.h"

class DataWriter : public virtual DataStream {
private:
    // Write count bytes from the specified buffer
    // Bytes are written in place, making space as necessary
    void write_some_bytes(const BYTE_t* buffer, size_t count) {
        std::visit(VUtils::Traits::overload{
            [this, count](std::reference_wrapper<BYTES_t> buf) { 
                if (this->check_offset(count))
                    buf.get().resize(this->m_pos + count);
            },
            [this, count](BYTE_VIEW_t buf) { this->try_offset(count); }
        }, this->m_data);

        std::copy(buffer,
                  buffer + count, 
                  this->data() + this->m_pos);

        this->m_pos += count;
    }

    // Write count bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto write_some_bytes(const BYTES_t& vec, size_t count) {
        return write_some_bytes(vec.data(), count);
    }

    // Write all bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto write_some_bytes(const BYTES_t& vec) {
        return write_some_bytes(vec, vec.size());
    }

    void write_encoded_int(int32_t value) {
        auto num = static_cast<uint32_t>(value);
        for (; num >= 128U; num >>= 7)
            write((BYTE_t)(num | 128U));

        write((BYTE_t)num);
    }

public:
    explicit DataWriter(BYTE_VIEW_t buf) : DataStream(buf) {}
    explicit DataWriter(BYTES_t &buf) : DataStream(buf) {}
    explicit DataWriter(BYTE_VIEW_t buf, size_t pos) : DataStream(buf, pos) {}
    explicit DataWriter(BYTES_t &buf, size_t pos) : DataStream(buf, pos) {}

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
    void nested_write(F func) {
        const auto start = this->get_pos();
        int32_t count = 0;
        write(count);

        // call func...
        func(std::ref(*this));

        const auto end = this->get_pos();
        this->set_pos(start);
        count = end - start - sizeof(count);
        assert(count >= 0);
        write(count);
        this->set_pos(end);
    }

    void write(const BYTE_t* in, size_t length) {
        write<int32_t>(length);
        write_some_bytes(in, length);
    }

    template<typename T>
        requires (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t> || std::is_same_v<T, DataReader>)
    void write(const T& in) {
        write(in.data(), in.size());
        //Write<int32_t>(in.size());
        //write_some_bytes(in.data(), in.size());
    }

    // Writes a string
    void write(std::string_view in) {
        auto length = in.length();

        auto byteCount = static_cast<int32_t>(length);

        write_encoded_int(byteCount);
        if (byteCount == 0)
            return;

        write_some_bytes(reinterpret_cast<const BYTE_t*>(in.data()), byteCount);
    }

    // Writes a ZDOID
    //  12 bytes total are written:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    void write(ZDOID in) {
        write((int64_t)in.GetOwner());
        write(in.GetUID());
    }

    // Writes a Vector3f
    //  12 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    void write(Vector3f in) {
        write(in.x);
        write(in.y);
        write(in.z);
    }

    // Writes a Vector2i
    //  8 bytes total are written:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    void write(Vector2i in) {
        write(in.x);
        write(in.y);
    }

    // Writes a Vector2s
    //  4 bytes total are written:
    //  int16_t: x (2 bytes)
    //  int16_t: y (2 bytes)
    void write(Vector2s in) {
        write(in.x);
        write(in.y);
    }

    // Writes a Quaternion
    //  16 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    void write(Quaternion in) {
        write(in.x);
        write(in.y);
        write(in.z);
        write(in.w);
    }

    // Writes a container of supported types
    //  uint32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
                && !std::is_fundamental_v<typename Iterable::value_type>)
    void write(const Iterable& in) {
        size_t size = in.size();
        write(static_cast<int32_t>(size));
        for (auto&& v : in) {
            write(v);
        }
    }

    // Writes a primitive type
    template<typename T> 
        requires (std::is_fundamental_v<T> && !std::is_same_v<T, char16_t>)
    void write(T in) { write_some_bytes(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    // Writes an enum
    //  Bytes written depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    void write(Enum value) {
        write(std::to_underlying(value));
    }

    //template<typename T> requires std::is_same_v<T, char16_t>
    //void write(T in) {
    void write(char16_t in) {
        if (in < 0x80) {
            write<BYTE_t>(in);
        }
        else if (in < 0x0800) {
            write<BYTE_t>(((in >> 6) & 0x1F) | 0xC0);
            write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
        else { // if (i < 0x010000) {
            write<BYTE_t>(((in >> 12) & 0x0F) | 0xE0);
            write<BYTE_t>(((in >> 6) & 0x3F) | 0x80);
            write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
    }

    void write(const UserProfile &in) {
        write(std::string_view(in.m_name));
        write(std::string_view(in.m_gamerTag));
        write(std::string_view(in.m_networkUserId));
    }

    void write(UInt64Wrapper in) {
        write((uint64_t)in);
    }

    void write(Int64Wrapper in) {
        write((int64_t)in);
    }



    // Empty template
    static void do_serialize(DataWriter& pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static decltype(auto) do_serialize(DataWriter& pkg, const T &var1, const Types&... var2) {
        pkg.write(var1);

        return do_serialize(pkg, var2...);
    }

    // Serialize variadic types to an array
    template <typename T, typename... Types>
    static decltype(auto) serialize(const T &var1, const Types&... var2) {
        BYTES_t bytes;
        DataWriter writer(bytes);

        do_serialize(writer, var1, var2...);
        return bytes;
    }

    // empty full template
    static decltype(auto) serialize() {
        return BYTES_t{};
    }


#if VH_IS_ON(VH_USE_MODS)
    void single_serialize_lua(IModManager::Type type, sol::object arg) {
        switch (type) {
            // TODO add recent unsigned types
        case IModManager::Type::UINT8:
            write(arg.as<uint8_t>());
            break;
        case IModManager::Type::UINT16:
            write(arg.as<uint16_t>());
            break;
        case IModManager::Type::UINT32:
            write(arg.as<uint32_t>());
            break;
        case IModManager::Type::UINT64:
            write(arg.as<uint64_t>());
            break;
        case IModManager::Type::INT8:
            write(arg.as<int8_t>());
            break;
        case IModManager::Type::INT16:
            write(arg.as<int16_t>());
            break;
        case IModManager::Type::INT32:
            write(arg.as<int32_t>());
            break;
        case IModManager::Type::INT64:
            write(arg.as<int64_t>());
            break;
        case IModManager::Type::FLOAT:
            write(arg.as<float>());
            break;
        case IModManager::Type::DOUBLE:
            write(arg.as<double>());
            break;
        case IModManager::Type::STRING:
            write(arg.as<std::string>());
            break;
        case IModManager::Type::BOOL:
            write(arg.as<bool>());
            break;
        case IModManager::Type::BYTES:
            write(arg.as<BYTES_t>());
            break;
        case IModManager::Type::ZDOID:
            write(arg.as<ZDOID>());
            break;
        case IModManager::Type::VECTOR3f:
            write(arg.as<Vector3f>());
            break;
        case IModManager::Type::VECTOR2i:
            write(arg.as<Vector2i>());
            break;
        case IModManager::Type::QUATERNION:
            write(arg.as<Quaternion>());
            break;
        default:
            throw std::runtime_error("Invalid data type");
        }
    }

    void serialize_lua(const IModManager::Types& types, const sol::variadic_results& results) {
        for (int i = 0; i < results.size(); i++) {
            single_serialize_lua(types[i], results[i]);
        }
    }

    static decltype(auto) serialize_lua_bytes(const IModManager::Types& types, const sol::variadic_results& results) {
        BYTES_t bytes;
        DataWriter params(bytes);

        params.serialize_lua(types, results);

        return bytes;
    }
#endif
};
