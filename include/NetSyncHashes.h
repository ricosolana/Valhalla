#pragma once

#include "Utils.h"

static constexpr hash_t __H(const std::string& key) {
	return Utils::GetStableHashCode(key);
}

enum class Sync_ArmorStand : hash_t {
	POSE = __H("pose"),
};

enum class 
