#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <bit>
#include <cstdint>
#include <functional>

#include "VUtilsTraits.h"

namespace avledet::util {

    template <class ...T>
    struct Streamer { };



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
        template <class ...T>
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
        // auto a = read(a, b, c, ..)
        template <class ...T>
            requires (sizeof...(T) >= 1)
        decltype(auto) read(T&&... args) {
            return Streamer<std::remove_cvref_t<T>...>{}.operator()(*this, std::forward<T>(args)...);
        }

        // auto a = read<int>()
        template <class T>
        decltype(auto) read() {
            return Streamer<std::remove_cvref_t<T>>{}.operator()(*this);
        }

        // auto [a, b, c] = read<int, char, string>();
        template <class ...T>
            requires (sizeof...(T) >= 2)
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
        template <class ...T>
        friend struct Streamer;

    public:
        Writer();
        explicit Writer(std::vector<char> buf);
        explicit Writer(std::vector<char> buf, std::size_t pos);

    private:
        void internal_write_bytes(const char* inBuf, std::size_t inBufSize);
        void write_varint(std::int32_t value);

    public:
        // write(a);
        // auto a = write(a, b, c);
        template <class ...T>
            requires (sizeof...(T) >= 1)
        decltype(auto) write(T const&... args) {
            return Streamer<std::remove_cvref_t<T>...>{}.operator()(*this, args...);
        }

        // variadic "serialize"
        //  write(std::tuple<> {});
        template <class T, class... Args>
            requires (sizeof...(Args) >= 1)
        void write(std::tuple<T, Args...> const& args) {
            return [&]<std::size_t... I>(std::index_sequence<I...>) {
                ((write(std::get<I>(args))), ...);
            }(std::make_index_sequence<sizeof...(Args)>{});
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
            writer.write((std::uint32_t)value.size());
            for (auto&& e : value) {
                writer.write(e);
            }
        }

        decltype(auto) operator()(Reader& reader) const {
            T value{};
            auto size = reader.read<std::uint32_t>();
            for (decltype(size) i = 0; i < size; i++) {
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
                auto b2 = reader.read<std::uint8_t>() & 0x3F;
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

    template <typename T> 
    concept invokable_read1 = requires(T t) {
        { &T::operator() }; // -> std::convertible_to<typename From, typename To>;
        //{ !std::is_same_v<Writer&, std::tuple_element_t<0, typename VUtils::Traits::func_traits<T>::args_type>> };
        
        //{ t == u } -> std::convertible_to<bool>;
        //{ u == t } -> std::convertible_to<bool>;
    };

    // nested byte write
    //  then returns to the original position, writes the following bytes written
    //template <class T>
    template <invokable_read1 T>
        //requires (std::is_invocable_v<T, Writer&> 
        //    && std::is_same_v<
        //        std::tuple_element_t<0, typename VUtils::Traits::func_traits<T>::raw_args_type>,
        //        Writer&>
        //)

    struct Streamer<T> { //<std::function<void(Writer&)>> {
        void operator()(Writer& writer, T const& value) const {
            //static_assert(std::is_invocable_v<T, Writer&>, "Writer::write(func) must have a Writer& as argument");

            const auto start = writer.get_pos();
            std::uint32_t count = 0;
            writer.write(count); //dummy
    
            // call func...
            value(std::ref(writer));
    
            const auto end = writer.get_pos();
            writer.set_pos(start);
            count = end - start - sizeof(count);
            assert(count >= 0);
            writer.write(count);
            writer.set_pos(end);
        }

        // usage: (as for_each)
        //  reader.read([](Object next) {
        //      ...
        //  })
        bool operator()(Reader& reader, T const& func) const {
            static_assert(!std::is_invocable_v<T, Writer&>, "Reader::read(func) must accept simple readable types (not a 'Writer' as arg)");
            using Type = std::tuple_element_t<0, typename VUtils::Traits::func_traits<T>::raw_args_type>;

            auto count = reader.read<std::uint32_t>();
            for (decltype(count) i=0; i < count; i++) {
                func(reader.read<Type>());
            }

            return count > 0;
        }
    };



    //template <class T>
    //concept invokable_read = (T) {
    //    Streamer<typename std::tuple_element_t<0, typename VUtils::Traits::func_traits<T>::args_type>>{}
//
    //    //Streamer<std::tuple_element_t<0, typename VUtils::Traits::func_traits<T>::args_type>>{}
    //    //    .operator()(std::declval<Reader&>())
    //};

    // container foreach(...) read
    //  writes all sub counts
    //  then returns to the original position, writes the following bytes written
    //template <invokable_read T>
    
    //template <class T>
    //    requires std::is_invocable_v<
    //        decltype(Streamer<    
    //            typename std::tuple_element_t<0, typename VUtils::Traits::func_traits<T>::args_type>
    //        >{}.operator()),
    //        Reader
    //    >
    /*
    template <invokable_read1 T>
    struct Streamer<T> {
        //makes no sense to deserialize a foreach function
        int operator()(Reader& reader, T const& func) const {
            assert(false); //TODO
            //auto count = reader.read<std::uint32_t>();
            //for (decltype(count) i=0; i < count; i++) {
            //    
            //}
            return 1;
        }
    };
    */
}// namespace avledet::util

using DataReader = avledet::util::Reader;
using DataWriter = avledet::util::Writer;
