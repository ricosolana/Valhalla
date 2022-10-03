#include "ZoneSystem.h"
#include "NetRpcManager.h"

namespace ZoneSystem {

	struct ZoneLocation {
		// Token: 0x0600118C RID: 4492 RVA: 0x00075999 File Offset: 0x00073B99
		public ZoneSystem.ZoneLocation Clone()
		{
			return base.MemberwiseClone() as ZoneSystem.ZoneLocation;
		}

		// everything below is serialized by unity

		// Token: 0x040011BC RID: 4540
		public bool m_enable = true;

		// Token: 0x040011BD RID: 4541
		public string m_prefabName;

		// Token: 0x040011BE RID: 4542
		[BitMask(typeof(Heightmap.Biome))]
		public Heightmap.Biome m_biome;

		// Token: 0x040011BF RID: 4543
		[BitMask(typeof(Heightmap.BiomeArea))]
		public Heightmap.BiomeArea m_biomeArea = Heightmap.BiomeArea.Everything;

		public int m_quantity;

		public float m_chanceToSpawn = 10f;

		public bool m_prioritized;

		public bool m_centerFirst;

		public bool m_unique;

		public string m_group = "";

		public float m_minDistanceFromSimilar;

		public bool m_iconAlways;

		public bool m_iconPlaced;

		public bool m_randomRotation = true;

		public bool m_slopeRotation;

		public bool m_snapToWater;

		public float m_minTerrainDelta;

		public float m_maxTerrainDelta = 2f;

		public bool m_inForest;

		public float m_forestTresholdMin;

		public float m_forestTresholdMax = 1f;

		public float m_minDistance;

		public float m_maxDistance;

		public float m_minAltitude = -1000f;

		public float m_maxAltitude = 1000f;


		// everything below is marked as
		// non serializable


		public GameObject m_prefab;

		public int m_hash;

		public Location m_location;

		public float m_interiorRadius = 10f;

		public float m_exteriorRadius = 10f;

		public Vector3 m_interiorPosition;

		public Vector3 m_generatorPosition;

		public List<ZNetView> m_netViews = new List<ZNetView>();

		public List<RandomSpawn> m_randomSpawns = new List<RandomSpawn>();

		public bool m_foldout;
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