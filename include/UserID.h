#pragma once

#include <string>
#include <string_view>
#include <bitset>
#include <type_traits>
#include <Stream.h>
#include <ankerl/unordered_dense.h>

namespace avledet::util {

    class UserID {
    private:
        std::uint16_t m_index{};

        static inline ankerl::unordered_dense::map<decltype(m_index), std::int64_t> m_ids {};

    public:
        UserID();
        UserID(std::int64_t value);

        std::int64_t get_value() const;

        bool operator==(const UserID&) const;
        operator bool() const;
    };

    template <>
    struct Streamer<UserID> {
        static_assert(std::is_same_v<
            std::int64_t,
            std::invoke_result_t<decltype(&UserID::get_value), UserID&>>);

        void operator()(Writer& writer, UserID const& value) {
            writer.write(value.get_value());
        }

        decltype(auto) operator()(Reader& reader) {
            return UserID(reader.read<std::int64_t>());
        }
    };

}// namespace avledet::util
