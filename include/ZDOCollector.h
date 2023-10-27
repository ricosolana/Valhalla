#include "ZDOManager.h"

// Easy to use ZDO Stream class inspired by c++ Ranges/v3 and/or Java Streams
// This is meant to be fast and have methods similar to streams, such as filter, limit, max, first...
class ZDOCollector {
	// Predicate for whether a zdo is a prefab with or without given flags
	//	prefabHash: if 0, then prefabHash check is skipped
	static bool prefab_filter(ZDO::unsafe_value zdo, HASH_t prefabHash, Prefab::Flag flagsPresent, Prefab::Flag flagsAbsent) {
		auto&& prefab = zdo->GetPrefab();

		return prefab.AllFlagsAbsent(flagsAbsent)
			&& (prefabHash == 0 || prefab.m_hash == prefabHash)
			&& prefab.AllFlagsPresent(flagsPresent);
	}

private:
	// Runs a singular consumer
	// If true, exit consumer/iteration
	bool _RunConsumer(ZDO::unsafe_value zdo) {
		if (m_filter(zdo)) {
			m_consumer(zdo);
			if (--m_max == 0) // underflow intentional and well defined
				return true;
		}
		return false;
	}

public:
	using filter_t = std::function<bool(ZDO::unsafe_value)>;
	using consumer_t = std::function<void(ZDO::unsafe_value)>;

	// Terminator function that runs consumers
	void Iterator(std::vector<HASH_t> &prefabHashes) {
		auto&& map = ZDOManager()->m_objectsByPrefab;
		for (auto&& prefabHash : prefabHashes) {
			if (Iterator(prefabHash))
				return;
		}
	}

	bool Iterator(HASH_t prefabHash) {
		auto&& map = ZDOManager()->m_objectsByPrefab;
		auto&& find = map.find(prefabHash);
		if (find != map.end()) {
			// then run 
			for (auto&& v : find->second) {
				auto&& zdo = ZDO::unsafe_value(v);
				if (_RunConsumer(zdo))
					return true;
			}
		}
		return false;
	}

	// Terminator function that runs consumers
	void Iterator() {
		for (auto&& v : ZDOManager()->m_objectsByID) {
			auto&& zdo = ZDO::make_unsafe_value(v);
			if (_RunConsumer(zdo))
				return;
		}
	}

	// Terminator function that runs consumers
	void Iterator(Vector3f pos, float radius) {
		auto zoneA = IZoneManager::WorldToZonePos(Vector3f(pos.x - radius, 0, pos.z - radius));
		auto zoneB = IZoneManager::WorldToZonePos(Vector3f(pos.x + radius, 0, pos.z + radius))
			+ ZoneID(1, 1);
		this->Distance(pos, radius)
			.Iterator(zoneA, zoneB);
	}

	// Terminator function that runs consumers
	void Iterator(ZoneID zoneA, ZoneID zoneB) {
		for (int z = zoneA.y; z < zoneB.y; z++) {
			for (int x = zoneA.x; x < zoneB.x; x++) {
				if (auto&& container = ZDOManager()->_GetZDOContainer(ZoneID(x, z))) {
					for (auto&& v : *container) {
						auto&& zdo = ZDO::make_unsafe_value(v);

						if (_RunConsumer(zdo))
							return;
					}
				}
			}
		}
	}
	
	// Terminator function that runs consumers
	void Iterator(ZoneID zone) {
		Iterator(zone, zone + ZoneID(1, 1));
	}

	// Return the first filter matched element / zdo
	template<typename Type, class ...Args>
	[[nodiscard]] decltype(auto) First(Args&&... args) {
		Type opt{};

		Limit(1).Consumer([&opt](ZDO::unsafe_value zdo) mutable {
			if constexpr (std::is_same_v<Type, ZDO>) {
				opt = *zdo;
			}
			else if constexpr (std::is_same_v<Type, ZDOID>) {
				opt = zdo->GetID();
			}
			else if constexpr (std::is_same_v<Type, ZDO::unsafe_value>) {
				opt = zdo;
			}
		}).Iterator(std::forward<Args...>(args)...);

		return opt;
	}

	// Convert this collector to an iterable structure
	// Arguments are passed to Iterator()
	// This utilizes Consumer, so the current consumer will be overridden
	// Similar to ranges::to
	template<typename Iterable, class ...Args>
		requires VUtils::Traits::is_iterable_v<Iterable>
	[[nodiscard]] decltype(auto) To(Args&&... args) {
		using Type = Iterable::value_type;

		Iterable t{};
		t.reserve(m_max); // Limit must be set, otherwise bad_alloc

		Consumer([&t](ZDO::unsafe_value zdo) {
			if constexpr (std::is_same_v<Type, ZDO>) {
				t.insert(t.end(), *zdo);
			}
			else if constexpr (std::is_same_v<Type, ZDOID>) {
				t.insert(t.end(), zdo->GetID());
			}
			else if constexpr (std::is_same_v<Type, ZDO::unsafe_value>) {
				t.insert(t.end(), zdo);
			}
		}).Iterator(std::forward<Args...>(args)...);

		return t;
	}

	// Consumer function that runs for every zdo matching all filters
	// Only 1 consumer can exist
	[[nodiscard]] ZDOCollector Consumer(consumer_t c) {
		this->m_consumer = c;
		return *this;
	}

	// Limiter to reduce the maximum amount of zdos which are successfully filtered
	[[nodiscard]] ZDOCollector Limit(size_t max) {
		this->m_max = max;
		return *this;
	}

	// Filter idiom for zdos within an area
	[[nodiscard]] ZDOCollector Distance(Vector3f pos, float radius) {
		return Filter([pos, sqRadius = radius * radius](ZDO::unsafe_value zdo) {
			return zdo->GetPosition().SqDistance(pos) <= sqRadius;
		});
	}

	// Filter out elements not matching the given predicate
	// Filters stack and are executed in the order they were applied.
	[[nodiscard]] ZDOCollector Filter(filter_t f) {
		this->m_filter = [first = m_filter, other = f](ZDO::unsafe_value zdo) -> bool { 
			if (first(zdo))
				return other(zdo);
			return false;
		};

		return *this;
	}

	
	/*
	// Get a capped number of ZDOs within a zone matching an optional predicate
	[[nodiscard]] void SomeZDOs(ZoneID zone, size_t max, consumer_t consumer) {
		if (auto&& container = ZDOManager()->_GetZDOContainer(zone)) {
			for (auto&& v : *container) {
				auto&& zdo = ZDO::make_unsafe_value(v);
				if (!consumer || consumer(zdo)) {
					if (max--)
						out.push_back(zdo);
					else
						return out;
				}
			}
		}

		return out;
	}
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
	}*/

private:
	filter_t m_filter = [](ZDO::unsafe_value) -> bool { return true; };
	consumer_t m_consumer;

	size_t m_max{};
};