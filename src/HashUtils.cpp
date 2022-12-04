#include "HashUtils.h"
#include "NetID.h"
#include "Vector.h"

/*
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
		return robin_hood::hash_int((value.m_uuid ^ value.m_id) 
			+ (value.m_uuid * value.m_id));

		//return robin_hood::hash_int(value.m_uuid) 
			//^ robin_hood::hash_int(value.m_id);

		//return value.m_hash;
		//return Hasher{}(value.m_uuid) ^ Hasher{}(value.m_id);
	}

	int Hasher::operator()(const Vector2i& value) const {
		robin_hood::hash
		return robin_hood::hash_int((value.x ^ value.y) 
			^ (value.x * value.y));

		//return value.x ^ value.y;
	}

	// then given an int hash, convert it to size_t

}
*/