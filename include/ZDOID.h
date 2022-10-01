#pragma once

#include <string>

struct ZDOID {
	explicit ZDOID();
	explicit ZDOID(int64_t userID, uint32_t id);

	std::string ToString();

	bool operator==(const ZDOID& other) const;
	bool operator!=(const ZDOID& other) const;

	// Return whether this has a value besides NONE
	explicit operator bool() const noexcept;

	static const ZDOID NONE;

	int64_t m_userID;
	uint32_t m_id;
	int32_t m_hash;
};
