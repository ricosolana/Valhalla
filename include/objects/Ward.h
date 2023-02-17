#pragma once

#include <robin_hood.h>

#include "Hashes.h"

// prefab: guard_stone
// component: PrivateArea
class Ward {
public:
	ZDO* m_zdo;

	Ward(ZDO* zdo) : m_zdo(zdo) {
		//if (m_zdo->GetPrefab()) // do a test to make sure this object has component PrivateArea
		// or is ward-like 
	}

	const std::string& GetCreatorName() {
		return m_zdo->GetString(Hashes::ZDO::PrivateArea::CREATOR, "");
	}

	void SetCreatorName(const std::string& name) {
		m_zdo->Set(Hashes::ZDO::PrivateArea::CREATOR, name);
	}
	
	// So insecure, since any player can change their player id to act as another players identity
	// TODO use steam-id instead 
	robin_hood::unordered_map<OWNER_t, std::string> GetPermitted() {
		robin_hood::unordered_map<OWNER_t, std::string> list;

		auto count = m_zdo->GetInt(Hashes::ZDO::PrivateArea::PERMITTED_COUNT, 0);
		for (int i = 0; i < count; i++) {
			auto id = m_zdo->GetLong("pu_id" + std::to_string(i), 0);
			auto name = m_zdo->GetString("pu_name" + std::to_string(i), "");
			if (id) {
				list[id] = name;
			}
		}

		return list;
	}

	void SetPermitted(const robin_hood::unordered_map<OWNER_t, std::string> &users) {
		m_zdo->Set(Hashes::ZDO::PrivateArea::PERMITTED_COUNT, static_cast<int32_t>(users.size()));
		int i = 0;
		for (auto&& pair : users) {
			m_zdo->Set("pu_id" + std::to_string(i), pair.first);
			m_zdo->Set("pu_name" + std::to_string(i), pair.second);
			i++;
		}
	}

	void AddPermitted(OWNER_t id, const std::string& name) {
		auto map = GetPermitted();
		map[id] = name;
		SetPermitted(map);
	}

	void RemovePermitted(OWNER_t id) {
		auto map = GetPermitted();
		map.erase(id);
		SetPermitted(map);
	}

	void SetEnabled(bool enable) {
		m_zdo->Set(Hashes::ZDO::PrivateArea::ENABLED, enable);
	}

	bool IsEnabled() {
		return m_zdo->GetBool(Hashes::ZDO::PrivateArea::ENABLED, false);
	}

	bool IsPermitted(OWNER_t id) {
		return GetPermitted().contains(id);
	}

};