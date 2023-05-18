#pragma once

#include <string.h>

#include "VUtils.h"
#include "VUtilsRandom.h"
#include "HashUtils.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "Vector.h"
#include "Quaternion.h"

using ZoneID = Vector2i;

class Heightmap;
class Peer;

template<typename SizeType, size_t EXT = 0>
	requires std::is_integral_v<SizeType>
class VHString {
private:
	char* m_encoded;

	bool same(const char* a, const char* b, SizeType size) const {
		for (SizeType i = 0; i < size; i++) {
			if (a[i] != b[i])
				return false;
		}
		return true;
	}

public:
	VHString(const char* str, SizeType length) {
		// If length is 0, encoded will be null (instead of allocating SizeType bytes for nothing)
		if (length || EXT) {
			assert(str);
			m_encoded = (char*) std::malloc(sizeof(SizeType) + length + EXT);
			if (m_encoded) {
				if (length) {
					// Copy length into encoded
					std::memcpy(this->m_encoded + EXT, &length, sizeof(length));
					// Copy string data afterwards
					std::memcpy(this->m_encoded + sizeof(length) + EXT, str, length);
				}
				else {
					std::memset(this->m_encoded, 0, EXT);
				}
			}
		}
		else {
			m_encoded = nullptr;
		}
	}

	VHString() : VHString(nullptr, 0) {}

	template<typename T>
		requires std::is_integral_v<T>
	VHString(const VHString<T>& other) : VHString(other., other.m_size) {}



	~VHString() {
		free(m_str);
	}

	size_t size() const {
		if (m_encoded)
			return *reinterpret_cast<SizeType*>(m_encoded + EXT);
		else
			return 0;
	}

	const char* data() const {
		if (m_encoded)
			return m_encoded + sizeof(SizeType) + EXT;
		return nullptr;
	}

	char* data() {
		if (m_encoded)
			return m_encoded + sizeof(SizeType) + EXT;
		return nullptr;
	}

	bool operator==(const VHString& other) const {
		// If memory addresses are same
		//	OR
		//		lengths are equal and string payloads are equal
		if (this == &other)
			return true;
				
		auto sz = this->size();
		return sz == other.size() 
			&& same(this->data(), other.data(), sz);
	}

	//bool operator==(const char* other) const {
	//	return std::strcmp(this->m_str, other) == 0;
	//}

	bool operator==(std::string_view other) const {
		auto sz = this->size();
		return sz == other.size() 
			&& same(this->m_str + sizeof(SizeType), other.data(), sz);
	}
};

using VHString8 = VHString<uint8_t>;
using VHString16 = VHString<uint16_t>;

class IZoneManager {
	friend class INetManager;
	friend class IModManager;

public:
	class Feature {
		friend class IModManager;

	public:
		//std::string m_name;
		//char* m_name;

		//uint8_t m_flags;
		VHString8 m_name;
		//HASH_t m_hash;
		//bool m_iconAlways;
		//bool m_iconPlaced;
		//bool m_unique;

		bool operator==(const Feature& other) const {
			return this == &other;
			//return this->m_name == other.m_name;
			//return this->m_hash == other.m_hash;
		}
	};
	static constexpr auto szzz = sizeof(Feature);
	static constexpr auto szzz1 = sizeof(std::string);

	using Instance = std::pair<std::reference_wrapper<const Feature>, Vector3f>;

public:
	static constexpr int NEAR_ACTIVE_AREA = 2;
	static constexpr int DISTANT_ACTIVE_AREA = 2;
	static constexpr int ZONE_SIZE = 64;
	static constexpr float WATER_LEVEL = 30;
	static constexpr int WORLD_RADIUS_IN_ZONES = 157;
	static constexpr int WORLD_DIAMETER_IN_ZONES = WORLD_RADIUS_IN_ZONES * 2;

private:
	std::list<std::unique_ptr<const Feature>> m_corefeatures;

	// All the generated Features in a world
	//UNORDERED_MAP_t<ZoneID, Instance> m_generatedFeatures;
	//std::list<

	// Game-state global keys
	UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_globalKeys;

private:
	void SendGlobalKeys();
	void SendGlobalKeys(Peer& peer);

	void SendLocationIcons();
	void SendLocationIcons(Peer& peer);

	void OnNewPeer(Peer& peer);

	bool HaveLocationInRange(HASH_t feature, Vector3f pos);

public:
	void PostPrefabInit();

	void Save(DataWriter& pkg);
	void Load(DataReader& reader, int32_t version);

	auto& GlobalKeys() {
		return m_globalKeys;
	}

	// Get the client based icons for minimap
	std::list<Instance> GetFeatureIcons();

	// Find the nearest location
	//	Nullable
	Instance *GetNearestFeature(std::string_view name, Vector3f pos);

	static ZoneID WorldToZonePos(Vector3f pos);
	static Vector3f ZoneToWorldPos(ZoneID zone);

	bool ZonesOverlap(ZoneID zone, Vector3f areaPoint);
	bool ZonesOverlap(ZoneID zone, ZoneID areaZone);

	bool IsPeerNearby(ZoneID zone, OWNER_t uid);
};

// Manager class for everything related to world generation
IZoneManager* ZoneManager();
