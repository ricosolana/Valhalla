#pragma once

#include <cstdint>
#include <stdexcept>
#include "VUtils.h"
#include "BitPack.h"

class ZDOID {
    friend struct ankerl::unordered_dense::hash<ZDOID>;
    friend class ZDO;

    using UType = uint64_t;

    // User: 0, ID: 1
    BitPack<UType, VH_USER_BITS_I_, sizeof(UType) * 8 - VH_USER_BITS_I_> m_pack;
        
    // Indexed UserIDs
    //  Capacity is equal to USER mask due to a ZDOID USER index of 0 referring to no active owner
    //static std::array<int64_t, (1 << 6) - 1> INDEXED_USERS;

    static std::array<int64_t, decltype(m_pack)::capacity<0>::value> INDEXED_USERS;

    static constexpr auto USER_PACK_INDEX = 0;
    static constexpr auto ID_PACK_INDEX = 1;

public:
    static const ZDOID NONE;

private:
    // Get the index of a UserID
    //  The UserID is inserted if it does not exist
    //  Returns the insertion index or the existing index of the UserID
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
    }

public:
    ZDOID() = default;

    ZDOID(OWNER_t owner, uint32_t uid) {
        this->SetOwner(owner);
        this->SetUID(uid);
    }

    ZDOID(const ZDOID&) = default;

    bool operator==(ZDOID other) const {
        return this->m_pack == other.m_pack;
    }

    bool operator!=(ZDOID other) const {
        return !(*this == other);
    }
    
    // Return whether this has a value besides NONE
    operator bool() const noexcept {
        return m_pack;
    }

    // TODO rename to User
    OWNER_t GetOwner() const {
        return INDEXED_USERS[_GetUserIDIndex()];
    }

    // Rename to SetUserID
    void SetOwner(OWNER_t owner) {
        _SetUserIDIndex(this->EnsureUserIDIndex((int64_t)owner));
    }



    // Retrieve the index of the UserID
    uint32_t _GetUserIDIndex() const {
        return m_pack.Get<USER_PACK_INDEX>();
    }

    // Set the associated UserID index 
    void _SetUserIDIndex(decltype(m_pack)::type index) {
        m_pack.Set<USER_PACK_INDEX>(index);
    }

    // TODO rename to GetID
    uint32_t GetUID() const {
        return m_pack.Get<ID_PACK_INDEX>();
    }

    // TODO rename to SetID
    void SetUID(uint32_t uid) {
        m_pack.Set<ID_PACK_INDEX>(uid);
    }



    friend std::ostream& operator<<(std::ostream& st, ZDOID zdoid) {
        return st << (int64_t)zdoid.GetOwner() << ":" << zdoid.GetUID();
    }
};

template <> struct quill::copy_loggable<ZDOID> : std::true_type {};
template <> struct fmt::formatter<ZDOID> : ostream_formatter {};
