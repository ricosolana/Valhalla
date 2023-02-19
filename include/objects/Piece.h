#pragma once

#include "../Hashes.h"
#include "../ZDO.h"

class Piece {
public:
    ZDO* m_zdo;
    Piece(ZDO* zdo) : m_zdo(zdo) {}

	PLAYER_ID_t GetCreator() {
		return m_zdo->GetLong(Hashes::ZDO::Piece::CREATOR, 0);
	}

	void SetCreator(PLAYER_ID_t id) {
		m_zdo->Set(Hashes::ZDO::Piece::CREATOR, id);
	}
};
