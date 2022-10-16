#include "NetSyncID.h"
#include "HashUtils.h"

const NetSyncID NetSyncID::NONE(0, 0);

NetSyncID::NetSyncID()
	: NetSyncID(NONE)
{}

NetSyncID::NetSyncID(int64_t userID, uint32_t id)
	: m_userID(userID), m_id(id) , m_hash(HashUtils::Hasher{}(m_userID) ^ HashUtils::Hasher{}(m_id))
{}

std::string NetSyncID::ToString() {
	return std::to_string(m_userID) + ":" + std::to_string(m_id);
}

bool NetSyncID::operator==(const NetSyncID& other) const {
	return m_userID == other.m_userID
		&& m_id == other.m_id;
}

bool NetSyncID::operator!=(const NetSyncID& other) const {
	return !(*this == other);
}

NetSyncID::operator bool() const noexcept {
	return *this != NONE;
}