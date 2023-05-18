#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "DataStream.h"

class DataReader : public DataStream {
private:
    bool ReadSomeBytes(BYTE_t* buffer, size_t count) {
        std::visit(VUtils::Traits::overload{
            [&](std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) { 
                auto&& buf = pair.first.get();
                auto&& pos = pair.second;

                // A vector is dynamic
                if (pos + count > buf.size())
                    throw std::runtime_error("no more to read");
                
                std::copy(buf.data() + pos,
                    buf.data() + pos + count,
                    buffer);

                pos += count;
            },
            [&](std::pair<BYTE_VIEW_t, size_t>& pair) { 
                auto&& buf = pair.first;
                auto&& pos = pair.second;

                // An array is fixed (error if exceeded)
                if (pos + count > buf.size())
                    throw std::runtime_error("read exceeds array bounds");

                std::copy(buf.data() + pos,
                    buf.data() + pos + count,
                    buffer);

                pos += count;
            },
            [&](std::pair<std::FILE*, Type>& pair) {
                // just write file
                if (pair.second == Type::READ) {
                    if (std::fread(buffer, count, 1, pair.first) != 1) {
                        throw std::runtime_error("error while reading from file");
                    }
                } else {
                    throw std::runtime_error("expected file reader got writer");
                }
            }
        }, this->m_data);
    }

    uint32_t Read7BitEncodedInt() {
        uint32_t out = 0;
        uint32_t num2 = 0;
        while (num2 != 35) {
            auto b = Read<uint8_t>();
            out |= static_cast<decltype(out)>(b & 127) << num2;
            num2 += 7;
            if ((b & 128) == 0)
            {
                return out;
            }
        }
        throw std::runtime_error("bad encoded int");
    }

public:
    explicit DataReader(BYTE_VIEW_t buf) : DataStream(buf) {}
    explicit DataReader(BYTES_t &buf) : DataStream(buf) {}
    explicit DataReader(FILE* file, Type type) : DataStream(file, type) {}

public:
    template<typename T>
        requires 
            (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>
                || std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
    decltype(auto) Read() {
        auto count = (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>) 
            ? Read<uint32_t>()
            : Read7BitEncodedInt();

        //this->AssertOffset(count);

        // use visit
        return std::visit(VUtils::Traits::overload{
            [&](std::pair<std::reference_wrapper<BYTES_t>, size_t>& pair) {
                auto&& buf = pair.first.get();
                auto&& pos = pair.second;

                // throw if end reached
                if (pos + count > buf.size())
                    throw std::runtime_error("read exceeds vector bounds");

                auto result = T(buf.data() + pos,
                    buf.data() + pos + count);

                pos += count;

                return result;
            },
            [&](std::pair<BYTE_VIEW_t, size_t>& pair) {
                auto&& buf = pair.first;
                auto&& pos = pair.second;

                // throw if end reached
                if (pos + count > buf.size())
                    throw std::runtime_error("read exceeds array bounds");

                auto result = T(buf.data() + pos,
                    buf.data() + pos + count);

                pos += count;

                return result;
            },
            [&](std::pair<std::FILE*, Type>& pair) {
                if constexpr (std::is_same_v<T, BYTE_VIEW_t> || std::is_same_v<T, std::string_view>) {
                    throw std::runtime_error("tried reading observer buffer from file");
                }
                else {
                    T result{};
                    result.resize(count);
                    // just write file
                    if (pair.second == Type::READ) {
                        if (std::fread(result.data(), count, 1, pair.first) != 1) {
                            throw std::runtime_error("error while reading string from file");
                        }
                        return result;
                    }
                    else {
                        throw std::runtime_error("expected file reader got writer");
                    }
                }
            }
        }, this->m_data);
    }

    
    // Reads a byte array as a Reader (more efficient than ReadBytes())
    //  int32_t:   size
    //  BYTES_t:    data
    template<typename T>
        requires std::is_same_v<T, DataReader>
    decltype(auto) Read() {
        return T(Read<BYTE_VIEW_t>());
    }



    //  Reads a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    decltype(auto) Read() {
        T out{};
        ReadSomeBytes(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    // Reads a container of supported types
    //  int32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
            && !std::is_arithmetic_v<typename Iterable::value_type>)
    decltype(auto) Read() {
        using Type = Iterable::value_type;

        const auto count = Read<int32_t>();

        Iterable out;

        if constexpr (std::is_same_v<Type, std::string>)
            out.reserve(count);

        for (int32_t i=0; i < count; i++) {
            auto type = Read<Type>();
            out.insert(out.end(), type);
        }

        return out;
    }

    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 1)
    void AsEach(F func) {
        using Type = std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>;

        const auto count = Read<int32_t>();

        for (int32_t i = 0; i < count; i++) {
            func(Read<Type>());
        }
    }

