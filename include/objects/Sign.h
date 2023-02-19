#pragma once

#include <robin_hood.h>

#include "Hashes.h"
#include "Peer.h"

class Sign {
public:
	ZDO &m_zdo;
	Sign(ZDO& zdo) : m_zdo(zdo) {
		if (zdo.GetPrefab()->m_hash != Hashes::Object::sign)
			throw std::runtime_error("not a ward");
	}

	void SetText(const std::string& text) {
		m_zdo.Set(Hashes::ZDO::Sign::TEXT, text);
	}

	std::string GetText() {
		//return m_zdo.GetString
	}

	void SetAuthor(const std::string& author) {
		m_zdo.Set(Hashes::ZDO::Sign::AUTHOR, author);
	}
};
