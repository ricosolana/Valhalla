#pragma once

#include "Utils.h"

struct NetID {
	explicit NetID();
	explicit NetID(int64_t userID, uint32_t id);

	std::string ToString();

	bool operator==(const NetID& other) const;
	bool operator!=(const NetID& other) const;

	// Return whether this has a value besides NONE
	explicit operator bool() const noexcept;

	static const NetID NONE;

	uuid_t m_userID;
	uint32_t m_id;
	hash_t m_hash;
};