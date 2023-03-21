#pragma once

#include <robin_hood.h>

#include "Hashes.h"
#include "Peer.h"
#include "Piece.h"
#include "Player.h"

// prefab: guard_stone
// component: PrivateArea
class Ward {
public:
	std::reference_wrapper<ZDO> m_zdo;
	Ward(ZDO& zdo) : m_zdo(zdo) {
		if (zdo.GetPrefab()->m_hash != Hashes::Object::guard_stone)
			throw std::runtime_error("not a ward");
	}

	const std::string& GetCreatorName() {
		return m_zdo.get().GetString(Hashes::ZDO::PrivateArea::CREATOR, "");
	}

	void SetCreatorName(const std::string& name) {
		m_zdo.get().Set(Hashes::ZDO::PrivateArea::CREATOR, name);
	}
	
	// So insecure, since any player can change their player id to act as another players identity
	// TODO use steam-id instead 
	robin_hood::unordered_map<PLAYER_ID_t, std::string> GetPermitted() {
		robin_hood::unordered_map<PLAYER_ID_t, std::string> list;

		auto count = m_zdo.get().GetInt(Hashes::ZDO::PrivateArea::PERMITTED_COUNT, 0);
		for (int i = 0; i < count; i++) {
			auto id = m_zdo.get().GetLong("pu_id" + std::to_string(i), 0);
			auto name = m_zdo.get().GetString("pu_name" + std::to_string(i), "");
			if (id) {
				list[id] = name;
			}
		}

		return list;
	}

	void SetPermitted(const robin_hood::unordered_map<PLAYER_ID_t, std::string> &users) {
		m_zdo.get().Set(Hashes::ZDO::PrivateArea::PERMITTED_COUNT, static_cast<int32_t>(users.size()));
		int i = 0;
		for (auto&& pair : users) {
			m_zdo.get().Set("pu_id" + std::to_string(i), pair.first);
			m_zdo.get().Set("pu_name" + std::to_string(i), pair.second);
			i++;
		}
	}

	void AddPermitted(PLAYER_ID_t id, const std::string& name) {
		auto map = GetPermitted();
		map[id] = name;
		SetPermitted(map);
	}

	void RemovePermitted(PLAYER_ID_t id) {
		auto map = GetPermitted();
		map.erase(id);
		SetPermitted(map);
	}

	void SetEnabled(bool enable) {
		m_zdo.get().Set(Hashes::ZDO::PrivateArea::ENABLED, enable);
	}

	bool IsEnabled() {
		return m_zdo.get().GetBool(Hashes::ZDO::PrivateArea::ENABLED, false);
	}

	bool IsPermitted(PLAYER_ID_t id) {
		return GetPermitted().contains(id);
	}

	void SetCreator(Peer& peer) {
		auto&& zdo = peer.GetZDO();
		if (zdo) {
			// Create the copy
			auto&& newZdo = ZDOManager()->Instantiate(m_zdo);

			// Destroy the old ZDO
			ZDOManager()->DestroyZDO(m_zdo, true);

			// Reassign
			m_zdo = newZdo;

			SetCreatorName(peer.m_name);
			Piece(m_zdo).SetCreator(Player(*zdo).GetID());
		}
	}

	bool IsAllowed(PLAYER_ID_t id) {
		return Piece(m_zdo).GetCreator() == id || IsPermitted(id);
	}
};