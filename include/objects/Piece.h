#pragma once

#include "../Hashes.h"
#include "../ZDO.h"

class Piece {
public:
    ZDO* m_zdo;
    Piece(ZDO* zdo) : m_zdo(zdo) {}

	std::string GetCreator() {
		return m_zdo->GetString(Hashes::ZDO::Piece::CREATOR, "");
	}

	void SetCreator(const std::string& name) {
		m_zdo->Set(Hashes::ZDO::Piece::CREATOR, name);
	}
};
