#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

// TODO use inline static within class
//decltype(ZDOID::INDEXED_USERS) ZDOID::INDEXED_USERS;

ZDOID::ZDOID(USER_ID_t owner, uint32_t uid) {
    this->SetOwner(owner);
    this->SetUID(uid);
    this->m_unusedPadding = 0;
}

ZDOID::ZDOID() 
    : ZDOID(0, 0) {}

ZDOID::ZDOID(const ZDOID& other) {
    *this = other;
}

ZDOID::ZDOID(ZDOID&& other) noexcept {
    *this = std::move(other);
}

    //
    // : m_userID(other.m_userID), m_id(other.m_id), m_unusedPadding(0) {}

void ZDOID::operator=(const ZDOID& other) {
    this->m_userID = other.m_userID;
    this->m_id = other.m_id;
    this->m_unusedPadding = 0;
}

void ZDOID::operator=(ZDOID&& other) noexcept {
    this->m_userID = other.m_userID;
    this->m_id = other.m_id;
    this->m_unusedPadding = 0;

    other.m_userID = 0;
    other.m_id = 0;
}
