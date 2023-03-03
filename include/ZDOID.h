#pragma once

#include "VUtils.h"

// TODO rename back to ZDOID
class ZDOID {
public:
    // TODO make this structure more efficient, currently padding takes up 1/4 of the size

    OWNER_t m_uuid = 0;
    uint32_t m_id = 0;

    constexpr ZDOID() = default;

    constexpr ZDOID(OWNER_t userID, uint32_t id)
        : m_uuid(userID), m_id(id) {}

    bool operator==(const ZDOID& other) const {
        return this->m_uuid == other.m_uuid
            && this->m_id == other.m_id;
    }

    bool operator!=(const ZDOID& other) const {
        return !(*this == other);
    }

    // Return whether this has a value besides NONE
    explicit operator bool() const noexcept {
        return *this != NONE;
    }

    static const ZDOID NONE;
};

std::ostream& operator<<(std::ostream& st, ZDOID& zdoid);
