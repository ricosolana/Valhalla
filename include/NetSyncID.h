#pragma once

#include <string>

struct NetSyncID {
	explicit NetSyncID();
	explicit NetSyncID(int64_t userID, uint32_t id);

	std::string ToString();

	bool operator==(const NetSyncID& other) const;
	bool operator!=(const NetSyncID& other) const;

	// Return whether this has a value besides NONE
	explicit operator bool() const noexcept;

	static const NetSyncID NONE;

	int64_t m_userID;
	uint32_t m_id;
	int32_t m_hash;
};
