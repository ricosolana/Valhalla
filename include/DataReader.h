#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "ModManager.h"
#include "DataStream.h"

class DataReader : public DataStream {
private:
    void PeekSomeBytes(BYTE_SPAN_t view, size_t &pos) const {
        AssertOffset(view.size(), pos);
        std::memmove(view.data(), this->data() + pos, view.size());
        pos += view.size();
    }

    void ReadSomeBytes(BYTE_SPAN_t view) {
        auto pos = Position();
        this->PeekSomeBytes(view, pos);
        this->SetPos(pos);
    }
    


    // TODO
    uint32_t Peek7BitEncodedInt(size_t& pos) const {
        size_t dummy = pos;

        uint32_t out = 0;
        uint32_t num2 = 0;
        while (num2 != 35) {
            auto b = Peek<uint8_t>(dummy);
            out |= static_cast<decltype(out)>(b & 127) << num2;
            num2 += 7;
            if ((b & 128) == 0)
            {
                pos = dummy;
                return out;
            }
        }
        throw std::runtime_error("bad encoded int");
    }

public:
    explicit DataReader(BYTE_SPAN_t buf) : DataStream(buf) {}
    explicit DataReader(BYTES_t &buf) : DataStream(buf) {}
    explicit DataReader(BYTE_SPAN_t buf, size_t pos) : DataStream(buf, pos) {}
    explicit DataReader(BYTES_t &buf, size_t pos) : DataStream(buf, pos) {}

public:
    // Peek a primitive type from the stream
    //  T:        value type
    template<typename T>
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        BYTE_t buf[sizeof(T)]{};
        PeekSomeBytes(BYTE_SPAN_t(buf, sizeof(T)), dummy);
        
        if constexpr (std::is_floating_point_v<T>) {
            auto classify = std::fpclassify(*reinterpret_cast<T*>(buf));
            if (!(classify == FP_NORMAL || classify == FP_ZERO)) {
                throw std::runtime_error("bad floating point number");
            }
        }
        else if constexpr (std::is_same_v<T, bool>) {
            if (buf[0] > 0b1) {
                throw std::runtime_error("bad bool");
            }
        }

        // To stop msvc warnings
        T out = *reinterpret_cast<T*>(buf);

