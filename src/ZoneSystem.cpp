#include "ZoneSystem.h"

#include <utility>
#include "NetPackage.h"
#include "NetRouteManager.h"
#include "HeightMap.h"
#include "VUtilsRandom.h"
#include "WorldGenerator.h"
#include "HashUtils.h"
//#include ""

namespace ZoneSystem {
	
	struct ZoneLocation {
		// Token: 0x0600118C RID: 4492 RVA: 0x00075999 File Offset: 0x00073B99
		//ZoneLocation(const ZoneLocation& other); // copy constructor

		// everything below is serialized by unity

		bool m_enable = true;

		// Token: 0x040011BD RID: 4541
		std::string m_prefabName;

		// Token: 0x040011BE RID: 4542
		//[BitMask(typeof(Heightmap.Biome))]
		Heightmap::Biome m_biome;

		// Token: 0x040011BF RID: 4543
		//[BitMask(typeof(Heightmap.BiomeArea))]
		Heightmap::BiomeArea m_biomeArea = Heightmap::BiomeArea::Everything;

		int m_quantity;

		float m_chanceToSpawn = 10.f;

		bool m_prioritized;

		bool m_centerFirst;

		bool m_unique;

		std::string m_group;

		float m_minDistanceFromSimilar;

		bool m_iconAlways;

		bool m_iconPlaced;

		bool m_randomRotation = true;

		bool m_slopeRotation;

		bool m_snapToWater;

		float m_minTerrainDelta;

		float m_maxTerrainDelta = 2.f;

		bool m_inForest;

		float m_forestTresholdMin;

		float m_forestTresholdMax = 1.f;

		float m_minDistance;

		float m_maxDistance;

		float m_minAltitude = -1000.f;

		float m_maxAltitude = 1000.f;


		// everything below is marked as
		// non serializable


		//GameObject m_prefab;
		//
		//int m_hash;
		//
		//Location m_location;
		//
		float m_interiorRadius = 10;
		//
		float m_exteriorRadius = 10;
		//
		//Vector3 m_interiorPosition;
		//
		//Vector3 m_generatorPosition;
		//
		//List<ZNetView> m_netViews = new List<ZNetView>();
		//
		//List<RandomSpawn> m_randomSpawns = new List<RandomSpawn>();
		//
		//bool m_foldout;
	};

	struct LocationInstance {
		ZoneLocation m_location;
		Vector3 m_position;
		bool m_placed;
	};

	robin_hood::unordered_set<std::string> m_globalKeys;

	// used for runestones/vegvisirs/boss temples/crypts/... any feature
	robin_hood::unordered_map<Vector2i, LocationInstance, HashUtils::Hasher> m_locationInstances;

	robin_hood::unordered_set<Vector2i, HashUtils::Hasher> m_generatedZones;

	//static const char* TEMPLE_START = "StartTemple";

	static constexpr float ZONE_SIZE = 64;

	void SendGlobalKeys(OWNER_t target) {
        LOG(INFO) << "Sending global keys to " << target;
		NetRouteManager::Invoke(target, Routed_Hash::GlobalKeys, m_globalKeys);
	}

	void GetLocationIcons(std::vector<std::pair<Vector3, std::string>> &icons) {
		throw std::runtime_error("Not implemented");
		
		for (auto&& pair : m_locationInstances) {
			auto&& loc = pair.second;
			if (loc.m_location.m_iconAlways
				|| (loc.m_location.m_iconPlaced && loc.m_placed))
			{
				//icons[loc.m_position] = loc.m_location.m_prefabName;
			}
		}

		//while (enumerator.MoveNext())
		//{
		//	ZoneSystem.LocationInstance locationInstance = enumerator.Current;
		//	if (locationInstance.m_location.m_iconAlways || (locationInstance.m_location.m_iconPlaced && locationInstance.m_placed))
		//	{
		//		icons[locationInstance.m_position] = locationInstance.m_location.m_prefabName;
		//	}
		//}
	}

