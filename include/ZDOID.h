#pragma once

#include <cstdint>
#include <stdexcept>
#include <quill/Backend.h>
#include <quill/bundled/fmt/ostream.h>
#include "QuillHelperMacros.h"
#include "VUtils.h"
#include "BitPack.h"
#include "DataStream.h"

class ZDOID {
    //friend struct ankerl::unordered_dense::hash<ZDOID>;
    friend class ZDO;

    //using UType = uint64_t;

    // User: 0, ID: 1
    //BitPack<UType, VH_USER_BITS_I_, sizeof(UType) * 8 - VH_USER_BITS_I_> m_pack;

    std::int64_t m_user_id{};
    std::uint32_t m_id{};

    
        
    // Indexed UserIDs
    //  Capacity is equal to USER mask due to a ZDOID USER index of 0 referring to no active owner
    //static std::array<int64_t, (1 << 6) - 1> INDEXED_USERS;

    //static std::array<int64_t, decltype(m_pack)::capacity<0>::value> INDEXED_USERS;

    //static constexpr auto USER_PACK_INDEX = 0;
    //static constexpr auto ID_PACK_INDEX = 1;

public:
    static const ZDOID NONE;

    std::uint32_t m_unusedPadding = 0;

private:
    // Get the index of a UserID
    //  The UserID is inserted if it does not exist
    //  Returns the insertion index or the existing index of the UserID
    /*
    static size_t EnsureUserIDIndex(int64_t owner) {
        if (!owner)
            return 0;

        for (size_t i = 1; i < INDEXED_USERS.size(); i++) {
            // Assume that a blank index prior to an existing UserID being found
            //  means that the UserID does not exist (so insert it)
            if (!INDEXED_USERS[i]) {
                INDEXED_USERS[i] = owner;
                return i;
            }
            else if (INDEXED_USERS[i] == owner) {
                return i;
            }
        }

        // TODO this is definitely reachable, assuming the server runs long enough
        //  for enough unique players to join, causing the INDEXED_USERS loop to completely finish
        //  and reach this point
        std::unreachable();
    }

    static int64_t GetUserIDByIndex(size_t index) {
        //if (!index)
            //return 0;

        //assert((index || (INDEXED_USERS[index] == 0))
            //&& "Array[0] should be 0 to represent no-owner");

        if (index < INDEXED_USERS.size())
            return INDEXED_USERS[index];

        throw std::runtime_error("user id by index not found");
    }*/

public:
    ZDOID() = default;
    ZDOID(std::int64_t user_id, std::uint32_t id);
    ZDOID(const ZDOID&) = default;

    bool operator==(const ZDOID &other) const noexcept {
        //return this->m_pack == other.m_pack;
        return this->m_user_id == other.m_user_id 
            && this->m_id == other.m_id;
    }

    bool operator!=(const ZDOID &other) const noexcept {
        return !(*this == other);
    }
    
    // Return whether this has a value besides NONE
    operator bool() const noexcept {
        //return m_pack;
        return *this != ZDOID::NONE;
    }

    // TODO rename to User
    std::int64_t get_user_id() const {
        //return INDEXED_USERS[_GetUserIDIndex()];
        return this->m_user_id;
    }

    // Rename to SetUserID
    void set_user_id(std::int64_t user_id) {
        //_SetUserIDIndex(this->EnsureUserIDIndex((int64_t)owner));
        this->m_user_id = user_id;
    }


    /*
    // Retrieve the index of the UserID
    uint32_t _GetUserIDIndex() const {
        return m_pack.Get<USER_PACK_INDEX>();
    }

    // Set the associated UserID index 
    void _SetUserIDIndex(decltype(m_pack)::type index) {
        m_pack.Set<USER_PACK_INDEX>(index);
    }*/

    // TODO rename to GetID
    std::uint32_t get_id() const {
        //return m_pack.Get<ID_PACK_INDEX>();
        return this->m_id;
    }

    // TODO rename to SetID
    void set_id(std::uint32_t id) {
        //m_pack.Set<ID_PACK_INDEX>(uid);
        this->m_id = id;
    }



    friend std::ostream& operator<<(std::ostream& st, ZDOID const& zdoid) {
        st << (std::int64_t)zdoid.get_user_id() << ":" << zdoid.get_id();
        return st;
    }
};

namespace avledet::sync {
    using ZDOID = ::ZDOID;
}

// Basic ankerl hash
template <>
struct ankerl::unordered_dense::hash<avledet::sync::ZDOID> {
    using is_avalanching = void; // high quality hash

    static_assert(std::has_unique_object_representations_v<avledet::sync::ZDOID>);

    auto operator()(avledet::sync::ZDOID const& value) const noexcept -> std::uint64_t {
        assert(value.m_unusedPadding == 0);
        return ankerl::unordered_dense::detail::wyhash::hash(&value, sizeof(value));
    }
};

template <>
struct avledet::util::Streamer<avledet::sync::ZDOID> {
    void operator()(avledet::util::Writer& writer, avledet::sync::ZDOID const& zdoid) {
        //assert(false); // TODO
        writer.write(zdoid.get_user_id()); //8 is first
        writer.write(zdoid.get_id());//4 is second
    }

    avledet::sync::ZDOID operator()(avledet::util::Reader& reader) {
        //throw std::runtime_error("TODO");
        return avledet::sync::ZDOID(
            reader.read<std::int64_t>(),
            reader.read<std::uint32_t>()
        );
    }
};

QUILL_LOGGABLE_DIRECT_FORMAT(ZDOID)
