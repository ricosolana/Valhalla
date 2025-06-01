#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <string_view>
#include <type_traits>
#include <vector>
#include <bit>
#include <cstdint>

#include "VUtilsTraits.h"

namespace avledet::util {

    /*
    struct ReaderObject {
        Reader& reader;

        //template <class T>
        //friend void operator=(T& rhs, ReaderObject& lhs) {
        //void operator=(int& rhs) {
        //
        //}

        //template<typename T>
        //    requires (!std::same_as<T, std::string>)
        //operator T() {
        //    return reader.read<T>();
        //}

        operator std::string() {
            return reader.read<std::string>();
        }
    };*/

    class NetworkShape { };
    class DiskShape { };

    template <class Sh>
    concept StreamableShape = std::same_as<Sh, NetworkShape> || std::same_as<Sh, DiskShape>;

    //template <class F>
    //concept is_form_type = std::is_void_v<F> || std::same_as<F, NetForm> || std::same_as<F, DiskForm>;

    //template <class F>
    //concept is_not_form_type = !is_form_type<F>;

    // Define a serializer for type T, with an optional form F (NetForm / DiskForm)
    //template <class T, class F = void>
    //    requires is_form_type<F>
    //struct Streamer { };
    template <class T, StreamableShape Sh = NetworkShape>
    struct Streamer { };

    //template <class ...Args>
    //struct Streamer {};

    //template <class T>
    //    requires (!std::is_void_v<T> && std::is_same_v<T, NetForm> && !std::is_same_v<F, DiskForm>)
    //    //requires (std::is_same_v<F, void> || std::is_same_v<F, NetForm> || std::is_same_v<F, DiskForm>)
    //struct Streamer {};
    //
    //template <class F>
    //    requires (std::is_void_v<F> || std::is_same_v<F, NetForm> || std::is_same_v<F, DiskForm>)
    //struct Streamer {};

    // Define a serializer for type T
    //template <class T>
    //struct Streamer {};

    class Stream {
        static_assert(std::endian::native == std::endian::little, "platform endianness not supported");

    protected:
        Stream();
        explicit Stream(std::vector<char> buf);
        explicit Stream(std::vector<char> buf, std::size_t pos);

    public:
        bool is_valid_pos(std::size_t pos) const;
        bool is_valid_offset(std::size_t offset) const;

        void check_pos(std::size_t pos) const;
        void check_offset(std::size_t offset) const;

        std::size_t get_pos() const;
        void set_pos(std::size_t pos);
        void seek(std::size_t offset);

        void unsafe_set_pos(std::size_t pos);
        void unsafe_seek(std::size_t offset);

        bool empty() const;
        std::size_t size() const;
        char* data();
        const char* data() const;

        std::vector<char>& get_buf();

    protected:
        std::vector<char> m_buf;
        std::size_t m_pos;
    };

    class Reader : public Stream {
        template <class T, StreamableShape Sh>
        friend struct Streamer;

    public:
        Reader();
        explicit Reader(std::vector<char> buf);
        explicit Reader(std::vector<char> buf, std::size_t pos);

    private:
        // consider void* instead of char
        void internal_read_bytes(char* outBuf, std::size_t outBufSize);

        std::int32_t read_varint();

    public:
        // Read a file into a buffer object
        static Reader from_file(std::filesystem::path path);

    public:
        template <StreamableShape Sh, class T>
        decltype(auto) read() {
            return Streamer<std::remove_cvref_t<T>, Sh>{}.operator()(*this);
        }

        template <class T>
            requires (!(StreamableShape<T>))
        decltype(auto) read() {
            return read<NetworkShape, T>();
        }


        // keep to test
        template <class ...T>
            requires (sizeof...(T) >= 2 && ((!StreamableShape<T>) && ...))
        std::tuple<T...> read() {
            return { this->template read<T>()... };
        }

        // TODO wrap into read instead
        template<class...T, class RD>
        //[[deprecated("Use read<T...>() instead")]]
        static std::tuple<T...> deserialize(RD& reader) {
            return { reader.template read<T>()... };
        }

    };

    class Writer : public Stream {
        template <class T, StreamableShape Sh>
        friend struct Streamer;

    public:
        Writer();
        explicit Writer(std::vector<char> buf);
        explicit Writer(std::vector<char> buf, std::size_t pos);

    private:
        void internal_write_bytes(const char* inBuf, std::size_t inBufSize);
        void write_varint(std::int32_t value);

    public:
        template <class Sh = NetworkShape, class T>
        void write(T const& obj) {
            static_assert(StreamableShape<Sh>, "Do not specify your template type first! ||| [ie. write<int>(5) is BAD] ||| [ie. write((int)5) is GOOD]");
            Streamer<std::remove_cvref_t<T>, Sh>{}.operator()(*this, obj);
        }

        template <StreamableShape Sh = NetworkShape, class T, class... Args>
            requires (sizeof...(Args) > 0)
        void write(const T& first, const Args&... args) {
            write<Sh>(first);
            write<Sh>(args...);
        }



        // Empty template
        //[[deprecated("Use write(T...) instead")]]
        static void serialize_impl(Writer&) { }

