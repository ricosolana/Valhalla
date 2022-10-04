#include "ZoneSystem.h"
#include "NetRpcManager.h"
#include "HeightMap.h"

namespace ZoneSystem {

	struct ZoneLocation {
		// Token: 0x0600118C RID: 4492 RVA: 0x00075999 File Offset: 0x00073B99
		ZoneLocation(const ZoneLocation& other); // copy constructor

		// everything below is serialized by unity

		bool m_enable = true;

		// Token: 0x040011BD RID: 4541
		std::string m_prefabName;

		// Token: 0x040011BE RID: 4542
		//[BitMask(typeof(Heightmap.Biome))]
		Biome m_biome;

		// Token: 0x040011BF RID: 4543
		//[BitMask(typeof(Heightmap.BiomeArea))]
		BiomeArea m_biomeArea = BiomeArea::Everything;

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


		GameObject m_prefab;

		int m_hash;

		Location m_location;

		float m_interiorRadius = 10f;

		float m_exteriorRadius = 10f;

		Vector3 m_interiorPosition;

		Vector3 m_generatorPosition;

		List<ZNetView> m_netViews = new List<ZNetView>();

		List<RandomSpawn> m_randomSpawns = new List<RandomSpawn>();

		bool m_foldout;
	};

	struct LocationInstance {
		ZoneLocation m_location;
		Vector3 m_position;
		bool m_placed;
	};

	static robin_hood::unordered_set<std::string> m_globalKeys;

	// used for runestones/vegvisirs
	static std::vector<std::pair<Vector2i, LocationInstance>> m_locationInstances;

	void SendGlobalKeys(uuid_t target) {
		NetRpcManager::InvokeRoute(target, "GlobalKeys", m_globalKeys);
	}

	void GetLocationIcons(std::vector<std::pair<Vector3, std::string>> &icons) {
		for (auto&& pair : m_locationInstances) {
			auto&& loc = pair.second;
			if (loc.m_location.m_iconAlways
				|| (loc.m_location.m_iconPlaced && loc.m_placed))
			{
				icons[loc.m_position] = loc.m_location.m_prefabName;
			}
		}

		while (enumerator.MoveNext())
		{
			ZoneSystem.LocationInstance locationInstance = enumerator.Current;
			if (locationInstance.m_location.m_iconAlways || (locationInstance.m_location.m_iconPlaced && locationInstance.m_placed))
			{
				icons[locationInstance.m_position] = locationInstance.m_location.m_prefabName;
			}
		}
	}

	void SendLocationIcons(uuid_t target) {
		auto pkg(PKG());
		tempIconList.Clear();
		GetLocationIcons(this.tempIconList);
		zpackage.Write(this.tempIconList.Count);
		foreach(KeyValuePair<Vector3, string> keyValuePair in this.tempIconList)
		{
			pkg->Write(keyValuePair.Key);
			pkg->Write(keyValuePair.Value);
		}
		NetRpcManager::

		ZRoutedRpc.instance.InvokeRoutedRPC(peer, "LocationIcons", new object[]
			{
				zpackage
			});
	}

	void OnNewPeer(uuid_t target) {
		SendGlobalKeys(target);
		//SendLocationIcons(target);
	}
}