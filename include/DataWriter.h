#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "ModManager.h"
#include "DataStream.h"

//class DataReader;

/*
template <class T>
struct Serializer;

template <class T>
struct Serializer
    : _Conditionally_enabled_hash<_Kty,
    !is_const_v<_Kty> && !is_volatile_v<_Kty> && (is_enum_v<_Kty> || is_integral_v<_Kty> || is_pointer_v<_Kty>)> {
    // hash functor primary template (handles enums, integrals, and pointers)
    static size_t _Do_hash(const _Kty& _Keyval) noexcept {
        return _Hash_representation(_Keyval);
    }
};*/

template<typename T>
class IDataWriter : public IDataStream<T> {
private:
    // Write count bytes from the specified buffer
    // Bytes are written in place, making space as necessary
    void WriteSomeBytes(const BYTE_t* buffer, size_t count) {
        //this->Assert31U(count);
        //this->Assert31U(this->m_pos + count);

        // this copies in place, without relocating bytes exceeding m_pos
        // resize, ensuring capacity for copy operation

        //vector::assign only works starting at beginning... so cant use it...
        if constexpr (std::is_same_v<T, BYTE_VIEW_t>) {
            this->AssertOffset(count);
        }
        else {
            if (this->CheckOffset(count))
                this->m_buf.get().resize(this->m_pos + count);
        }

        std::copy(buffer,
                  buffer + count, 
                  this->data() + this->m_pos);

        this->m_pos += count;
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
    IDataWriter(T buf) : IDataStream<T>(buf) {}

    IDataWriter(T buf, size_t pos) : IDataStream<T>(buf, pos) {}

    // Clears the underlying container and resets position
    void Clear() {
        if constexpr (std::is_same_v<T, std::reference_wrapper<BYTES_t>>) {
            this->m_pos = 0;
            this->m_buf.get().clear();
        }
    }

    // Sets the length of this stream
    //void SetLength(uint32_t length) {
    //    m_provider.get().resize(length);
    //}

public:
    /*
    DataReader ToReader() {
        if constexpr (std::is_same_v<T, BYTE_VIEW_t>)
            return DataReader(this->m_buf, this->m_pos);
        else
            return DataReader(this->m_buf.get(), this->m_pos);
    }*/

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

    template<typename T>
        requires (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>)
    void Write(const T& in) {
        Write<int32_t>(in.size());
        WriteSomeBytes(in.data(), in.size());
    }

    /*
    // Writes a BYTE_t* as byte array of length
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTE_t* in, size_t count) {
        Write<int32_t>(count);
        WriteSomeBytes(in, count);
    }

    // Writes a BYTES_t* as byte array of length
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTES_t& in, size_t count) {
        Write(in.data(), count);
    }

    // Writes a BYTE_t* as byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    void Write(const BYTES_t& in) {
        Write(in.data(), in.size());
    }*/

    // Writes a NetPackage as byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    //void Write(const NetPackage& in);

    // Writes a string
    void Write(std::string_view in) {
        auto length = in.length();
        IDataStream<T>::Assert31U(length);

        auto byteCount = static_cast<int32_t>(length);

        Write7BitEncodedInt(byteCount);
        if (byteCount == 0)
            return;

        WriteSomeBytes(reinterpret_cast<const BYTE_t*>(in.data()), byteCount);
    }

private:
    void svUnpack() {}

    template<typename A, typename ...B>
    void svUnpack(const A& a, const B&... b) {
        auto&& sv = std::string_view(a);
        WriteSomeBytes(reinterpret_cast<const BYTE_t*>(sv.data()), sv.length());

        svUnpack(b...);
    }

public:
    // Writes a tuple of strings as a singular concatenated string
    template<typename ...Strings>
    void Write(const std::tuple<Strings...> &in) {
        //static_assert(
        //        std::is_same_v<std::tuple_element_t<0, decltype(in)>, std::string_view>
        //    ||  std::is_same_v<std::tuple_element_t<0, decltype(in)>, std::string_view>
        //);
        // Unpack the tuple and sum length using a fold
        auto length = std::apply([](const auto&... args) {
            return (std::string_view(args).length() + ...);
        }, in);

        IDataStream<T>::Assert31U(length);

        auto byteCount = static_cast<int32_t>(length);

        Write7BitEncodedInt(byteCount);
        if (byteCount == 0)
            return;

        /*
        // Unpack the tuple and write all bytes using a fold
        std::apply([this](const auto&... args) {
            auto&& view = std::string_view(args);
            //(WriteSomeBytes(reinterpret_cast<const BYTE_t*>(view.data()), view.length()), ...);
            svUnpack(args...);

            
            if constexpr (std::is_same_v<decltype(args), std::string_view>)
                (WriteSomeBytes(reinterpret_cast<const BYTE_t*>(args.data()), args.length()), ...);
            else {
                auto&& view(std::string_view(args));
                (WriteSomeBytes(reinterpret_cast<const BYTE_t*>(view.data()), view.length()), ...);
            }*
        }, in);*/
        //std::apply(std::bind_front(&IDataWriter<T>::svUnpack<Strings, Strings...>, this), in);
        std::apply([this](const auto& ... args) {
            svUnpack(args...);
        }, in);
    }

    // Writes a ZDOID
    //  12 bytes total are written:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    void Write(const ZDOID& in) {
        Write(in.GetOwner());
        Write(in.GetUID());
    }

