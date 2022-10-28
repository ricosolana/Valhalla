#pragma once

#include "Utils.h"

struct NetID {
	// TODO make these private and use getters; 
	//	could make this a friend class of the Hasher object
	OWNER_t m_uuid;
	uint32_t m_id;

	explicit NetID();
	explicit NetID(int64_t userID, uint32_t id);

	std::string ToString();

	bool operator==(const NetID& other) const;
	bool operator!=(const NetID& other) const;

	// Return whether this has a value besides NONE
	explicit operator bool() const noexcept;

	static const NetID NONE;
};

//typedef NetID ZDOID;
