#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <quill/Backend.h>
#include <quill/bundled/fmt/ostream.h>
#include <quill/bundled/fmt/ranges.h>
#include <quill/Frontend.h>
#include <quill/HelperMacros.h>
#include <quill/Logger.h>
#include <quill/LogMacros.h>
#include <quill/sinks/ConsoleSink.h>

#include <ankerl/unordered_dense.h>

#include "BitPack.h"
#include "CompileSettings.h"
#include "DataStream.h"

namespace avledet::util {
    class ZDOID
    {
        friend struct ankerl::unordered_dense::hash<ZDOID>;
        //friend class ZDO;

        using UType = std::uint32_t;

        // User: 0, ID: 1
        BitPack<UType, AVL_USERID_BITS_I_, sizeof(UType) * 8 - AVL_USERID_BITS_I_> m_pack;

        static inline std::array<std::int64_t, decltype(m_pack)::capacity<0>::value> INDEXED_USERID;

        static constexpr auto USERID_PACK_INDEX = 0;
        static constexpr auto ID_PACK_INDEX     = 1;

        static constexpr auto BIT_SHARING = 4;

      public:
        static const ZDOID NONE;

      private:
        // Get the index of a UserID
        //  The UserID is inserted if it does not exist
        //  Returns the insertion index or the existing index of the UserID
        static std::size_t _get_user_id_index(std::int64_t user_id)
        {
            if (user_id == 0)
                return 0;

            //We start at index 4, because the first 4 are reserved for ordinal bit sharing
            for (std::size_t i = BIT_SHARING; i < INDEXED_USERID.size(); i++) {
                // after first index, values of 0 mean free
                if (INDEXED_USERID[i] == 0) {
                    INDEXED_USERID[i] = user_id;
                    return i;
                } else if (INDEXED_USERID[i] == user_id) {
                    return i;
                }
            }

            // TODO
            //  there is no real exhaustion protection for UserID, only for ID
            //  we are screwed, and thus a server restart is mandatory
            //  After knowing how 2^USERID_BITS number of PLAYERS have joined / left, restarts are expected
            throw std::runtime_error("ID pool exhaustion");
        }

        static std::int64_t _get_user_id(std::size_t index)
        {
            // index 0 means no owner,
            //if (index < BIT_SHARING)
            //return 0;

            if (index < INDEXED_USERID.size())
                return INDEXED_USERID[index];

            throw std::runtime_error("user id by index not found");
        }

        // Retrieve the index of the UserID
        decltype(auto) _get_user_id_index() const
        {
            return m_pack.get<USERID_PACK_INDEX>();
        }

        // Set the associated UserID index
        void _set_user_id_index(decltype(m_pack)::type index)
        {
            m_pack.set<USERID_PACK_INDEX>(index);
        }

      public:
        ZDOID() = default;
        ZDOID(std::int64_t user_id, std::uint32_t id);

        //ZDOID(const ZDOID &) = default;

        bool operator==(const ZDOID &other) const noexcept
        {
            return this->m_pack == other.m_pack;
            //return this->m_user_id == other.m_user_id
            //&& this->m_id == other.m_id;
        }

        bool operator!=(const ZDOID &other) const noexcept
        {
            return !(*this == other);
        }

        // Return whether this has a value besides NONE
        operator bool() const noexcept
        {
            return *this != ZDOID::NONE;
        }

        std::int64_t get_user_id() const
        {
            return INDEXED_USERID[_get_user_id_index()];
        }

        void set_user_id(std::int64_t user_id)
        {
            // logic:
            //  if setting to 0, and current index is NOT sharing, set index to 0
            //  if setting to any UserID != 0, and we are bitsharing greater than 0, we THROW

            auto sharing = _get_user_id_index();
            if (user_id == 0 && sharing >= BIT_SHARING) {
                _set_user_id_index(_get_user_id_index(user_id));
                //_set_user_id_index(0); //AKA
            } else if (user_id != 0 && sharing > 0 && sharing < BIT_SHARING) {
                // panic
                throw std::runtime_error("ID pool exhaustion of user id set");
            } else {
                _set_user_id_index(_get_user_id_index(user_id));
            }
        }

        std::uint32_t get_id() const
        {
            //If we are borrowing an extended UserID, then shift our result
            auto result  = m_pack.get<ID_PACK_INDEX>();
            auto sharing = _get_user_id_index();
            if (sharing < BIT_SHARING) {
                result |= (sharing << decltype(m_pack)::count<ID_PACK_INDEX>());
            }
            return result;
        }

        void set_id(std::uint32_t id)
        {
            if (_get_user_id_index()
                < BIT_SHARING) {// [user_id.index < BIT_SHARING] also refers to 0; AKA no owner
                // we must set the user id
                auto sharing = id >> decltype(m_pack)::count<ID_PACK_INDEX>();
                if (sharing
                    < BIT_SHARING) {// if there are upper bits set, which we can handle (without overflow)
                    _set_user_id_index(sharing);
                    id &= decltype(m_pack)::capacity<
                            ID_PACK_INDEX>();// the & trims off the significant bits, which we just denoted within user id index
                }
            }

            if (id > decltype(m_pack)::capacity<ID_PACK_INDEX>()) {
                // exhaustion
                throw std::domain_error("exhaustion of ID pool");
            }

            assert(id <= decltype(m_pack)::capacity<ID_PACK_INDEX>());

            m_pack.set<ID_PACK_INDEX>(id);
        }
    };

}// namespace avledet::util

using ZDOID = avledet::util::ZDOID;

// Basic ankerl hash
template<>
struct ankerl::unordered_dense::hash<avledet::util::ZDOID>
{
    using is_avalanching = void;// high quality hash

    //static_assert(std::has_unique_object_representations_v<avledet::util::ZDOID>);

    auto operator()(avledet::util::ZDOID const &value) const noexcept -> std::uint64_t
    {
        return ankerl::unordered_dense::detail::wyhash::hash(value.m_pack.get_underlying());
    }
};

template<>
struct avledet::util::Streamer<avledet::util::ZDOID>
{
    void operator()(avledet::util::Writer &writer, avledet::util::ZDOID const &zdoid)
    {
        //assert(false); // TODO
        writer.write(zdoid.get_user_id());//8 is first
        writer.write(zdoid.get_id());     //4 is second
    }

    avledet::util::ZDOID operator()(avledet::util::Reader &reader)
    {
        auto user_id = reader.read<std::int64_t>();
        auto id      = reader.read<std::uint32_t>();
        return avledet::util::ZDOID(user_id, id);
    }
};

std::ostream &operator<<(std::ostream &st, ZDOID const &zdoid);

QUILL_LOGGABLE_DEFERRED_FORMAT(ZDOID)
