#pragma once

#include "../Hashes.h"
#include "../ZDO.h"

class Player {
public:
	ZDO* m_zdo;
	Player(ZDO* zdo) : m_zdo(zdo) {}

	PLAYER_ID_t GetID() {
		return m_zdo->GetLong(Hashes::ZDO::Player::PLAYER_ID, 0);
	}

	std::string GetName() {
		return m_zdo->GetString(Hashes::ZDO::Player::PLAYER_NAME, "");
	}

};
