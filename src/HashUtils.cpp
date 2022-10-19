#include "HashUtils.h"
#include "NetID.h"

namespace HashUtils {
	int Hasher::operator()(const int64_t& value) const {
		return (int32_t)value ^ ((int32_t)(value >> 32));
	}

	int Hasher::operator()(const float& value) const {
		if (value == 0.f) return 0;
		return *(int32_t*)(&value);
	}

	int Hasher::operator()(const double &value) const {
		if (value == 0.0) return 0;

		int64_t num2 = *(int64_t*)(&value);
		return (int32_t)num2 ^ (int32_t)(num2 >> 32);
	}

	int Hasher::operator()(const NetID& value) const {
		return value.m_hash;
	}

	// then given an int hash, convert it to size_t

}
