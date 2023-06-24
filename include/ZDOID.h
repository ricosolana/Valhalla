#pragma once

#include <cstdint>
#include <stdexcept>
#include <bitset>
#include "VUtils.h"
#include "BitPack.h"

//VH_PACKED(
#pragma pack(push, 1)
class ZDOID {
    friend struct ankerl::unordered_dense::hash<ZDOID>;
    friend class ZDO;

    // 62 players per session (2 total reserved for null/server)
    // ~67.1M ZDOs
    using UType = uint32_t;

    // User: 0, ID: 1
    //BitPack<UType, VH_ZDOID_USER_BITS_I_, sizeof(UType) * 8 - VH_ZDOID_USER_BITS_I_> m_pack;
      
    uint32_t m_uid{};
    uint16_t m_userIDIndex{};

    // Indexed UserIDs
    static inline std::array<int64_t, std::numeric_limits<decltype(m_userIDIndex)>::max()> INDEXED_USERS;

    static inline decltype(m_userIDIndex) NEXT_EMPTY_INDEX = 1;
    static inline decltype(m_userIDIndex) LAST_SET_INDEX = 1;

    static inline constexpr auto USER_PACK_INDEX = 0;
    static inline constexpr auto ID_PACK_INDEX = 1;

public:
    static const ZDOID NONE;

private:
    static decltype(m_userIDIndex) GetOrCreateIndexByUserID(int64_t userID) {
        if (userID == 0)
            return 0;

        // Try searching for a preexisting userID
        for (decltype(LAST_SET_INDEX) i = 1; i <= LAST_SET_INDEX; i++) {
            if (INDEXED_USERS[i] == userID) {
                return i;
            }
        }

        // Set the userID in the next available slot
        assert(INDEXED_USERS[NEXT_EMPTY_INDEX] == 0);
        const auto index = NEXT_EMPTY_INDEX;
        INDEXED_USERS[NEXT_EMPTY_INDEX] = userID;

        LAST_SET_INDEX = std::max(LAST_SET_INDEX, NEXT_EMPTY_INDEX);

        NEXT_EMPTY_INDEX++;

        // Find the next empty slot starting from the previous empty
        for (auto i = NEXT_EMPTY_INDEX; i <= LAST_SET_INDEX + 1; i++) {
            if (INDEXED_USERS[i] == 0) {
                NEXT_EMPTY_INDEX = i;
                break;
            }
        }

        return index;
    }

    static void RemoveIndexByUserID(int64_t userID) {
        if (userID == 0)
            return;

        // Try searching for a preexisting userID
        for (decltype(LAST_SET_INDEX) i = 1; i <= LAST_SET_INDEX; i++) {
            if (INDEXED_USERS[i] == userID) {
                INDEXED_USERS[i] = 0;
                NEXT_EMPTY_INDEX = std::min(i, NEXT_EMPTY_INDEX);
                break;
            }
        }

        // Try setting the last index with a value
        for (auto i = LAST_SET_INDEX; i > 0; i--) {
            if (INDEXED_USERS[i] != 0) {
                LAST_SET_INDEX = i;
                break;
            }
        }
    }

    static int64_t GetUserIDByIndex(uint16_t index) {
        if (index < INDEXED_USERS.size())
            return INDEXED_USERS[index];

        throw std::runtime_error("userID index exceeds capacity");
    }

    /*
    // Get the index of a UserID
    //  The UserID is inserted if it does not exist
    //  Returns the insertion index or the existing index of the UserID
    static uint16_t EnsureUserIDIndex(int64_t owner) {
        if (!owner)
            return 0;

        // 2 cases here
        //  find either the value by index
        //  assign the new index
        for (uint16_t i = 1; i < LAST_EMPTY_INDEX - 1; i++) {
            if (GetUserIDByIndex(i) == owner) {
                return i;
            }
        }

        LOWER_USER_BYTES[NEXT_EMPTY_INDEX] = static_cast<uint32_t>(owner);
        USER_SIGN[NEXT_EMPTY_INDEX] = owner < 0;

        for (uint16_t i = NEXT_EMPTY_INDEX + 1; i < LAST_EMPTY_INDEX; i++) {
            if (GetUserIDByIndex(i) == 0) {
                NEXT_EMPTY_INDEX = i;
                break;
            }
        }

        throw std::runtime_error("user bit cap exceeded");
    }

    static void RemoveUserIDByIndex(uint16_t index) {
        if (index >= LOWER_USER_BYTES.size())
            throw std::runtime_error("user index exceeded");

        if (index == 0) {
            return;
        }

        NEXT_EMPTY_INDEX = std::min(NEXT_EMPTY_INDEX, index);

        if (index == LAST_EMPTY_INDEX - 1)
            LAST_EMPTY_INDEX = index;

        // Strict Constraint
        LOWER_USER_BYTES[index] = 0;
        USER_SIGN[index] = 0;
    }

    static int64_t GetUserIDByIndex(size_t index) {
        if (index < LOWER_USER_BYTES.size())
            return (USER_SIGN[index] ? -1 : 1) * static_cast<int64_t>(LOWER_USER_BYTES[index]);

        throw std::runtime_error("user id by index exceeded");
    }*/

public:
    ZDOID() = default;

    ZDOID(OWNER_t owner, uint32_t uid) {
        this->SetOwner(owner);
        this->SetUID(uid);
    }

    ZDOID(const ZDOID&) = default;

    bool operator==(ZDOID other) const {
        //return this->m_pack == other.m_pack;
        return this->m_uid == other.m_uid 
            && this->m_userIDIndex == other.m_userIDIndex;
    }

    bool operator!=(ZDOID other) const {
        return !(*this == other);
    }
    
    // Return whether this has a value besides NONE
    operator bool() const noexcept {
        return m_uid;
    }

    // TODO rename to User
    OWNER_t GetOwner() const {
        return INDEXED_USERS[_GetUserIDIndex()];
    }

    // Rename to SetUserID
    void SetOwner(OWNER_t owner) {
        //_SetUserIDIndex(this->EnsureUserIDIndex((int64_t)owner));
        _SetUserIDIndex(GetOrCreateIndexByUserID(owner));
    }



    // Retrieve the index of the UserID
    uint32_t _GetUserIDIndex() const {
        return m_userIDIndex;
    }

    // Set the associated UserID index 
    void _SetUserIDIndex(decltype(m_userIDIndex) index) {
        this->m_userIDIndex = index;
    }

    // TODO rename to GetID
    uint32_t GetUID() const {
        return this->m_uid;
    }

    // TODO rename to SetID
    void SetUID(uint32_t uid) {
        this->m_uid = uid;
    }



    friend std::ostream& operator<<(std::ostream& st, ZDOID zdoid) {
        return st << (int64_t)zdoid.GetOwner() << ":" << zdoid.GetUID();
    }
};//);
#pragma pack(pop)

template <> struct quill::copy_loggable<ZDOID> : std::true_type {};
template <> struct fmt::formatter<ZDOID> : ostream_formatter {};
