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

	const int64_t m_userID;
	const uint32_t m_id;
	const int32_t m_hash;
};
