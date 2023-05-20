#pragma once

#include <cstdint>
#include <stdexcept>
#include "VUtils.h"

class ZDOID {
#if INTPTR_MAX == INT32_MAX
    // Only 2 players are allowed on a server according to this spec
    //static constexpr uint16_t ENCODED_ID_MASK = 0b0111111111111111;
    static constexpr int LEADING_ID_BITS = 15;
    using T = uint16_t;
#elif INTPTR_MAX == INT64_MAX
    //static constexpr uint32_t ENCODED_ID_MASK = 0b00000011111111111111111111111111;
    static constexpr int LEADING_ID_BITS = 28;
    using T = uint32_t;
#else
#error "Environment not 32 or 64-bit."
#endif

    //static ankerl::unordered_dense::segmented_vector<OWNER_t> INDEXED_UIDS;
    static std::array<int64_t, (1 << (sizeof(T) * 8 - LEADING_ID_BITS)) - 1> INDEXED_IDS;
    
public:
    static const ZDOID NONE;

private:
    //std::remove_cvref_t<decltype(ENCODED_ID_MASK)> m_encoded;

    T m_encoded;

private:
    uint16_t GetIndex(int64_t owner) {
        if (!owner)
            return 0;

        for (int i = 1; i < INDEXED_IDS.size(); i++) {
            if (!INDEXED_IDS[i]) {
                INDEXED_IDS[i] = owner;
                return i;
            }
            else if (INDEXED_IDS[i] == owner) {
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
        re
        //return INDEXED_UIDS[m_encoded >> LEADING_ID_BITS];
    }

    constexpr void SetOwner(OWNER_t owner) {
        

        if (!(owner >= -2147483647LL && owner <= 4294967293LL)) {
            // Ensure filler complement bits are all the same (full negative or full positive)
            //if ((owner < 0 && (static_cast<uint64_t>(owner) & ~ENCODED_OWNER_MASK) != ~ENCODED_OWNER_MASK)
                //|| (owner >= 0 && (static_cast<uint64_t>(owner) & ~ENCODED_OWNER_MASK) == ~ENCODED_OWNER_MASK))
            throw std::runtime_error("OWNER_t unexpected encoding (client Utils.GenerateUID() differs?)");
        }

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