        pos = dummy;
        return out;
    }
    
    // Peek several consecutive types from the stream
    //  T...:       value types
    template<typename T>
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    void PeekSome(std::span<T> view, size_t& pos) const {
        size_t dummy = pos;
        for (int i = 0; i < view.size(); i++) {
            view[i] = Peek<T>(dummy);
        }
        pos = dummy;
    }

    // Peek a byte array from the stream
    //  int32_t:        size
    //  bytes...:       data
    template<typename T>
        requires 
            (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>
                || std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        auto count = (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>) 
            ? Peek<uint32_t>(dummy)
            : Peek7BitEncodedInt(dummy);
        
        this->AssertOffset(count);
        
        auto result = T(reinterpret_cast<const T::value_type*>(this->data()) + dummy,
            reinterpret_cast<const T::value_type*>(this->data()) + dummy + count);

        pos = dummy + count;

        return result;
    }

    template<typename T>
        requires std::is_same_v<T, BYTE_SPAN_t>
    decltype(auto) Peek(size_t& pos) {
        size_t dummy = pos;

        auto count = Peek<uint32_t>(dummy);
        
        this->AssertOffset(count);
        
        auto result = T(this->data() + dummy, this->data() + dummy + count);

        pos = dummy + count;

        return result;
    }
    
    // Reads a byte array as a Reader (more efficient than ReadBytes())
    //  int32_t:        size
    //  bytes...:       data
    template<typename T>
        requires std::is_same_v<T, DataReader>
    decltype(auto) Peek(size_t &pos) {
        return T(Peek<BYTE_SPAN_t>(pos));
    }

    // Reads a container of supported types
    //  int32_t:        count
    //  T1, T2, T3...:  value types
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
            && !std::is_arithmetic_v<typename Iterable::value_type>)
    decltype(auto) Peek(size_t& pos) const {
        using Type = Iterable::value_type;

        size_t dummy = pos;
        const auto count = Peek<int32_t>(dummy);

        Iterable out{};
        out.reserve(count);

        for (int32_t i=0; i < count; i++) {
            auto type = Peek<Type>(dummy);
            out.insert(out.end(), type);
        }

        pos = dummy;

        return out;
    }

    // Peek a container and pass each element to a consumer function
    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 1)
    void PeekEach(F func, size_t& pos) const {
        using T = std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>;

        size_t dummy = pos;
        const auto count = Peek<int32_t>(dummy);

        for (int32_t i = 0; i < count; i++) {
            func(Peek<T>(dummy));
        }

        pos = dummy;
    }

    // Read a container and pass each element to a consumer function
    template<typename F>
    void ReadEach(F func) {
        auto pos = Position();
        PeekEach(func, pos);
        SetPos(pos);
    }

    // Reads a ZDOID
    //  12 bytes total are read:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    template<typename T> requires std::same_as<T, ZDOID>
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        auto a(Peek<int64_t>(dummy));
        auto b(Peek<uint32_t>(dummy));
        auto out = ZDOID(a, b);
        
        pos = dummy;
        return out;
    }

    static inline float x1v{ 99999999.f }, y1v{ 99999999.f }, z1v{ 99999999.f };
    static inline float x2v{ -99999999.f }, y2v{ -99999999.f }, z2v{ -99999999.f };

    // Reads a Vector3f
    //  12 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    template<typename T> requires std::same_as<T, Vector3f>
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        auto a(Peek<float>(dummy));
        auto b(Peek<float>(dummy));
        auto c(Peek<float>(dummy));

        x1v = std::min(a, x1v); y1v = std::min(b, y1v); z1v = std::min(c, z1v);
        x2v = std::max(a, x2v); y2v = std::max(b, y2v); z2v = std::max(c, z2v);

        pos = dummy;

        return Vector3f(a, b, c);
    }

    // Reads a Vector2s
    //  8 bytes total are read:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    template<typename T> requires std::same_as<T, Vector2i>
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        auto a(Peek<int32_t>(dummy));
        auto b(Peek<int32_t>(dummy));

        pos = dummy;

        return Vector2i(a, b);
    }

    // Reads a Vector2s
    //  4 bytes total are read:
    //  int16_t: x (2 bytes)
    //  int16_t: y (2 bytes)
    template<typename T> requires std::same_as<T, Vector2s>
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        auto a(Peek<int16_t>(dummy));
        auto b(Peek<int16_t>(dummy));

        pos = dummy;

        return Vector2s(a, b);
    }

    // Reads a Quaternion
    //  16 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    template<typename T> requires std::same_as<T, Quaternion>
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        // TODO use PeekSome
        auto a(Peek<float>(dummy));
        auto b(Peek<float>(dummy));
        auto c(Peek<float>(dummy));
        auto d(Peek<float>(dummy));

        // 1.00000024
        if (std::abs(a) > 1.000001 || std::abs(b) > 1.000001 || std::abs(c) > 1.000001 || std::abs(d) > 1.000001)
            throw std::runtime_error("bad quaternion range");

        auto sqlength = a * a + b * b + c * c + d * d;
        if (std::abs(sqlength - 1.f) > 0.001f)
            throw std::runtime_error("bad quaternion length");

        //x1q = std::min(a, x1q); y1q = std::min(b, y1q); z1q = std::min(c, z1q); w1q = std::min(d, w1q);
        //x2q = std::max(a, x2q); y2q = std::max(b, y2q); z2q = std::max(c, z2q); w2q = std::min(d, w2q);

        pos = dummy;

        return Quaternion{ a, b, c, d };
    }

    // Reads an enum type
    //  - Bytes read depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    decltype(auto) Peek(size_t& pos) const {
        return static_cast<Enum>(Peek<std::underlying_type_t<Enum>>(pos));
    }

    // Read a UTF-8 encoded C# char
    //  Will advance 1 -> 3 bytes, depending on size of first char
    template<typename T> requires std::is_same_v<T, char16_t>
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        auto b1 = Peek<uint8_t>(pos);

        // 4 byte
        if (b1 >= 0xF0) {
            throw std::runtime_error("4-byte utf8 unsupported");
        }
        // 3 byte
        else if (b1 >= 0xE0) {
            auto b2 = Peek<uint8_t>(dummy) & 0x3F; //CHECK_TRAILING(b2);
            auto b3 = Peek<uint8_t>(dummy) & 0x3F; //CHECK_TRAILING(b3);
            pos = dummy;
            return ((b1 & 0xF) << 12) | (b2 << 6) | b3;
        }
        // 2 byte
        else if (b1 >= 0xC0) {
            auto b2 = Peek<uint8_t>(dummy) & 0x3F; //CHECK_TRAILING(b2);
            pos = dummy;
            return ((b1 & 0x1F) << 6) | b2;
        }
        // 1 byte
        else { // if
            pos = dummy;
            return b1 & 0x7F;
        }
    }

    template<typename T> requires std::is_same_v<T, UserProfile>
    decltype(auto) Peek(size_t& pos) const {
        size_t dummy = pos;

        auto name = Peek<std::string>(dummy);
        auto gamerTag = Peek<std::string>(dummy);
        auto networkUserId = Peek<std::string>(dummy);

        pos = dummy;

        return UserProfile(std::move(name), std::move(gamerTag), std::move(networkUserId));
    }



    // Generic type peek
    template<typename T>
    decltype(auto) Peek() const {
        size_t dummy = Position();
        return Peek<T>(dummy);
    }

    // Generic type read
    template<typename T>
    decltype(auto) Read() {
        auto pos = Position();
        auto out = Peek<T>(pos);
        SetPos(pos);
        return out;
    }



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
    decltype(auto) ReadInt64Wrapper() { return (Int64Wrapper) Read<int64_t>(); }

    decltype(auto) ReadUInt8() { return Read<uint8_t>(); }
    decltype(auto) ReadUInt16() { return Read<uint16_t>(); }
    decltype(auto) ReadUInt32() { return Read<uint32_t>(); }
    decltype(auto) ReadUInt64() { return Read<uint64_t>(); }
    decltype(auto) ReadUInt64Wrapper() { return (UInt64Wrapper) Read<uint64_t>(); }

    decltype(auto) ReadFloat() { return Read<float>(); }
    decltype(auto) ReadDouble() { return Read<double>(); }
    
    decltype(auto) ReadChar() { return Read<char16_t>(); }



    // Deserialize a reader to a tuple of types
    template<class...Ts, class RD>
    static std::tuple<Ts...> Deserialize(RD& reader) {
        return { reader.template Read<Ts>()... };
    }