	void SendLocationIcons(OWNER_t target) {
        LOG(INFO) << "Senging location icons to " << target;

		NetPackage pkg;

        // TODO this is temporarary to get the client to login to the world

		// count
		pkg.Write((int32_t)1);
		// key
		pkg.Write(Vector3{ 0, 40, 0 });
		// value
		pkg.Write(LOCATION_SPAWN);

		//tempIconList.Clear();
		//GetLocationIcons(this.tempIconList);
		//zpackage.Write(this.tempIconList.Count);
		//foreach(KeyValuePair<Vector3, string> keyValuePair in this.tempIconList)
		//{
		//	pkg->Write(keyValuePair.Key);
		//	pkg->Write(keyValuePair.Value);
		//}
		
		NetRouteManager::Invoke(target, Routed_Hash::LocationIcons, pkg);
	}

	// private
	Vector2i GetRandomZone(VUtils::Random::State &state, float range) {
		int num = (int32_t) range / (int32_t) ZONE_SIZE;
		Vector2i vector2i;
		do {
			vector2i = Vector2i(state.Range(-num, num), state.Range(-num, num));
		} while (GetZonePos(vector2i).Magnitude() >= 10000);
		return vector2i;
	}

	// public
	Vector3 GetZonePos(const Vector2i &id) {
		return {(float)id.x * ZONE_SIZE, 0, (float)id.y * ZONE_SIZE};
	}

	// private
	bool IsZoneGenerated(const Vector2i &zoneID) {
		return m_generatedZones.contains(zoneID);
	}

	// private
	void RegisterLocation(ZoneLocation location, const Vector3 &pos, bool generated) {
		auto zone = GetZoneCoords(pos);
		if (m_locationInstances.contains(zone)) {
			LOG(ERROR) << "Location already exist in zone " << zone.x << " " << zone.y;
		}
		else {
			LocationInstance value;
			value.m_location = std::move(location);
			value.m_position = pos;
			value.m_placed = generated;
			m_locationInstances[zone] = value;
		}
	}

