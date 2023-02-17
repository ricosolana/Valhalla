#pragma once

#include "../Hashes.h"
#include "../ZDO.h"

class Portal {
public:
	ZDO* m_zdo;
	Piece(ZDO* zdo) : m_zdo(zdo) {}

    std::string GetTag() {
		return m_zdo->GetString(Hashes::ZDO::TeleportWorld::TAG, "");
    }

	ZDOID GetTarget() {
		return m_zdo->GetZDOID(Hashes::ZDO::TeleportWorld::TARGET);
	}

	ZDOID GetAuthor() {
		return m_zdo->GetString(Hashes::ZDO::TeleportWorld::AUTHOR, "");
	}
};
