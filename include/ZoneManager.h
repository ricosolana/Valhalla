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

/*
class VHAsciiEncodedString {
private:
	char* m_encoded;

	bool same(const char* a, const char* b, uint8_t size) const {
		for (uint8_t i = 0; i < size; i++) {
			if (a[i] != b[i])
				return false;
		}
		return true;
	}

public:
	VHAsciiEncodedString(const char* str, uint8_t length) {
		// If length is 0, encoded will be null (instead of allocating SizeType bytes for nothing)
		if (length) {
			m_encoded = (char*) std::malloc(sizeof(uint8_t) + length);
			if (length <= 0b111111 && str && m_encoded) {
				// Copy length into encoded
				std::memcpy(this->m_encoded, &length, sizeof(length));
				// Copy string data afterwards
				std::memcpy(this->m_encoded + sizeof(length), str, length);
			}
			else {
				exit(-1);
			}
		}
		else {
			m_encoded = nullptr;
		}
	}

	VHAsciiEncodedString() : VHAsciiEncodedString(nullptr, 0) {}

	VHAsciiEncodedString(const VHAsciiEncodedString& other) : VHAsciiEncodedString(other.data(), other.size()) {}



	~VHAsciiEncodedString() {
		free(this->m_encoded);
	}

	size_t size() const {
		if (this->m_encoded)
			return *reinterpret_cast<uint8_t*>(this->m_encoded);
		else
			return 0;
	}

	const char* data() const {
		if (this->m_encoded)
			return this->m_encoded + sizeof(uint8_t);
		return nullptr;
	}

	char* data() {
		if (this->m_encoded)
			return this->m_encoded + sizeof(uint8_t);
		return nullptr;
	}

	bool operator==(const VHAsciiEncodedString& other) const {
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
			&& same(this->m_encoded + sizeof(uint8_t), other.data(), sz);
	}
};*/

class IZoneManager {
	friend class INetManager;

public:
	// A null name denotes that the feature is unknown or ignored (to save space)
	using Instance = std::pair<const char*, Vector3f>;

public:
	static constexpr int NEAR_ACTIVE_AREA = 2;
	static constexpr int DISTANT_ACTIVE_AREA = 2;
	static constexpr int ZONE_SIZE = 64;
	static constexpr float WATER_LEVEL = 30;
	static constexpr int WORLD_RADIUS_IN_ZONES = 157;
	static constexpr int WORLD_DIAMETER_IN_ZONES = WORLD_RADIUS_IN_ZONES * 2;

private:
	//std::list<std::unique_ptr<const Feature>> m_corefeatures;
	static constexpr const char* FEATURE_START_TEMPLE = "StartTemple";
	static constexpr const char* FEATURE_HALDOR = "Vendor_BlackForest";
	static constexpr const char* FEATURE_EIKTHYR = "Eikthyrnir";
	static constexpr const char* FEATURE_ELDER = "GDKing";
	static constexpr const char* FEATURE_BONEMASS = "Bonemass";
	static constexpr const char* FEATURE_MODER = "Dragonqueen";
	static constexpr const char* FEATURE_YAGLUTH = "GoblinKing";
	static constexpr const char* FEATURE_QUEEN = "DvergrBoss";

	std::vector<Instance> m_generatedFeatures;

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
	bool IsZoneGenerated(ZoneID zone);

	void OnNewPeer(Peer& peer);

public:
	void PostPrefabInit();

	void Save(DataWriter& pkg);
	void Load(DataReader& reader, int32_t version);
	
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
