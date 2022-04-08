#include "ZDOID.hpp"
#include "HashUtils.hpp"

const ZDOID ZDOID::NONE(0, 0);

ZDOID::ZDOID()
	: ZDOID(NONE)
{}

ZDOID::ZDOID(int64_t userID, uint32_t id)
	: m_userID(userID), m_id(id) , m_hash(HashUtils::Hasher{}(m_userID) ^ HashUtils::Hasher{}(m_id))
{}

std::string ZDOID::ToString() {
	return std::to_string(m_userID) + ":" + std::to_string(m_id);
}

bool ZDOID::operator==(const ZDOID& other) const {
	return m_userID == other.m_userID
		&& m_id == other.m_id;
}

bool ZDOID::operator!=(const ZDOID& other) const {
	return !(*this == other);
}

ZDOID::operator bool() const noexcept {
	return *this != NONE;
}