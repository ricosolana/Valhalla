#pragma once

#include <cstdint>
#include <stdexcept>
#include "VUtils.h"

class ZDOID {
    friend struct ankerl::unordered_dense::hash<ZDOID>;

#if INTPTR_MAX == INT32_MAX
    // 2 players per session
    // 32,768 ZDOs
    static constexpr int LEADING_ID_BITS = 15;
    using T = uint16_t;
#elif INTPTR_MAX == INT64_MAX
    // 16 players per session
    // ~268 million ZDOs
    static constexpr int LEADING_ID_BITS = 28;
    using T = uint32_t;
#else
#error "Unable to detect 32-bit or 64-bit system"
#endif

    //static ankerl::unordered_dense::segmented_vector<OWNER_t> INDEXED_UIDS;
    static std::array<int64_t, (1 << (sizeof(T) * 8 - LEADING_ID_BITS)) - 1> INDEXED_USERS;
    
public:
    static const ZDOID NONE;

private:
    T m_encoded;

private:
    uint16_t GetIndex(int64_t owner) {
        if (!owner)
            return 0;

        for (int i = 1; i < INDEXED_USERS.size(); i++) {
            if (!INDEXED_USERS[i]) {
                INDEXED_USERS[i] = owner;
                return i;
            }
            else if (INDEXED_USERS[i] == owner) {
                return i;
            }
        }

        std::unreachable();
    }

public:
    constexpr ZDOID() = default;

    constexpr ZDOID(OWNER_t owner, uint32_t uid) {
        this->SetOwner(owner);
        this->SetUID(uid);
    }

    constexpr ZDOID(const ZDOID& other) = default;

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

    OWNER_t GetOwner() const {
        return INDEXED_USERS[m_encoded >> LEADING_ID_BITS];
    }

    constexpr void SetOwner(OWNER_t owner) {


        // Set owner bits to 0
        this->m_encoded &= ~ENCODED_OWNER_MASK;
        //assert(GetOwner() == 0);


        // Set the owner bits
        //  ignore the 2's complement middle bytes
        this->m_encoded |= (static_cast<uint64_t>(owner) & ENCODED_OWNER_MASK);

        //assert(GetOwner() == owner);
    }

    constexpr uint32_t GetUID() const {
        return static_cast<uint32_t>((this->m_encoded >> 32) & 0x7FFFFFFF);
    }

    constexpr void SetUID(uint32_t uid) {
        // Set uid bits to 0
        this->m_encoded &= ENCODED_OWNER_MASK;
        //assert(this->GetUID() == 0);

        // Set the uid bits
        this->m_encoded |= (static_cast<uint64_t>(uid) << (8 * 4)) & ~ENCODED_OWNER_MASK;
        //assert(this->GetUID() == uid);
    }

    friend std::ostream& operator<<(std::ostream& st, const ZDOID& zdoid) {
        return st << zdoid.GetOwner() << ":" << zdoid.GetUID();
    }
};

template <> struct quill::copy_loggable<ZDOID> : std::true_type {};
template <> struct fmt::formatter<ZDOID> : ostream_formatter {};
