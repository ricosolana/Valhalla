#include "ZDOManager.h"

class ZDOCollector {
	//ranges::any_view<ZDO::unsafe_value> m_view;
	//std::shared_ptr<ranges::ref_view<ZDO::unsafe_value>> m_view;

	// Predicate for whether a zdo is a prefab with or without given flags
	//	prefabHash: if 0, then prefabHash check is skipped
	static bool prefab_filter(ZDO::unsafe_value zdo, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& prefab = zdo->GetPrefab();

		return prefab.AllFlagsAbsent(flagsAbsent)
			&& (prefabHash == 0 || prefab.m_hash == prefabHash)
			&& prefab.AllFlagsPresent(flagsPresent);
	}

public:
	using filter_t = const std::function<bool(ZDO::unsafe_value)>&;
	using consumer_t = const std::function<void(ZDO::unsafe_value)>&;

	//auto GetZDOs() {
	//	return ranges::views::all(m_objectsByID);
	//}

	[[nodiscard]] void SomeZDOs(Vector3f pos, float radius, size_t max, filter_t filter, consumer_t consumer) {
		float sqRadius = radius * radius;

		auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
		auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));

		for (auto z = minZone.y; z <= maxZone.y; z++) {
			for (auto x = minZone.x; x <= maxZone.x; x++) {
				if (auto&& container = ZDOManager()->_GetZDOContainer(ZoneID(x, z))) {
					for (auto&& v : *container) {
						auto&& zdo = ZDO::make_unsafe_value(v);
						if (zdo->GetPosition().SqDistance(pos) > sqRadius)
							continue;

						if (!filter || filter(zdo)) {
							consumer(zdo);
							if (max-- == 0) // underflow intentional and well defined
								return;
						}
					}
				}
			}
		}
	}

	// Get a capped number of ZDOs within a radius matching an optional predicate
	//[[nodiscard]] auto SomeZDOs(Vector3f pos, float radius) {
	//	float sqRadius = radius * radius;
	//
	//	auto minZone = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
	//	auto maxZone = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius));
	//
	//	for (auto z = minZone.y; z <= maxZone.y; z++) {
	//		for (auto x = minZone.x; x <= maxZone.x; x++) {
	//			if (auto&& container = ZDOManager()->_GetZDOContainer(ZoneID(x, z))) {
	//				m_view = ranges::concat_view(m_view, *container);
	//			}
	//		}
	//	}
	//
	//	// pred_t pred
	//	// && (!pred || pred(zdo));
	//	//ranges::views::take_fi
	//
	//	auto rng = m_view | ranges::views::filter([pos, sqRadius](const ZDO::unsafe_value zdo) {
	//		return zdo->GetPosition().SqDistance(pos) <= sqRadius;
	//	});
	//
	//	return this;
	//}
	//[[nodiscard]] auto SomeZDOs()

	// TODO finish this:
	// Get a capped number of ZDOs with prefab and/or flag
	//	*Note: Prefab or Flag must be non-zero for anything to be returned
	[[nodiscard]] auto SomeZDOs(Vector3f pos, float radius, size_t max, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {

		return SomeZDOs(pos, radius, max, std::bind_back(prefab_filter, prefab, flagsPresent, flagsAbsent));

		//[&](ZDO::unsafe_value zdo) {
		//	return PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
		//}
	}

	// Get a capped number of ZDOs within a zone matching an optional predicate
	[[nodiscard]] std::list<ZDO::unsafe_value> SomeZDOs(ZoneID zone, size_t max, pred_t pred);
	// Get a capped number of ZDOs within a zone
	[[nodiscard]] std::list<ZDO::unsafe_value> SomeZDOs(ZoneID zone, size_t max) {
		return SomeZDOs(zone, max, nullptr);
	}
	// Get a capped number of ZDOs within a radius in zone with prefab and/or flag
	[[nodiscard]] std::list<ZDO::unsafe_value> SomeZDOs(ZoneID zone, size_t max, Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		float sqRadius = radius * radius;
		return SomeZDOs(zone, max, [&](ZDO::unsafe_value zdo) {
			return zdo->GetPosition().SqDistance(pos) <= sqRadius
				&& PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
			});
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	[[nodiscard]] std::list<ZDO::unsafe_value> SomeZDOs(ZoneID zone, size_t max, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, max, Vector3f::Zero(), std::numeric_limits<float>::max(), prefab, flagsPresent, flagsAbsent);
	}
	// Get a capped number of ZDOs within a zone with prefab and/or flag
	[[nodiscard]] std::list<ZDO::unsafe_value> SomeZDOs(ZoneID zone, size_t max, Vector3f pos, float radius) {
		return SomeZDOs(zone, max, pos, radius, 0, Prefab::Flag::NONE, Prefab::Flag::NONE);
	}


	// Get all ZDOs with prefab
	//	This method is optimized assuming VH_STANDARD_PREFABS is on
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(HASH_t prefab);

	// Get all ZDOs fulfilling a given predicate
	//	Try to avoid using this method too frequently (it iterates all ZDOs in the world, which is *very* slow)
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(pred_t pred);

	// Get all ZDOs matching the given prefab flags
	//	Try to avoid using this method too frequently (it iterates all ZDOs in the world, which is *very* slow)
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return GetZDOs([&](ZDO::unsafe_value zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, 0, flagsPresent, flagsAbsent);
			});
	}

	// Get all ZDOs within a radius matching an optional predicate
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(Vector3f pos, float radius, pred_t pred) {
		return SomeZDOs(pos, radius, -1, pred);
	}
	// Get all ZDOs within a radius
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(Vector3f pos, float radius) {
		return SomeZDOs(pos, radius, -1, nullptr);
	}

	// Get all ZDOs within a radius with prefab and/or flag
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(pos, radius, -1, prefab, flagsPresent, flagsAbsent);
	}



	// Get all ZDOs within a zone matching an optional predicate
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(ZoneID zone, pred_t pred) {
		return SomeZDOs(zone, -1, pred);
	}
	// Get all ZDOs within a zone
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(ZoneID zone) {
		return SomeZDOs(zone, -1, nullptr);
	}
	// Get all ZDOs within a zone of prefab and/or flag
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(ZoneID zone, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return SomeZDOs(zone, -1, [&](ZDO::unsafe_value zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
			});
	}
	// Get all ZDOs within a radius in zone
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(ZoneID zone, Vector3f pos, float radius, HASH_t prefab, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		const auto sqRadius = radius * radius;
		return SomeZDOs(zone, -1, [&](ZDO::unsafe_value zdo) {
			return zdo->GetPosition().SqDistance(pos) <= sqRadius
				&& PREFAB_CHECK_FUNCTION(zdo, prefab, flagsPresent, flagsAbsent);
			});
	}
	// Get all ZDOs within a radius in zone
	[[nodiscard]] std::list<ZDO::unsafe_value> GetZDOs(ZoneID zone, Vector3f pos, float radius) {
		return GetZDOs(zone, pos, radius, 0, Prefab::Flag::NONE, Prefab::Flag::NONE);
	}


	// Get any ZDO within a radius with prefab and/or flag
	[[nodiscard]] ZDO::unsafe_optional AnyZDO(Vector3f pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(pos, radius, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return ZDO::unsafe_nullopt;
		return zdos.front();
	}
	// Get any ZDO within a zone with prefab and/or flag
	[[nodiscard]] ZDO::unsafe_optional AnyZDO(ZoneID zone, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& zdos = SomeZDOs(zone, 1, prefabHash, flagsPresent, flagsAbsent);
		if (zdos.empty())
			return ZDO::unsafe_nullopt;
		return zdos.front();
	}



	// Get the nearest ZDO within a radius matching an optional predicate
	// TODO this is not best-optimized
	[[nodiscard]] ZDO::unsafe_optional NearestZDO(Vector3f pos, float radius, pred_t pred);

	// Get the nearest ZDO within a radius with prefab and/or flag
	// TODO this is not best-optimized
	[[nodiscard]] ZDO::unsafe_optional NearestZDO(Vector3f pos, float radius, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		return NearestZDO(pos, radius, [&](ZDO::unsafe_value zdo) {
			return PREFAB_CHECK_FUNCTION(zdo, prefabHash, flagsPresent, flagsAbsent);
			});
	}

};