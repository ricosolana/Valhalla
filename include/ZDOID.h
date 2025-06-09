#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/HelperMacros.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/bundled/fmt/ostream.h>
#include <quill/bundled/fmt/ranges.h>
#include <quill/sinks/ConsoleSink.h>

#include <ankerl/unordered_dense.h>

#include "CompileSettings.h"
#include "BitPack.h"
#include "DataStream.h"

namespace avledet::util {
    class ZDOID {
        friend struct ankerl::unordered_dense::hash<ZDOID>;
        //friend class ZDO;

        using UType = std::uint32_t;

        // User: 0, ID: 1
        BitPack<UType, VH_USERID_BITS_I_, sizeof(UType) * 8 - VH_USERID_BITS_I_> m_pack;

        static inline std::array<std::int64_t, decltype(m_pack)::capacity<0>::value> INDEXED_USERID;

        static constexpr auto USERID_PACK_INDEX = 0;
        static constexpr auto ID_PACK_INDEX = 1;

    public:
        static const ZDOID NONE;

    private:
        // Get the index of a UserID
        //  The UserID is inserted if it does not exist
        //  Returns the insertion index or the existing index of the UserID
        static std::size_t _get_user_id_index(std::int64_t user_id) {
            if (user_id == 0)
                return 0;

            for (std::size_t i = 1; i < INDEXED_USERID.size(); i++) {
                // after first index, values of 0 mean free
                if (INDEXED_USERID[i] == 0) {
                    INDEXED_USERID[i] = user_id;
                    return i;
                }
                else if (INDEXED_USERID[i] == user_id) {
                    return i;
                }
            }

            // TODO
            //  there is no real exhaustion protection for UserID, only for ID
            //  we are screwed, and thus a server restart is mandatory
            //  After knowing how 2^USERID_BITS number of PLAYERS have joined / left, restarts are expected
            throw std::runtime_error("ID pool exhaustion");
        }

        static std::int64_t _get_user_id(std::size_t index) {
            // index 0 means no owner, 
            if (index == 0)
                return 0;

            if (index < INDEXED_USERID.size())
                return INDEXED_USERID[index];

            throw std::runtime_error("user id by index not found");
        }

        // Retrieve the index of the UserID
        decltype(auto) _get_user_id_index() const {
            return m_pack.Get<USERID_PACK_INDEX>();
        }

        // Set the associated UserID index 
        void _set_user_id_index(decltype(m_pack)::type index) {
            m_pack.Set<USERID_PACK_INDEX>(index);
        }

    public:
        ZDOID() = default;
        ZDOID(std::int64_t user_id, std::uint32_t id);
        ZDOID(const ZDOID&) = default;

        bool operator==(const ZDOID &other) const noexcept {
            return this->m_pack == other.m_pack;
            //return this->m_user_id == other.m_user_id 
                //&& this->m_id == other.m_id;
        }

        bool operator!=(const ZDOID &other) const noexcept {
            return !(*this == other);
        }
        
        // Return whether this has a value besides NONE
        operator bool() const noexcept {
            return *this != ZDOID::NONE;
        }

        std::int64_t get_user_id() const {
            return INDEXED_USERID[_get_user_id_index()];
        }

        void set_user_id(std::int64_t user_id) {
            _set_user_id_index(_get_user_id_index(user_id));
        }

        std::uint32_t get_id() const {
            return m_pack.Get<ID_PACK_INDEX>();
        }

        void set_id(std::uint32_t id) {
            if (id > decltype(m_pack)::capacity<ID_PACK_INDEX>()) {
                throw std::runtime_error("ID exhausts id pool");
            }

            m_pack.Set<ID_PACK_INDEX>(id);
        }

    };

}// namespace avledet::util

using ZDOID = avledet::util::ZDOID;

// Basic ankerl hash
template <>
struct ankerl::unordered_dense::hash<avledet::util::ZDOID> {
    using is_avalanching = void; // high quality hash

    //static_assert(std::has_unique_object_representations_v<avledet::util::ZDOID>);

    auto operator()(avledet::util::ZDOID const& value) const noexcept -> std::uint64_t {
        return ankerl::unordered_dense::detail::wyhash::hash(value.m_pack.get_value());
    }
};

template <>
struct avledet::util::Streamer<avledet::util::ZDOID> {
    void operator()(avledet::util::Writer& writer, avledet::util::ZDOID const& zdoid) {
        //assert(false); // TODO
        writer.write(zdoid.get_user_id()); //8 is first
        writer.write(zdoid.get_id());//4 is second
    }

    avledet::util::ZDOID operator()(avledet::util::Reader& reader) {
        auto user_id = reader.read<std::int64_t>();
        auto id = reader.read<std::uint32_t>();
        return avledet::util::ZDOID(user_id, id);
    }
};

std::ostream& operator<<(std::ostream& st, ZDOID const& zdoid);

QUILL_LOGGABLE_DEFERRED_FORMAT(ZDOID)