        // Writes variadic parameters into a package
        template <typename T, typename... Types>
        //[[deprecated("Use write(T...) instead")]]
        static decltype(auto) serialize_impl(Writer& pkg, const T& var1, const Types&... var2) {
            pkg.write(var1);

            return serialize_impl(pkg, var2...);
        }

        // Serialize variadic types to an array
        template <typename T, typename... Types>
        //[[deprecated("Use write(T...) instead")]]
        static decltype(auto) serialize(const T& var1, const Types&... var2) {
            Writer writer;
            serialize_impl(writer, var1, var2...);
            return std::vector<char>(writer.get_buf());
        }

        // empty full template
        //[[deprecated("Use write(T...) instead")]]
        static decltype(auto) serialize() {
            return std::vector<char>();
        }
    };



    //Primitive Streamer
    template <class T>
        requires std::is_scoped_enum_v<T> || (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    struct Streamer<T> {
        void operator()(Writer& writer, T const value) const {
            writer.internal_write_bytes(reinterpret_cast<const char*>(&value), sizeof(value));
        }

        decltype(auto) operator()(Reader& reader) const {
            T out{};
            reader.internal_read_bytes(reinterpret_cast<char*>(&out), sizeof(T));
            return out;
        }
    };

    //String Streamer
    template <class T>
        requires std::is_same_v<typename T::value_type, char>
    struct Streamer<T> {
        void operator()(Writer& writer, T const& buf) const {
            if constexpr (avledet::util::traits::has_traits_type<T>) {
                writer.write_varint(static_cast<std::int32_t>(buf.size()));
            } else {
                writer.write(static_cast<std::int32_t>(buf.size()));
            }
            writer.internal_write_bytes(buf.data(), buf.size());
        }

        decltype(auto) operator()(Reader& reader) const {
            auto count = static_cast<std::uint32_t>(
                avledet::util::traits::has_traits_type<T>
                    ? reader.read_varint()
                    : reader.read<std::int32_t>());

            reader.check_offset(count);

            auto result = T(reader.data() + reader.get_pos(),
                reader.data() + reader.get_pos() + count);

            reader.unsafe_seek(count);

            return result;
        }
    };

    //String* Streamer
    template <class T>
        requires (!std::is_same_v<T, char> 
            && std::is_same_v<std::remove_const_t<std::remove_pointer_t<std::remove_all_extents_t<T>>>, char>)
    struct Streamer<T> {
        void operator()(Writer& writer, T const& value) const {
            writer.write(std::string_view(value));
        }
    };

    //Object Array Streamer
    template <class T>
        requires (avledet::util::traits::is_iterable<T>
    && !std::same_as<typename T::value_type, char>)
    struct Streamer<T> {
        static_assert(!std::is_arithmetic_v<T>, "do not use slow overload for arithmetic types");

        void operator()(Writer& writer, T const& value) const {
            writer.write(static_cast<std::int32_t>(value.size()));
            for (auto&& e : value) {
                writer.write(e);
            }
        }

        decltype(auto) operator()(Reader& reader) const {
            T value{};
            const auto size = reader.read<std::int32_t>();
            for (int i = 0; i < size; i++) {
                value.insert(value.end(),
                    reader.read<typename T::value_type>());
            }
            return value;
        }
    };

    //UTF8 Character Streamer
    template<>
    struct Streamer<char16_t> {
        void operator()(Writer& writer, char16_t const value) const {
            if (value < 0x80) {
                writer.write(static_cast<std::uint8_t>(value));
            }
            else if (value < 0x0800) {
                writer.write(static_cast<std::uint8_t>(((value >> 6) & 0x1F) | 0xC0));
                writer.write(static_cast<std::uint8_t>(((value >> 0) & 0x3F) | 0x80));
            }
            else { // if (value < 0x010000) {
                writer.write(static_cast<std::uint8_t>(((value >> 12) & 0x0F) | 0xE0));
                writer.write(static_cast<std::uint8_t>(((value >> 6) & 0x3F) | 0x80));
                writer.write(static_cast<std::uint8_t>(((value >> 0) & 0x3F) | 0x80));
            }
        }

        decltype(auto) operator()(Reader& reader) const {
            char16_t b1 = reader.read<std::uint8_t>();

            // 3 byte
            if (b1 >= 0xE0) {
                auto b2 = reader.read<std::uint8_t>() & 0x3F;
                auto b3 = reader.read<std::uint8_t>() & 0x3F;
                return ((b1 & 0xF) << 12) | (b2 << 6) | b3;
            }
            // 2 byte
            else if (b1 >= 0xC0) {
                auto b2 = reader.read<uint8_t>() & 0x3F;
                return ((b1 & 0x1F) << 6) | b2;
            }
            // 1 byte
            else {
                return b1 & 0x7F;
            }
        }
    };

    template <>
    struct Streamer<Reader> {
        void operator()(Writer& writer, Reader const& value) const {
            writer.write(value.m_buf);
        }

        decltype(auto) operator()(Reader& reader) const {
            return Reader(reader.read<std::vector<char>>());
        }
    };

}// namespace avledet::util

using DataReader = avledet::util::Reader;
using DataWriter = avledet::util::Writer;