    // Writes a Vector3f
    //  12 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    void Write(const Vector3f& in) {
        Write(in.x);
        Write(in.y);
        Write(in.z);
    }

    // Writes a Vector2i
    //  8 bytes total are written:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    void Write(const Vector2i& in) {
        Write(in.x);
        Write(in.y);
    }

    // Writes a Quaternion
    //  16 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    void Write(const Quaternion& in) {
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
        //this->Assert31U(size);
        Write(static_cast<int32_t>(size));
        for (auto&& v : in) {
            if constexpr (std::is_same_v<typename Iterable::value_type, std::string>)
                Write(std::string_view(v));
            else
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

    void Write(const UInt64Wrapper& in) {
        Write((uint64_t)in);
    }

    void Write(const Int64Wrapper& in) {
        Write((int64_t)in);
    }



    // Empty template
    static void SerializeImpl(IDataWriter<std::reference_wrapper<BYTES_t>>& pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static decltype(auto) SerializeImpl(IDataWriter<std::reference_wrapper<BYTES_t>>& pkg, const T &var1, const Types&... var2) {
        pkg.Write(var1);

        return SerializeImpl(pkg, var2...);
    }

    // Serialize variadic types to an array
    template <typename T, typename... Types>
    static decltype(auto) Serialize(const T &var1, const Types&... var2) {
        BYTES_t bytes;
        IDataWriter<std::reference_wrapper<BYTES_t>> writer(bytes);

        SerializeImpl(writer, var1, var2...);
        return bytes;
    }

    // empty full template
    static decltype(auto) Serialize() {
        return BYTES_t{};
    }



    void SerializeOneLua(IModManager::Type type, sol::object arg) {
        switch (type) {
            // TODO add recent unsigned types
        case IModManager::Type::UINT8:
            Write(arg.as<uint8_t>());
            break;
        case IModManager::Type::UINT16:
            Write(arg.as<uint16_t>());
            break;
        case IModManager::Type::UINT32:
            Write(arg.as<uint32_t>());
            break;
        case IModManager::Type::UINT64:
            Write(arg.as<uint64_t>());
            break;
        case IModManager::Type::INT8:
            Write(arg.as<int8_t>());
            break;
        case IModManager::Type::INT16:
            Write(arg.as<int16_t>());
            break;
        case IModManager::Type::INT32:
            Write(arg.as<int32_t>());
            break;
        case IModManager::Type::INT64:
            Write(arg.as<int64_t>());
            break;
        case IModManager::Type::FLOAT:
            Write(arg.as<float>());
            break;
        case IModManager::Type::DOUBLE:
            Write(arg.as<double>());
            break;
        case IModManager::Type::STRING:
            Write(arg.as<std::string_view>());
            break;
        case IModManager::Type::BOOL:
            Write(arg.as<bool>());
            break;
        case IModManager::Type::BYTES:
            Write(arg.as<BYTES_t>());
            break;
        case IModManager::Type::ZDOID:
            Write(arg.as<ZDOID>());
            break;
        case IModManager::Type::VECTOR3f:
            Write(arg.as<Vector3f>());
            break;
        case IModManager::Type::VECTOR2i:
            Write(arg.as<Vector2i>());
            break;
        case IModManager::Type::QUATERNION:
            Write(arg.as<Quaternion>());
            break;
        default:
            throw std::runtime_error("Invalid data type");
        }
    }

    void SerializeLua(const IModManager::Types& types, const sol::variadic_results& results) {
        for (int i = 0; i < results.size(); i++) {
            SerializeOneLua(types[i], results[i]);
        }
    }

    static decltype(auto) SerializeExtLua(const IModManager::Types& types, const sol::variadic_results& results) {
        BYTES_t bytes;
        IDataWriter<std::reference_wrapper<BYTES_t>> params(bytes);

        params.SerializeLua(types, results);

        return bytes;
    }
};

using DataWriter = IDataWriter<std::reference_wrapper<BYTES_t>>;
using DataWriterStatic = IDataWriter<BYTE_VIEW_t>;
