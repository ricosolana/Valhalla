#pragma once

#include <cstdint>
#include <stdexcept>
#include "VUtils.h"
#include "BitPack.h"

//class BDOID {
//    friend class ZDO;
//    //uint16_t m_userIDIndex;
//    //uint16_t m_id;
//
//    USER_ID_t m_userID;
//    uint32_t m_id;
//    uint32_t m_unusedPadding; // garbage
//
//    static const ZDOID NONE;
//};

class ZDOID {
    friend class ZDO;

    //uint16_t m_userIDIndex;
    //uint16_t m_id;

    USER_ID_t m_userID{};
    uint32_t m_id{};
    uint32_t m_unusedPadding{}; // garbage

    //static std::array<USER_ID_t, std::numeric_limits<decltype(m_userIDIndex)>::max()> INDEXED_USERS;

public:
    static const ZDOID NONE;

private:
    // Get the index of a UserID
    //  The UserID is inserted if it does not exist
    //  Returns the insertion index or the existing index of the UserID
    //static size_t EnsureUserIDIndex(int64_t owner) {
    //    if (!owner)
    //        return 0;
    //
    //    for (size_t i = 1; i < INDEXED_USERS.size(); i++) {
    //        // Assume that a blank index prior to an existing UserID being found
    //        //  means that the UserID does not exist (so insert it)
    //        if (!INDEXED_USERS[i]) {
    //            INDEXED_USERS[i] = owner;
    //            return i;
    //        }
    //        else if (INDEXED_USERS[i] == owner) {
    //            return i;
    //        }
    //    }
    //
    //    // TODO this is definitely reachable, assuming the server runs long enough
    //    //  for enough unique players to join, causing the INDEXED_USERS loop to completely finish
    //    //  and reach this point
    //    std::unreachable();
    //}
    //
    //static int64_t GetUserIDByIndex(size_t index) {
    //    //if (!index)
    //        //return 0;
    //
    //    //assert((index || (INDEXED_USERS[index] == 0))
    //        //&& "Array[0] should be 0 to represent no-owner");
    //
    //    if (index < INDEXED_USERS.size())
    //        return INDEXED_USERS[index];
    //
    //    throw std::runtime_error("user id by index not found");
    //}

public:
    ZDOID() {}
    ZDOID(USER_ID_t owner, uint32_t uid);
    //ZDOID();
    //
    //ZDOID(const ZDOID& other); // should default
    //ZDOID(ZDOID&& other) noexcept;
    //
    //void operator=(const ZDOID& other);
    //void operator=(ZDOID&& other) noexcept;

    bool operator==(const ZDOID &other) const noexcept {
        //return this->m_pack == other.m_pack;
        return this->m_userID == other.m_userID 
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
    USER_ID_t GetOwner() const {
        //return INDEXED_USERS[_GetUserIDIndex()];
        return this->m_userID;
    }

    // Rename to SetUserID
    void SetOwner(USER_ID_t owner) {
        //_SetUserIDIndex(this->EnsureUserIDIndex((int64_t)owner));
        this->m_userID = owner;
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
    uint32_t GetUID() const {
        //return m_pack.Get<ID_PACK_INDEX>();
        return this->m_id;
    }

    // TODO rename to SetID
    void SetUID(uint32_t uid) {
        //m_pack.Set<ID_PACK_INDEX>(uid);
        this->m_id = uid;
    }



    friend std::ostream& operator<<(std::ostream& st, ZDOID zdoid) {
        return st << (int64_t)zdoid.GetOwner() << ":" << zdoid.GetUID();
    }
};

template <> struct quill::copy_loggable<ZDOID> : std::true_type {};
template <> struct fmt::formatter<ZDOID> : ostream_formatter {};
