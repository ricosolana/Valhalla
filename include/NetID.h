#pragma once

#include "VUtils.h"

struct NetID {
    // TODO make this structure more efficient, currently padding takes up 1/4 of the size

    OWNER_t m_uuid;
    uint32_t m_id;

    explicit NetID();
    explicit NetID(OWNER_t userID, uint32_t id);

    bool operator==(const NetID& other) const;
    bool operator!=(const NetID& other) const;

    // Return whether this has a value besides NONE
    explicit operator bool() const noexcept;

    static const NetID NONE;
};

using ZDOID = NetID;