    // Reads a ZDOID
    //  12 bytes total are read:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    template<typename T> requires std::same_as<T, ZDOID>
    decltype(auto) Read() {
        auto a(Read<int64_t>());
        auto b(Read<uint32_t>());
        return ZDOID(a, b);
    }

    // Reads a Vector3f
    //  12 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    template<typename T> requires std::same_as<T, Vector3f>
    decltype(auto) Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        return Vector3f{ a, b, c };
    }

    // Reads a Vector2i
    //  8 bytes total are read:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    template<typename T> requires std::same_as<T, Vector2i>
    decltype(auto) Read() {
        auto a(Read<int32_t>());
        auto b(Read<int32_t>());
        return Vector2i(a, b);
    }

    // Reads a Quaternion
    //  16 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    template<typename T> requires std::same_as<T, Quaternion>
    decltype(auto) Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        auto d(Read<float>());
        return Quaternion{ a, b, c, d };
    }

    // Reads an enum type
    //  - Bytes read depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    decltype(auto) Read() {
        return static_cast<Enum>(Read<std::underlying_type_t<Enum>>());
    }

    // Read a UTF-8 encoded C# char
    //  Will advance 1 -> 3 bytes, depending on size of first char
    template<typename T> requires std::is_same_v<T, char16_t>
    decltype(auto) Read() {
        auto b1 = Read<uint8_t>();

        // 3 byte
        if (b1 >= 0xE0) {
            auto b2 = Read<uint8_t>() & 0x3F;
            auto b3 = Read<uint8_t>() & 0x3F;
            return ((b1 & 0xF) << 12) | (b2 << 6) | b3;
        }
        // 2 byte
        else if (b1 >= 0xC0) {
            auto b2 = Read<uint8_t>() & 0x3F;
            return ((b1 & 0x1F) << 6) | b2;
        }
        // 1 byte
        else {
            return b1 & 0x7F;
        }
    }

    template<typename T> requires std::is_same_v<T, UserProfile>
    decltype(auto) Read() {
        auto name = Read<std::string>();
        auto gamerTag = Read<std::string>();
        auto networkUserId = Read<std::string>();

        return UserProfile(std::move(name), std::move(gamerTag), std::move(networkUserId));
    }



    // verbose extension methods
    //  I want these to actually all be in lua
    //  templates in c, wrappers for lua in modman

    decltype(auto) ReadBool() { return Read<bool>(); }

    decltype(auto) ReadString() { return Read<std::string>(); }
    decltype(auto) ReadStrings() { return Read<std::vector<std::string>>(); }

    decltype(auto) ReadBytes() { return Read<BYTES_t>(); }

    decltype(auto) ReadZDOID() { return Read<ZDOID>(); }
    decltype(auto) ReadVector3f() { return Read<Vector3f>(); }
    decltype(auto) ReadVector2i() { return Read<Vector2i>(); }
    decltype(auto) ReadQuaternion() { return Read<Quaternion>(); }
    decltype(auto) ReadProfile() { return Read<UserProfile>(); }

    decltype(auto) ReadInt8() { return Read<int8_t>(); }
    decltype(auto) ReadInt16() { return Read<int16_t>(); }
    decltype(auto) ReadInt32() { return Read<int32_t>(); }
    decltype(auto) ReadInt64() { return Read<int64_t>(); }

    decltype(auto) ReadUInt8() { return Read<uint8_t>(); }
    decltype(auto) ReadUInt16() { return Read<uint16_t>(); }
    decltype(auto) ReadUInt32() { return Read<uint32_t>(); }
    decltype(auto) ReadUInt64() { return Read<uint64_t>(); }

    decltype(auto) ReadFloat() { return Read<float>(); }
    decltype(auto) ReadDouble() { return Read<double>(); }
    
    decltype(auto) ReadChar() { return Read<char16_t>(); }



    // Deserialize a reader to a tuple of types
    template<class...Ts, class RD>
    static std::tuple<Ts...> Deserialize(RD& reader) {
        return { reader.template Read<Ts>()... };
    }
};
