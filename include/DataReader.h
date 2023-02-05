#pragma once

#include "VUtils.h"
#include "NetID.h"
#include "Vector.h"
#include "Quaternion.h"

class DataReader {
public:
    //const BYTES_t *m_provider = nullptr;
    std::reference_wrapper<const BYTES_t> m_provider;
    int32_t m_pos;

private:
    void ReadBytes(BYTE_t* buffer, int32_t count);

    // Read a single byte from the buffer (slow if used to read a bunch of bytes)
    // Throws if end of buffer is reached
    //BYTE_t ReadByte();

    // Reads count bytes overriding the specified vector
    // Throws if not enough bytes to read
    void ReadBytes(std::vector<BYTE_t>& vec, int32_t count);

    // Reads count bytes overriding the specified string
    // '\0' is not included in count (raw bytes only)
    // Will throw if count exceeds end
    //void Read(std::string& s, uint32_t count);

    int32_t Read7BitEncodedInt() {
        int32_t out = 0;
        int32_t num2 = 0;
        while (num2 != 35) {
            auto b = Read<BYTE_t>();
            out |= (int32_t)(b & 127) << num2;
            num2 += 7;
            if ((b & 128) == 0)
            {
                return out;
            }
        }
        throw std::runtime_error("bad encoded int");
    }

public:
    DataReader(const BYTES_t& bytes) : m_provider(bytes), m_pos(0) {}

    DataReader(const BYTES_t& bytes, int32_t pos) : m_provider(bytes) {
        SetPos(pos);
    }

    DataReader Sub() {
        auto count = Read<int32_t>();
        return DataReader(m_provider.get(), Position());
    }

    // Gets all the data in the package
    //BYTES_t Bytes() const {
    //    return *m_provider;
    //}

    // Gets all the data in the package
    //void Bytes(BYTES_t &out) const {
    //    out = *m_buf;
    //}

    //BYTES_t Remaining() {
    //    BYTES_t res;
    //    Read(res, m_provider->size() - m_pos);
    //    return res;
    //}



    // Returns the length of this stream
    int32_t Length() const {
        auto size = m_provider.get().size();
        //if (size > static_cast<decltype(size)>(std::numeric_limits<int32_t>::max()))
            //throw std::runtime_error("int32_t size exceeded");

        return static_cast<int32_t>(size);
    }

    // Returns the position of this stream
    int32_t Position() const {
        return m_pos;
    }

    // Sets the positino of this stream
    void SetPos(int32_t pos);

public:
    //  Reads a primitive type
    template<typename T> requires std::is_fundamental_v<T>
    T Read() {
        T out;
        ReadBytes(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    // Reads a string
    template<typename T> requires std::same_as<T, std::string>
    T Read() {
        auto count = Read7BitEncodedInt();
        if (count == 0)
            return "";

        if (m_pos + count > Length())
            throw std::runtime_error("NetPackage::Read<std::string>() length exceeded");

        std::string s;

        s.insert(s.end(),
            m_provider.get().begin() + m_pos,
            m_provider.get().begin() + m_pos + count);

        m_pos += count;

        return s;
    }

    // Reads a byte array
    //  uint32_t:   size
    //  BYTES_t:    data
    template<typename T> requires std::same_as<T, BYTES_t>
    T Read() {
        T out;
        ReadBytes(out, Read<int32_t>());
        return out;
    }

    // Reads a container of supported types
    //  uint32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (is_iterable_v<Iterable> && !std::is_same_v<Iterable, std::string> && !std::is_same_v<Iterable, BYTES_t>)
        Iterable Read() 
    {
        const auto count = Read<int32_t>();

        if (count < 0)
            throw std::runtime_error("negative count");

        Iterable out;

        for (int32_t i=0; i < count; i++) {
            out.insert(out.end(), Read<Iterable::value_type>());
        }

        return out;
    }



    // Reads a NetPackage
    //  uint32_t:   size
    //  BYTES_t:    data
    //template<typename T> requires std::same_as<T, NetPackage>
    //T Read() {
    //    auto count = Read<uint32_t>();
    //
    //    NetPackage pkg(count);
    //    Read(pkg.m_stream.m_buf, count);
    //    assert(pkg.m_stream.Length() == count);
    //    assert(pkg.m_stream.Position() == 0);
    //    return pkg;
    //}

    // Reads a ZDOID
    //  12 bytes total are read:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    template<typename T> requires std::same_as<T, ZDOID>
    T Read() {
        auto a(Read<int64_t>());
        auto b(Read<uint32_t>());
        return ZDOID(a, b);
    }

    // Reads a Vector3
    //  12 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    template<typename T> requires std::same_as<T, Vector3>
    T Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        return Vector3{ a, b, c };
    }

    // Reads a Vector2i
    //  8 bytes total are read:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    template<typename T> requires std::same_as<T, Vector2i>
    T Read() {
        auto a(Read<int32_t>());
        auto b(Read<int32_t>());
        return Vector2i{ a, b };
    }

    // Reads a Quaternion
    //  16 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    template<typename T> requires std::same_as<T, Quaternion>
    T Read() {
        auto a(Read<float>());
        auto b(Read<float>());
        auto c(Read<float>());
        auto d(Read<float>());
        return Quaternion{ a, b, c, d };
    }

    // Reads an enum type
    //  Bytes read depend on the underlying value
    template<typename E> requires std::is_enum_v<E>
    E Read() {
        return static_cast<E>(Read<std::underlying_type_t<E>>());
    }

    // Reads a byte array 
    // The target vector be overwritten
    //void Read(BYTES_t& out);

        // Deserialize a NetPackage to a tuple of types
    template<class...Ts, class RD>
    static std::tuple<Ts...> Deserialize(RD& reader) {
        return { reader.template Read<Ts>()... };
    }
};