    // TODO
    //  work on getting some locations to spawn, for instance, START_TEMPLE
    //  this is probably the most important thing to get implemented
    //  Next would be objects and better ZDO syncing
	// private
	void GenerateLocations(const ZoneLocation& location) {
		VUtils::Random::State state(WorldGenerator::GetSeed() + VUtils::String::GetStableHashCode(location.m_prefabName));
        const float locationRadius = std::max(location.m_exteriorRadius, location.m_interiorRadius);
        unsigned int spawnedLocations = 0;

		unsigned int errLocations = 0;
		unsigned int errCenterDistances = 0;
		unsigned int errNoneBiomes = 0;
		unsigned int errBiomeArea = 0;
		unsigned int errAltitude = 0;
		unsigned int errForestFactor = 0;
		unsigned int errSimilarLocation = 0;
		unsigned int errTerrainDelta = 0;

		for (auto&& inst : m_locationInstances) {
			if (inst.second.m_location.m_prefabName == location.m_prefabName)
				spawnedLocations++;
		}
		if (spawnedLocations)
			LOG(INFO) << "Old location found " << location.m_prefabName << " x " << spawnedLocations;



		float range = location.m_centerFirst ? location.m_minDistance : 10000;

		if (location.m_unique && spawnedLocations)
			return;

        const unsigned int spawnAttempts = location.m_prioritized ? 200000 : 100000;
        for (unsigned int a=0; a < spawnAttempts && spawnedLocations < location.m_quantity; a++) {
			Vector2i randomZone = GetRandomZone(state, range);
			if (location.m_centerFirst)
				range++;

			if (m_locationInstances.contains(randomZone))
				errLocations++;
			else if (!IsZoneGenerated(randomZone)) {
				Vector3 zonePos = GetZonePos(randomZone);
				Heightmap::BiomeArea biomeArea = WorldGenerator::GetBiomeArea(zonePos);
				if ((location.m_biomeArea & biomeArea) == (Heightmap::BiomeArea)0)
					errBiomeArea++;
				else {
					for (int i = 0; i < 20; i++) {

						// generate point in zone
						float num = ZONE_SIZE / 2.f;
						float x = state.Range(-num + locationRadius, num - locationRadius);
						float z = state.Range(-num + locationRadius, num - locationRadius);
						Vector3 randomPointInZone = zonePos + Vector3(x, 0, z);



						float magnitude = randomPointInZone.Magnitude();
						if ((location.m_minDistance != 0 && magnitude < location.m_minDistance)
                            || (location.m_maxDistance != 0 && magnitude > location.m_maxDistance))
							errCenterDistances++;
						else {
							auto biome = WorldGenerator::GetBiome(randomPointInZone);
							if ((location.m_biome & biome) == Heightmap::Biome::None)
								errNoneBiomes++;
							else {
								randomPointInZone.y = WorldGenerator::GetHeight(randomPointInZone.x, randomPointInZone.z);
								float waterDiff = randomPointInZone.y - WATER_LEVEL;
								if (waterDiff < location.m_minAltitude || waterDiff > location.m_maxAltitude)
									errAltitude++;
								else {
									if (location.m_inForest) {
										float forestFactor = WorldGenerator::GetForestFactor(randomPointInZone);
										if (forestFactor < location.m_forestTresholdMin
                                            || forestFactor > location.m_forestTresholdMax) {
											errForestFactor++;
                                            continue;
										}
									}

									float delta = 0;
									Vector3 vector;
									WorldGenerator::GetTerrainDelta(state, randomPointInZone, location.m_exteriorRadius, delta, vector);
									if (delta > location.m_maxTerrainDelta
                                        || delta < location.m_minTerrainDelta)
										errTerrainDelta++;
									else {
										if (location.m_minDistanceFromSimilar <= 0 ) {
                                            bool locInRange = false;
											for (auto&& inst : m_locationInstances) {
												auto&& loc = inst.second.m_location;
												if ((loc.m_prefabName == location.m_prefabName 
													|| (!location.m_group.empty() && location.m_group == loc.m_group))
													&& inst.second.m_position.Distance(randomPointInZone) < location.m_minDistanceFromSimilar)
												{
													locInRange = true;
													break;
												}
											}
											
											if (!locInRange) {
												RegisterLocation(location, randomPointInZone, false);
												spawnedLocations++;
												break;
											}
										}
										errSimilarLocation++;
									}
								}
							}
						}
					}
				}
			}
		}

		if (spawnedLocations < location.m_quantity) {
            LOG(ERROR) << "Failed to place all " << location.m_prefabName << ", placed " << spawnedLocations << "/" << location.m_quantity;

            LOG(DEBUG) << "errLocations " << errLocations;
            LOG(DEBUG) << "errCenterDistances " << errCenterDistances;
            LOG(DEBUG) << "errNoneBiomes " << errNoneBiomes;
            LOG(DEBUG) << "errBiomeArea " << errBiomeArea;
            LOG(DEBUG) << "errAltitude " << errAltitude;
            LOG(DEBUG) << "errForestFactor " << errForestFactor;
            LOG(DEBUG) << "errSimilarLocation " << errSimilarLocation;
            LOG(DEBUG) << "errTerrainDelta " << errTerrainDelta;
		}
	}



	void OnNewPeer(NetPeer *peer) {
		SendGlobalKeys(peer->m_uuid);
		SendLocationIcons(peer->m_uuid);
	}

	Vector2i GetZoneCoords(const Vector3 &point) {
		int x = (int)((point.x + ZONE_SIZE / 2.f) / ZONE_SIZE);
		int y = (int)((point.z + ZONE_SIZE / 2.f) / ZONE_SIZE);
		return { x, y };
	}

	void Init() {
		// inserts the blank dummy key
		m_globalKeys.insert("");
	}
}