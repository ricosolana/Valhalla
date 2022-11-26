#include "ZoneSystem.h"

#include <utility>
#include "NetPackage.h"
#include "NetRouteManager.h"
#include "HeightMap.h"
#include "VUtilsRandom.h"
#include "WorldGenerator.h"
#include "VUtilsResource.h"
#include "HashUtils.h"
#include <openssl/aes.h>
//#include ""

namespace ZoneSystem {
	
	struct ZoneLocation {
		// Token: 0x0600118C RID: 4492 RVA: 0x00075999 File Offset: 0x00073B99
		//ZoneLocation(const ZoneLocation& other); // copy constructor

		// everything below is serialized by unity

		//bool m_enable = true;

		// Token: 0x040011BD RID: 4541
		std::string m_prefabName;

		// Token: 0x040011BE RID: 4542
		//[BitMask(typeof(Heightmap.Biome))]
		Heightmap::Biome m_biome;

		// Token: 0x040011BF RID: 4543
		//[BitMask(typeof(Heightmap.BiomeArea))]
		Heightmap::BiomeArea m_biomeArea; // = Heightmap::BiomeArea::Everything;

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

	ZoneLocation StartTemple = { "StartTemple", Heightmap::Meadows, (Heightmap::BiomeArea)2, 1, 100, true, true, false, "", 0, true, false, false, false, false, 0, 3, true, 1, 5, 0, 10000, 3, 1000 };
	ZoneLocation Eikthyrnir = { "Eikthyrnir", Heightmap::Meadows, (Heightmap::BiomeArea)2, 3, 15, true, false, false, "", 0, false, false, true, false, false, 0, 3, false, 1, 5, 0, 1000, 1, 1000 };
	ZoneLocation GoblinKing = { "GoblinKing", Heightmap::Plains, (Heightmap::BiomeArea)2, 4, 15, true, false, false, "", 3000, false, false, true, false, false, 0, 4, false, 1, 5, 0, 0, 1, 1000 };
	ZoneLocation GDKing = { "GDKing", Heightmap::BlackForest, (Heightmap::BiomeArea)6, 4, 15, true, false, false, "", 3000, false, false, true, false, false, 0, 5, false, 1, 5, 1000, 7000, 1, 1000 };
	ZoneLocation Bonemass = { "Bonemass", Heightmap::Swamp, (Heightmap::BiomeArea)6, 5, 20, true, false, false, "", 3000, false, false, true, false, false, 0, 4, false, 1, 5, 2000, 10000, 0, 2 };
	ZoneLocation SunkenCrypt4 = { "SunkenCrypt4", Heightmap::Swamp, (Heightmap::BiomeArea)2, 400, 15, true, false, false, "", 64, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 0, 2 };
	ZoneLocation Vendor_BlackForest = { "Vendor_BlackForest", Heightmap::BlackForest, (Heightmap::BiomeArea)2, 10, 10, true, false, false, "", 512, false, true, true, false, false, 0, 2, false, 0, 1, 1500, 0, 1, 1000 };
	ZoneLocation Dragonqueen = { "Dragonqueen", Heightmap::Mountain, (Heightmap::BiomeArea)2, 3, 10, true, false, false, "", 3000, false, false, true, false, false, 0, 4, false, 0, 1, 0, 8000, 150, 500 };
	ZoneLocation StoneCircle = { "StoneCircle", Heightmap::Heightmap::Meadows, (Heightmap::BiomeArea)7, 25, 5, false, false, false, "", 200, false, false, true, false, false, 0, 3, false, 1, 5, 0, 0, 1, 1000 };
	ZoneLocation Greydwarf_camp1 = { "Greydwarf_camp1", Heightmap::BlackForest, (Heightmap::BiomeArea)2, 300, 15, false, false, false, "", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Runestone_Greydwarfs = { "Runestone_Greydwarfs", Heightmap::BlackForest, (Heightmap::BiomeArea)7, 25, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 1, 99, 0, 2000, 1, 1000 };
	ZoneLocation Grave1 = { "Grave1", Heightmap::Swamp, (Heightmap::BiomeArea)6, 200, 15, false, false, false, "", 0, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 0.5, 1000 };
	ZoneLocation SwampRuin1 = { "SwampRuin1", Heightmap::Swamp, (Heightmap::BiomeArea)6, 50, 15, false, false, false, "", 512, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 0, 1000 };
	ZoneLocation SwampRuin2 = { "SwampRuin2", Heightmap::Swamp, (Heightmap::BiomeArea)6, 50, 15, false, false, false, "", 512, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 0, 1000 };
	ZoneLocation FireHole = { "FireHole", Heightmap::Swamp, (Heightmap::BiomeArea)6, 200, 15, false, false, false, "", 0, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 0.5, 1000 };
	
	ZoneLocation Runestone_Draugr = { "Runestone_Draugr", Heightmap::Swamp, (Heightmap::BiomeArea)-1, 50, 5, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 0.5, 1000 };
	ZoneLocation Meteorite = { "Meteorite", Heightmap::AshLands, (Heightmap::BiomeArea)7, 500, 10, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 1, 5, 0, 0, 1, 1000 };
	ZoneLocation Crypt2 = { "Crypt2", Heightmap::BlackForest, (Heightmap::BiomeArea)-1, 200, 20, false, false, false, "", 128, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Ruin1 = { "Ruin1", Heightmap::BlackForest, (Heightmap::BiomeArea)-1, 200, 15, false, false, false, "", 0, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Ruin2 = { "Ruin2", Heightmap::BlackForest, (Heightmap::BiomeArea)-1, 200, 15, false, false, false, "", 0, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation StoneHouse3 = { "StoneHouse3", Heightmap::BlackForest, (Heightmap::BiomeArea)-1, 200, 15, false, false, false, "", 0, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation StoneHouse4 = { "StoneHouse4", Heightmap::BlackForest, (Heightmap::BiomeArea)-1, 200, 15, false, false, false, "", 0, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Ruin3 = { "Ruin3", Heightmap::Plains, (Heightmap::BiomeArea)-1, 50, 15, false, false, false, "Goblintower", 512, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation GoblinCamp2 = { "GoblinCamp2", Heightmap::Plains, (Heightmap::BiomeArea)-1, 200, 10, false, false, false, "", 250, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation StoneTower1 = { "StoneTower1", Heightmap::Plains, (Heightmap::BiomeArea)-1, 50, 10, false, false, false, "Goblintower", 512, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation StoneTower3 = { "StoneTower3", Heightmap::Plains, (Heightmap::BiomeArea)-1, 50, 10, false, false, false, "Goblintower", 512, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation StoneHenge1 = { "StoneHenge1", Heightmap::Plains, (Heightmap::BiomeArea)-1, 5, 10, false, false, false, "Stonehenge", 1000, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 5, 1000 };
	ZoneLocation StoneHenge2 = { "StoneHenge2", Heightmap::Plains, (Heightmap::BiomeArea)-1, 5, 10, false, false, false, "Stonehenge", 1000, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 5, 1000 };
	ZoneLocation StoneHenge3 = { "StoneHenge3", Heightmap::Plains, (Heightmap::BiomeArea)-1, 5, 10, false, false, false, "Stonehenge", 1000, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 5, 1000 };
	ZoneLocation StoneHenge4 = { "StoneHenge4", Heightmap::Plains, (Heightmap::BiomeArea)-1, 5, 10, false, false, false, "Stonehenge", 1000, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 5, 1000 };
	ZoneLocation StoneHenge5 = { "StoneHenge5", Heightmap::Plains, (Heightmap::BiomeArea)-1, 20, 10, false, false, false, "Stonehenge", 500, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation StoneHenge6 = {"StoneHenge6", Heightmap::Plains, (Heightmap::BiomeArea)-1, 20, 10, false, false, false, "Stonehenge", 500, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation WoodHouse1 = {"WoodHouse1", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse2 = {"WoodHouse2", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse3 = {"WoodHouse3", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse4 = {"WoodHouse4", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse5 = {"WoodHouse5", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse6 = {"WoodHouse6", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse7 = {"WoodHouse7", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse8 = {"WoodHouse8", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse9 = {"WoodHouse9", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse10 = {"WoodHouse10", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse11 = {"WoodHouse11", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse12 = {"WoodHouse12", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodHouse13 = {"WoodHouse13", Heightmap::Meadows, (Heightmap::BiomeArea)7, 20, 15, false, false, false, "", 0, false, false, true, false, false, 0, 4, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation WoodFarm1 = {"WoodFarm1", Heightmap::Meadows, (Heightmap::BiomeArea)7, 10, 15, false, false, false, "woodvillage", 128, false, false, true, false, false, 0, 4, false, 0, 1, 500, 2000, 1, 1000 };
	ZoneLocation WoodVillage1 = {"WoodVillage1", Heightmap::Meadows, (Heightmap::BiomeArea)7, 15, 15, false, false, false, "woodvillage", 256, false, false, true, false, false, 0, 4, false, 0, 1, 2000, 10000, 1, 1000 };
	ZoneLocation TrollCave02 = {"TrollCave02", Heightmap::BlackForest, (Heightmap::BiomeArea)2, 250, 20, false, false, false, """", 256, false, false, true, true, false, 5, 10, false, 0, 1, 0, 0, 3, 1000 };
	ZoneLocation Dolmen01 = {"Dolmen01", (Heightmap::Biome)9, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "", 0, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Dolmen02 = {"Dolmen02", (Heightmap::Biome)9, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "", 0, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Dolmen03 = {"Dolmen03", (Heightmap::Biome)9, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "", 0, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Crypt3 = {"Crypt3", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 200, 10, false, false, false, "", 128, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 3, 1000 };
	ZoneLocation Crypt4 = {"Crypt4", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 200, 10, false, false, false, "", 128, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation InfestedTree01 = { "InfestedTree01", Heightmap::Swamp, (Heightmap::BiomeArea)3, 700, 10, false, false, false, "", 0, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, -1, 1000 };
	ZoneLocation SwampHut1 = {"SwampHut1", Heightmap::Swamp, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Swamphut", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, -2, 1000 };
	ZoneLocation SwampHut2 = {"SwampHut2", Heightmap::Swamp, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Swamphut", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation SwampHut3 = {"SwampHut3", Heightmap::Swamp, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Swamphut", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation SwampHut4 = {"SwampHut4", Heightmap::Swamp, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Swamphut", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, -1, 1000 };
	ZoneLocation SwampHut5 = {"SwampHut5", Heightmap::Swamp, (Heightmap::BiomeArea)3, 25, 10, false, false, false, "Swamphut", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, -1, 1000 };
	ZoneLocation SwampWell1 = {"SwampWell1", Heightmap::Swamp, (Heightmap::BiomeArea)3, 25, 10, false, false, false, "", 1024, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, -1, 1000 };
	ZoneLocation StoneTowerRuins04 = {"StoneTowerRuins04", Heightmap::Mountain, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Mountainruin", 128, false, false, true, true, false, 6, 40, false, 0, 1, 0, 0, 150, 1000 };
	ZoneLocation StoneTowerRuins05 = {"StoneTowerRuins05", Heightmap::Mountain, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Mountainruin", 128, false, false, true, false, false, 6, 40, false, 0, 1, 0, 0, 150, 1000 };
	ZoneLocation StoneTowerRuins03 = {"StoneTowerRuins03", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 80, 10, false, false, false, "Stonetowerruins", 200, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation StoneTowerRuins07 = {"StoneTowerRuins07", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 80, 10, false, false, false, "Stonetowerruins", 200, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation StoneTowerRuins08 = {"StoneTowerRuins08", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 80, 10, false, false, false, "Stonetowerruins", 200, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation StoneTowerRuins09 = {"StoneTowerRuins09", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 80, 10, false, false, false, "Stonetowerruins", 200, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation StoneTowerRuins10 = {"StoneTowerRuins10", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 80, 10, false, false, false, "Stonetowerruins", 200, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 2, 1000 };
	ZoneLocation ShipSetting01 = {"ShipSetting01", Heightmap::Meadows, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation DrakeNest01 = {"DrakeNest01", Heightmap::Mountain, (Heightmap::BiomeArea)3, 200, 10, false, false, false, "", 100, false, false, true, false, false, 0, 3, false, 0, 1, 0, 10000, 100, 2000 };
	ZoneLocation Waymarker01 = {"Waymarker01", Heightmap::Mountain, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "", 0, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation Waymarker02 = {"Waymarker02", Heightmap::Mountain, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "", 0, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation AbandonedLogCabin02 = {"AbandonedLogCabin02", Heightmap::Mountain, (Heightmap::BiomeArea)3, 33, 10, false, false, false, "Abandonedcabin", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation AbandonedLogCabin03 = {"AbandonedLogCabin03", Heightmap::Mountain, (Heightmap::BiomeArea)3, 33, 10, false, false, false, "Abandonedcabin", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation AbandonedLogCabin04 = {"AbandonedLogCabin04", Heightmap::Mountain, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Abandonedcabin", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation MountainGrave01 = {"MountainGrave01", Heightmap::Mountain, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "", 50, false, false, true, false, false, 0, 2, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation DrakeLorestone = {"DrakeLorestone", Heightmap::Mountain, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation MountainWell1 = {"MountainWell1", Heightmap::Mountain, (Heightmap::BiomeArea)3, 25, 10, false, false, false, "", 256, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation ShipWreck01 = {"ShipWreck01", (Heightmap::Biome)282, (Heightmap::BiomeArea)3, 25, 10, false, false, false, "Shipwreck", 1024, false, false, true, false, false, 0, 10, false, 0, 1, 0, 0, -1, 1 };
	ZoneLocation ShipWreck02 = {"ShipWreck02", (Heightmap::Biome)282, (Heightmap::BiomeArea)3, 25, 10, false, false, false, "Shipwreck", 1024, false, false, true, false, false, 0, 10, false, 0, 1, 0, 0, -1, 1 };
	ZoneLocation ShipWreck03 = {"ShipWreck03", (Heightmap::Biome)282, (Heightmap::BiomeArea)3, 25, 10, false, false, false, "Shipwreck", 1024, false, false, true, false, false, 0, 10, false, 0, 1, 0, 0, -1, 1 };
	ZoneLocation ShipWreck04 = {"ShipWreck04", (Heightmap::Biome)282, (Heightmap::BiomeArea)3, 25, 10, false, false, false, "Shipwreck", 1024, false, false, true, false, false, 0, 10, false, 0, 1, 0, 0, -1, 1 };
	ZoneLocation Runestone_Meadows = {"Runestone_Meadows", Heightmap::Meadows, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Runestone_Boars = {"Runestone_Boars", Heightmap::Meadows, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Runestone_Swamps = {"Runestone_Swamps", Heightmap::Swamp, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 0, 1000 };
	ZoneLocation Runestone_Mountains = {"Runestone_Mountains", Heightmap::Mountain, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 100, 1000 };
	ZoneLocation Runestone_BlackForest = {"Runestone_BlackForest", Heightmap::BlackForest, (Heightmap::BiomeArea)3, 50, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation Runestone_Plains = {"Runestone_Plains", Heightmap::Plains, (Heightmap::BiomeArea)3, 100, 10, false, false, false, "Runestones", 128, false, false, true, false, false, 0, 3, false, 0, 1, 0, 0, 1, 1000 };
	ZoneLocation TarPit1 = {"TarPit1", Heightmap::Plains, (Heightmap::BiomeArea)2, 100, 0, false, false, false, "tarpit", 128, false, false, true, false, false, 0, 1.5, false, 0, 0, 0, 0, 5, 60 };
	ZoneLocation TarPit2 = {"TarPit2", Heightmap::Plains, (Heightmap::BiomeArea)2, 100, 0, false, false, false, "tarpit", 128, false, false, true, false, false, 0, 1.5, false, 0, 0, 0, 0, 5, 60 };
	ZoneLocation TarPit3 = {"TarPit3", Heightmap::Plains, (Heightmap::BiomeArea)2, 100, 0, false, false, false, "tarpit", 128, false, false, true, false, false, 0, 1.5, false, 0, 0, 0, 0, 5, 60 };
	ZoneLocation MountainCave02 = {"MountainCave02", Heightmap::Mountain, (Heightmap::BiomeArea)3, 160, 0, false, false, false, "mountaincaves", 200, false, false, false, true, false, 0, 40, false, 0, 0, 0, 0, 100, 5000 };






	struct LocationInstance {
		ZoneLocation *m_location;
		Vector3 m_position;
		bool m_placed;
	};

	robin_hood::unordered_set<std::string> m_globalKeys;

	// used for runestones/vegvisirs/boss temples/crypts/... any feature
	robin_hood::unordered_map<Vector2i, LocationInstance, HashUtils::Hasher> m_locationInstances;

	robin_hood::unordered_set<Vector2i, HashUtils::Hasher> m_generatedZones;

	std::vector<std::unique_ptr<ZoneLocation>> m_locations;

	bool m_locationsGenerated = false;
		
	ZoneLocation *GetLocation(const std::string &name) {
		for (auto&& zoneLocation : m_locations)
			if (zoneLocation->m_prefabName == name)
				return zoneLocation.get();

		return nullptr;
	}

	void SendGlobalKeys(OWNER_t target) {
        LOG(INFO) << "Sending global keys to " << target;
		NetRouteManager::Invoke(target, NetHashes::Routed::GlobalKeys, m_globalKeys);
	}

	void GetLocationIcons(std::vector<std::pair<Vector3, std::string>> &icons) {
		throw std::runtime_error("Not implemented");
		
		for (auto&& pair : m_locationInstances) {
			auto&& loc = pair.second;
			if (loc.m_location->m_iconAlways
				|| (loc.m_location->m_iconPlaced && loc.m_placed))
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
        LOG(INFO) << "Sending location icons to " << target;

		assert(false);

		NetPackage pkg;

        // TODO this is temporarary to get the client to login to the world

		// count
		pkg.Write((int32_t)1);
		// key
		pkg.Write(Vector3{ 0, 40, 0 });
		// value
		//pkg.Write(LOCATION_SPAWN);

		//tempIconList.Clear();
		//GetLocationIcons(this.tempIconList);
		//zpackage.Write(this.tempIconList.Count);
		//foreach(KeyValuePair<Vector3, string> keyValuePair in this.tempIconList)
		//{
		//	pkg->Write(keyValuePair.Key);
		//	pkg->Write(keyValuePair.Value);
		//}
		
		NetRouteManager::Invoke(target, NetHashes::Routed::LocationIcons, pkg);
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
	void RegisterLocation(ZoneLocation *location, const Vector3 &pos, bool generated) {
		auto zone = GetZoneCoords(pos);
		if (m_locationInstances.contains(zone)) {
			LOG(ERROR) << "Location already exist in zone " << zone.x << " " << zone.y;
		}
		else {
			LocationInstance value;
			value.m_location = location;
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
	void GenerateLocations(ZoneLocation *location) {
		VUtils::Random::State state(WorldGenerator::GetSeed() + VUtils::String::GetStableHashCode(location->m_prefabName));
        const float locationRadius = std::max(location->m_exteriorRadius, location->m_interiorRadius);
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
			if (inst.second.m_location->m_prefabName == location->m_prefabName)
				spawnedLocations++;
		}
		if (spawnedLocations)
			LOG(INFO) << "Old location found " << location->m_prefabName << " x " << spawnedLocations;



		float range = location->m_centerFirst ? location->m_minDistance : 10000;

		if (location->m_unique && spawnedLocations)
			return;

        const unsigned int spawnAttempts = location->m_prioritized ? 200000 : 100000;
        for (unsigned int a=0; a < spawnAttempts && spawnedLocations < location->m_quantity; a++) {
			Vector2i randomZone = GetRandomZone(state, range);
			if (location->m_centerFirst)
				range++;

			if (m_locationInstances.contains(randomZone))
				errLocations++;
			else if (!IsZoneGenerated(randomZone)) {
				Vector3 zonePos = GetZonePos(randomZone);
				Heightmap::BiomeArea biomeArea = WorldGenerator::GetBiomeArea(zonePos);
				if ((location->m_biomeArea & biomeArea) == (Heightmap::BiomeArea)0)
					errBiomeArea++;
				else {
					for (int i = 0; i < 20; i++) {

						// generate point in zone
						float num = ZONE_SIZE / 2.f;
						float x = state.Range(-num + locationRadius, num - locationRadius);
						float z = state.Range(-num + locationRadius, num - locationRadius);
						Vector3 randomPointInZone = zonePos + Vector3(x, 0, z);



						float magnitude = randomPointInZone.Magnitude();
						if ((location->m_minDistance != 0 && magnitude < location->m_minDistance)
                            || (location->m_maxDistance != 0 && magnitude > location->m_maxDistance))
							errCenterDistances++;
						else {
							auto biome = WorldGenerator::GetBiome(randomPointInZone);
							if ((location->m_biome & biome) == Heightmap::Biome::None)
								errNoneBiomes++;
							else {
								randomPointInZone.y = WorldGenerator::GetHeight(randomPointInZone.x, randomPointInZone.z);
								float waterDiff = randomPointInZone.y - WATER_LEVEL;
								if (waterDiff < location->m_minAltitude || waterDiff > location->m_maxAltitude)
									errAltitude++;
								else {
									if (location->m_inForest) {
										float forestFactor = WorldGenerator::GetForestFactor(randomPointInZone);
										if (forestFactor < location->m_forestTresholdMin
                                            || forestFactor > location->m_forestTresholdMax) {
											errForestFactor++;
                                            continue;
										}
									}

									float delta = 0;
									Vector3 vector;
									WorldGenerator::GetTerrainDelta(state, randomPointInZone, location->m_exteriorRadius, delta, vector);
									if (delta > location->m_maxTerrainDelta
                                        || delta < location->m_minTerrainDelta)
										errTerrainDelta++;
									else {
										if (location->m_minDistanceFromSimilar <= 0 ) {
                                            bool locInRange = false;
											for (auto&& inst : m_locationInstances) {
												auto&& loc = inst.second.m_location;
												if ((loc->m_prefabName == location->m_prefabName
													|| (!location->m_group.empty() && location->m_group == loc->m_group))
													&& inst.second.m_position.Distance(randomPointInZone) < location->m_minDistanceFromSimilar)
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

		if (spawnedLocations < location->m_quantity) {
            LOG(ERROR) << "Failed to place all " << location->m_prefabName << ", placed " << spawnedLocations << "/" << location->m_quantity;

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

	void RPC_DiscoverClosestLocation(OWNER_t sender, std::string name, Vector3 point, std::string pinName, int32_t pinType, bool showMap) {
		LocationInstance locationInstance;
		//if (FindClosestLocation(name, point, out locationInstance)) {
		//	ZLog.Log("Found location of type " + name);
		//	ZRoutedRpc.instance.InvokeRoutedRPC(sender, "DiscoverLocationRespons", new object[]
		//		{
		//			pinName,
		//			pinType,
		//			locationInstance.m_position,
		//			showMap
		//		});
		//	return;
		//}
		//ZLog.LogWarning("Failed to find location of type " + name);
	}

	void Init() {

		NetRouteManager::Register(NetHashes::Routed::GetClosestLocation, RPC_DiscoverClosestLocation);

		//auto lines(VUtils::Resource::ReadFileLines("locations.csv"));
		/*
		auto fs(VUtils::Resource::GetInFile("locations.csv"));

		//auto path = VUtils::Resource::GetPath("locations.csv");
		//FILE* f = fopen(path.string().c_str(), "r");
		//std::ifstream fs; // (path); // , std::ios::in);
		//
		//fs.open(path);

		std::string line;
		std::string garbage;
		
		while (std::getline(fs, line)) {
			// now unpack fields
			if (line.empty() || line[0] == '#')
				continue;

			std::istringstream ss(line);

			std::string line1;
			std::getline(ss, line1, ',');
			
			std::underlying_type_t<Heightmap::Biome> biome;
			std::underlying_type_t<Heightmap::BiomeArea> biomeArea;
			//Heightmap::BiomeArea::Median = 2
			ZoneLocation loc;
			ss >> loc.m_enable;
			ss >> loc.m_prefabName;
			ss >> biome;
			ss >> biomeArea;
			ss >> loc.m_quantity;
			ss >> loc.m_chanceToSpawn;
			ss >> garbage; // loc.m_priority;
			ss >> loc.m_centerFirst;
			ss >> loc.m_unique;
			ss >> loc.m_group;
			ss >> loc.m_minDistanceFromSimilar;
			ss >> garbage; ////ss >> loc.m_iconAlways; // 
			ss >> garbage; ////ss >> loc.m_iconPlaced;
			ss >> loc.m_randomRotation;
			ss >> loc.m_slopeRotation;
			ss >> loc.m_snapToWater;
			ss >> loc.m_minTerrainDelta;
			ss >> loc.m_maxTerrainDelta;
			ss >> loc.m_inForest;
			ss >> loc.m_forestTresholdMin;
			ss >> loc.m_forestTresholdMax;
			ss >> loc.m_minDistance;
			ss >> loc.m_maxDistance;
			ss >> loc.m_minAltitude;
			ss >> loc.m_maxAltitude;
		}*/

		// inserts the blank dummy key
		m_globalKeys.insert("");
	}


	void Load(NetPackage& reader, int32_t worldVersion) {
		auto num = reader.Read<int32_t>();
		for (int i = 0; i < num; i++) {
			Vector2i item;
			item.x = reader.Read<int32_t>();
			item.y = reader.Read<int32_t>();
			m_generatedZones.insert(item);
		}

		if (worldVersion >= 13) {
			int num2 = reader.Read<int32_t>();
			int num3 = (worldVersion >= 21) ? reader.Read<int32_t>() : 0;
			if (num2 != VALHEIM_PGW_VERSION)
				m_generatedZones.clear();

			if (worldVersion >= 14) {
				int num4 = reader.Read<int32_t>();
				for (int j = 0; j < num4; j++) {
					auto item2 = reader.Read<std::string>();
					m_globalKeys.insert(item2);
				}
			}

			if (worldVersion >= 18) {
				if (worldVersion >= 20)
					m_locationsGenerated = reader.Read<bool>();

				auto num5 = reader.Read<int32_t>();
				for (int k = 0; k < num5; k++) {
					auto text = reader.Read<std::string>();
					Vector3 zero;
					zero.x = reader.Read<float>();
					zero.y = reader.Read<float>();
					zero.z = reader.Read<float>();
					bool generated = false;
					if (worldVersion >= 19)
						generated = reader.Read<bool>();

					auto location = GetLocation(text);
					if (location)
						RegisterLocation(location, zero, generated);
					else
						LOG(INFO) << "Failed to find location " << text;
				}

				LOG(INFO) << "Loaded " << num5 << " locations";

				if (num2 != VALHEIM_PGW_VERSION) {
					m_locationInstances.clear();
					m_locationsGenerated = false;
				}

				if (num3 != m_locationVersion)
					m_locationsGenerated = false;
			}
		}
	}

}