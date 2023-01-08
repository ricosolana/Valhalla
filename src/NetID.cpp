#include "NetID.h"

const NetID NetID::NONE(0, 0);

NetID::NetID()
	: NetID(NONE)
{}

NetID::NetID(int64_t userID, uint32_t id)
	: m_uuid(userID), m_id(id)
{}

std::string NetID::ToString() {
	return std::to_string(m_uuid) + ":" + std::to_string(m_id);
}

bool NetID::operator==(const NetID& other) const {
	return m_uuid == other.m_uuid
		&& m_id == other.m_id;
}

bool NetID::operator!=(const NetID& other) const {
	return !(*this == other);
}

NetID::operator bool() const noexcept {
	return *this != NONE;
}
