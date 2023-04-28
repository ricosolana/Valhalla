#pragma once

#include "VUtils.h"

class ZDOID {
    static constexpr uint64_t ENCODED_OWNER_MASK =  0b1000000000000000000000000000000011111111111111111111111111111111ULL;

public:
    uint64_t m_encoded {};

public:
    constexpr ZDOID() = default;

    constexpr ZDOID(OWNER_t owner, uint32_t uid) {
        this->SetOwner(owner);
        this->SetUID(uid);
    }

    bool operator==(const ZDOID& other) const {
        return this->m_encoded == other.m_encoded;
    }

    bool operator!=(const ZDOID& other) const {
        return !(*this == other);
    }

    // Return whether this has a value besides NONE
    explicit operator bool() const noexcept {
        return static_cast<bool>(this->m_encoded);
    }

    constexpr OWNER_t GetOwner() const {
        if (std::make_signed_t<decltype(this->m_encoded)>(this->m_encoded) < 0)
            return static_cast<OWNER_t>((this->m_encoded & ENCODED_OWNER_MASK) | ~ENCODED_OWNER_MASK);
        return static_cast<OWNER_t>(this->m_encoded & ENCODED_OWNER_MASK);
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

    static const ZDOID NONE;
};

std::ostream& operator<<(std::ostream& st, ZDOID& zdoid);
