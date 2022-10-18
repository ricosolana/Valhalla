#pragma once

#include "Utils.h"

class NetSync {
public:
	struct ID {
		explicit ID();
		explicit ID(int64_t userID, uint32_t id);

		std::string ToString();

		bool operator==(const ID& other) const;
		bool operator!=(const ID& other) const;

		// Return whether this has a value besides NONE
		explicit operator bool() const noexcept;

		static const ID NONE;

		uuid_t m_userID;
		uint32_t m_id;
		hash_t m_hash;
	};
}