#if VH_IS_ON(VH_USE_MODS)
    sol::object DeserializeOneLua(sol::state_view state, IModManager::Type type) {
        switch (type) {
        case IModManager::Type::BYTES:
            // Will be interpreted as sol container type
            // see https://sol2.readthedocs.io/en/latest/containers.html
            return sol::make_object(state, ReadBytes());
        case IModManager::Type::STRING:
            // Primitive: string
            return sol::make_object(state, ReadString());
        case IModManager::Type::ZDOID:
            // Userdata: ZDOID
            return sol::make_object(state, ReadZDOID());
        case IModManager::Type::VECTOR3f:
            // Userdata: Vector3f
            return sol::make_object(state, ReadVector3f());
        case IModManager::Type::VECTOR2i:
            // Userdata: Vector2i
            return sol::make_object(state, ReadVector2i());
        case IModManager::Type::QUATERNION:
            // Userdata: Quaternion
            return sol::make_object(state, ReadQuaternion());
        case IModManager::Type::STRINGS:
            // Container type of Primitive: string
            return sol::make_object(state, ReadStrings());
        case IModManager::Type::BOOL:
            // Primitive: boolean
            return sol::make_object(state, ReadBool());
        case IModManager::Type::INT8:
            // Primitive: number
            return sol::make_object(state, ReadInt8());
        case IModManager::Type::INT16:
            // Primitive: number
            return sol::make_object(state, ReadInt16());
        case IModManager::Type::INT32:
            // Primitive: number
            return sol::make_object(state, ReadInt32());
        case IModManager::Type::INT64:
            // Userdata: Int64Wrapper
            return sol::make_object(state, ReadInt64());
        case IModManager::Type::UINT8:
            // Primitive: number
            return sol::make_object(state, ReadUInt8());
        case IModManager::Type::UINT16:
            // Primitive: number
            return sol::make_object(state, ReadUInt16());
        case IModManager::Type::UINT32:
            // Primitive: number
            return sol::make_object(state, ReadUInt32());
        case IModManager::Type::UINT64:
            // Userdata: UInt64Wrapper
            return sol::make_object(state, ReadUInt64Wrapper());
        case IModManager::Type::FLOAT:
            // Primitive: number
            return sol::make_object(state, ReadFloat());
        case IModManager::Type::DOUBLE:
            // Primitive: number
            return sol::make_object(state, ReadDouble());
        default:
            throw std::runtime_error("invalid mod DataReader type");
        }
    }

    sol::variadic_results DeserializeLua(sol::state_view state, const IModManager::Types& types) {
        sol::variadic_results results;

        for (auto&& type : types) {
            results.push_back(DeserializeOneLua(state, type));
        }

        return results;
    }
#endif
};
