#pragma once

#include <cstdint>
#include <stdexcept>
#include "VUtils.h"

class ZDOID {
    friend struct ankerl::unordered_dense::hash<ZDOID>;
    friend class ZDO;

#if INTPTR_MAX == INT32_MAX
    // 2 players per session
    // 32,768 ZDOs
    using T = uint16_t;
    static constexpr int USER_BIT_COUNT = 1;
#elif INTPTR_MAX == INT64_MAX
    // 64 players per session
    // ~67.1M ZDOs
    using T = uint32_t;
    static constexpr int USER_BIT_COUNT = 6;
#else
#error "Unable to detect 32-bit or 64-bit system"
#endif
    static constexpr int USER_BIT_OFFSET = 0;
    static constexpr int ID_BIT_OFFSET = USER_BIT_COUNT;

    static constexpr int ID_BIT_COUNT = 8 * sizeof(T) - ID_BIT_OFFSET;
    
    static constexpr T USER_BIT_MASK = (1 << (USER_BIT_COUNT)) - 1;
    static constexpr T ID_BIT_MASK = (~USER_BIT_MASK) >> USER_BIT_COUNT;

    // Indexed UserIDs
    //  Capacity is equal to USER mask due to a ZDOID USER index of 0 referring to no active owner
    static std::array<int64_t, USER_BIT_MASK> INDEXED_USERS;
    
public:
    static const ZDOID NONE;

private:
    // Encoding of ID and USER bits
    //  iiiiiiii iiiiiiii iiiiiiii iiuuuuuu
    //  i: ID, u: USER
    T m_encoded{};

private:
    // Get the index of a UserID
    //  The UserID is inserted if it does not exist
    //  Returns the insertion index or the existing index of the UserID
    static T EnsureUserIDIndex(int64_t owner) {
        if (!owner)
            return 0;

        for (T i = 1; i < INDEXED_USERS.size(); i++) {
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

    static int64_t GetUserIDByIndex(T index) {
        if (!index)
            return 0;

        //assert((index || (INDEXED_USERS[index] == 0))
            //&& "Array[0] should be 0 to represent no-owner");

        if (INDEXED_USERS.size() < index)
            return INDEXED_USERS[index];
        throw std::runtime_error("user id by index not found");
    }

public:
    constexpr ZDOID() = default;

    constexpr ZDOID(OWNER_t owner, uint32_t uid) {
        this->SetOwner(owner);
        this->SetUID(uid);
    }

    constexpr ZDOID(const ZDOID&) = default;

    bool operator==(ZDOID other) const {
        return this->m_encoded == other.m_encoded;
    }

    bool operator!=(ZDOID other) const {
        return !(*this == other);
    }
    
    // Return whether this has a value besides NONE
    explicit operator bool() const noexcept {
        return static_cast<bool>(this->m_encoded);
    }

    // TODO rename to User
    OWNER_t GetOwner() const {
        return INDEXED_USERS[_GetUserIDIndex()];
    }

    // Rename to SetUserID
    constexpr void SetOwner(OWNER_t owner) {
        _SetUserIDIndex(this->EnsureUserIDIndex((int64_t)owner));
    }



    // Retrieve the index of the UserID
    constexpr T _GetUserIDIndex() const {
        return (this->m_encoded >> USER_BIT_OFFSET) & USER_BIT_MASK;
    }

    // Set the associated UserID index 
    constexpr void _SetUserIDIndex(T index) {
        this->m_encoded &= (index & USER_BIT_MASK) << USER_BIT_OFFSET;
    }

    // TODO rename to GetID
    constexpr uint32_t GetUID() const {
        return (this->m_encoded >> ID_BIT_OFFSET) & ID_BIT_MASK;
    }

    // TODO rename to SetID
    constexpr void SetUID(uint32_t uid) {
        this->m_encoded &= (uid & ID_BIT_MASK) << ID_BIT_OFFSET;
    }



    friend std::ostream& operator<<(std::ostream& st, ZDOID zdoid) {
        return st << (int64_t)zdoid.GetOwner() << ":" << zdoid.GetUID();
    }
};

template <> struct quill::copy_loggable<ZDOID> : std::true_type {};
template <> struct fmt::formatter<ZDOID> : ostream_formatter {};
