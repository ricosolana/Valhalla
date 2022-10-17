#pragma once

#include "Utils.h"

// Recursive method
constexpr hash_t __H(const char* str, unsigned num, unsigned num2, unsigned num3) {
	if (str[num3] != '\0') {
		num = ((num << 5) + num) ^ (unsigned)str[num3];
		if (str[num3 + 1] != '\0') {
			num2 = ((num2 << 5) + num2) ^ (unsigned)str[num3 + 1];
			num3 += 2;
			return __H(str, num, num2, num3);
		}
	}
	return num + num2 * 1566083941;
}

// Primer method
constexpr hash_t __H(const char* str) {
	int num = 5381;
	int num2 = num;
	int num3 = 0;
	
	return __H(str, num, num2, num3);
}


// https://github.com/Valheim-Modding/Wiki/wiki/ZDO-Hashes
enum class Hash_ArmorStand : hash_t {
	POSE = __H("pose")
};

enum class Hash_BaseAI : hash_t {
	ALERT = __H("alert"),
	HUNT_PLAYER = __H("huntplayer"),
	PATROL = __H("patrol"),
	PATROL_POINT = __H("patrolPoint"),
	SPAWN_POINT = __H("spawnpoint"),
	SPAWN_TIME = __H("spawntime")
};

enum class Hash_Bed : hash_t {
	OWNER = __H("owner"),
	OWNER_NAME = __H("ownerName"),
};

enum class Hash_Beehive : hash_t {
	ALERT = __H("health"),
	LAST_TIME = __H("lastTime"),
	LEVEL = __H("level"),
	PRODUCT = __H("product"),
};

enum class Hash_Character : hash_t {
	ITEMS_ADDED = __H("addedDefaultItems"),
	BODY_VELOCITY = __H("BodyVelocity"),
	HEALTH = __H("health"),
	LEVEL = __H("level"),
	MAX_HEALTH = __H("max_health"),
	NOISE = __H("noise"),
	TAMED = __H("tamed"),
	TILT_ROT = __H("tiltrot")
};

enum class Hash_CharacterAnimEvent : hash_t {
	LOOK_TARGET = __H("LookTarget"),
};

enum class Hash_Container : hash_t {
	IN_USE = __H("inUse"),
	ITEMS = __H("items"),
};

enum class Hash_CookingStation : hash_t {
	FUEL = __H("fuel"),
	SLOT = __H("slot"), // 2 types
	SLOT_STATUS = __H("slotStatus"),
	START_TIME = __H("StartTime"),
};

enum class Hash_Corpse : hash_t {
	CHEST_ITEM = __H("ChestItem"),
	LEG_ITEM = __H("LegItem"),
	TIME_OF_DEATH = __H("timeOfDeath"),
};

enum class Hash_CreatureSpawner : hash_t {
	ALIVE_TIME = __H("alive_time"),
	SPAWN_ID = __H("spawn_id"),
};

enum class Hash_Destructible : hash_t {
	HEALTH = __H("health"),
};

enum class Hash_Door : hash_t {
	STATE = __H("state"),
};

enum class Hash_DungeonGenerator : hash_t {
	// other partials...

	ROOMS = __H("rooms"),
};

enum class Hash_Fermenter : hash_t {
	CONTENT = __H("Content"),
	START_TIME = __H("StartTime"),
};


enum class Hash_Fireplace : hash_t {
	FUEL = __H("fuel"),
	LAST_TIME = __H("lastTime"),
};

enum class Hash_Fish : hash_t {
	SPAWN_POINT = __H("spawnpoint"),
};

enum class Hash_FishingFloat : hash_t {
	CATCH_ID = __H("CatchID"),
	ROD_OWNER = __H("RodOwner"),
};


enum class Hash_Game : hash_t {
	TARGET = __H("target"),
};

enum class Hash_Gibber : hash_t {
	HIT_DIR = __H("HitDir"),
	HIT_POINT = __H("HitPoint"),
};

//enum class Hash_Humanoid : hash_t {
// is blocking hash
//};

enum class Hash_ItemDrop : hash_t {
	// other partials

	CRAFTER_ID = __H("crafterID"),
	CRAFTER_NAME = __H("crafterName"),
	DURABILITY = __H("durability"),
	QUALITY = __H("quality"),
	SPAWN_TIME = __H("spawntime"),
	STACK = __H("stack"),
	VARIANT = __H("variant"),
};

enum class Hash_ItemStand : hash_t {
	ITEMS = __H("items"),
};

//enum class Hash_LineConnect : hash_t {
//	// partials
//};

enum class Hash_LiquidVolume : hash_t {
	LIQUID_DATA = __H("LiquidData"),
};

enum class Hash_LocationProxy : hash_t {
	LOCATION = __H("location"),
	SEED = __H("seed"),
};

enum class Hash_LootSpawner : hash_t {
	LOCATION = __H("location"),
	SEED = __H("seed"),
};


enum class Hash_MapTable : hash_t {
	DATA = __H("data"),
};

//enum class Hash_MineRock : hash_t {
//	// partials
//	ITEMS_ADDED = __H("Health"),
//};

enum class Hash_MineRock5 : hash_t {
	HEALTH = __H("health"),
};

enum class Hash_MonsterAI : hash_t {
	DESPAWN_IN_DAY = __H("DespawnInDay"),
	EVENT_CREATURE = __H("EventCreature"),
	SLEEPING = __H("sleeping"),
};

enum class Hash_MusicLocation : hash_t {
	PLAYED = __H("played"),
};

enum class Hash_MusicVolume : hash_t {
	PLAYS = __H("plays"),
};

enum class Hash_Character : hash_t {
	PICKED = __H("picked"),
	PICKED_TIME = __H("picked_time"),
};

enum class Hash_PickableItem : hash_t {
	ITEM_PREFAB = __H("itemPrefab"),
	ITEMSTACK = __H("itemStack"),
};

//enum class Hash_Piece : hash_t {
//	// partial
//};

enum class Hash_Plant : hash_t {
	PLANT_TIME = __H("plantTime"),
};

enum class Hash_Player : hash_t {
	BASE_VALUE = __H("baseValue"),
	DEAD = __H("dead"),
	DEBUG_FLY = __H("DebugFly"),
	DODGE_INVULN = __H("dodgeinv"),
	EMOTE = __H("emote"),
	EMOTE_ONESHOT = __H("emote_oneshot"),
	EMOTE_ID = __H("emoteID"),
	IN_BED = __H("inBed"),
	PLAYER_ID = __H("playerID"),
	PLAYER_NAME = __H("playerName"),
	PVP = __H("pvp"),
	STAMINA = __H("stamina"),
	STEALTH = __H("Stealth"),
	WAKEUP = __H("wakeup"),
};

enum class Hash_PrivateArea : hash_t {
	CREATOR_NAME = __H("creatorName"),
	ENABLED = __H("enabled"),
	PERMITTED = __H("permitted"),
	ID = __H("pu_id"),
	NAME = __H("pu_name"),
};

enum class Hash_Procreation : hash_t {
	LOVE_POINTS = __H("lovePoints"),
	PREGNANT = __H("pregnant"),
};


enum class Hash_Ragdoll : hash_t {
	DROP_AMOUNT = __H("drop_amount"),
	DROP_HASH = __H("drop_hash"),
	DROPS = __H("drops"),
	HUE = __H("Hue"),
	INIT_VEL = __H("InitVel"),
	SATURATION = __H("Saturation"),
	VALUE = __H("Value"),
};

//enum class Hash_RandomAnimation : hash_t {
//	// partial
//};

enum class Hash_RandomFlyingBird : hash_t {
	LANDED = __H("landed"),
	SPANW_POINT = __H("spawnpoint"),
};

enum class Hash_Saddle : hash_t {
	// partial
	//ID= __H("-empty"),

	STAMINA = __H("stamina"),
	USER = __H("user"),
};

enum class Hash_StatusEffectManager : hash_t {
	ATTRIBUTE = __H("seAttrib"),
};

enum class Hash_Ship : hash_t {
	FORWARD = __H("forward"),
	RUDDER = __H("rudder"), // 2 types
};

enum class Hash_ShipConstructor : hash_t {
	DONE = __H("done"),
	SPAWN_TIME = __H("spawntime"),
};

enum class Hash_ShipControls : hash_t {
	USER = __H("user"),
};

enum class Hash_Sign : hash_t {
	AUTHOR = __H("author"),
	TEXT = __H("text"),
};

enum class Hash_Smelter : hash_t {
	// partial

	ACCUMULATED_TIME = __H("accTime"),
	BAKE_TIMER = __H("bakeTimer"),
	FUEL = __H("fuel"),
	ITEM = __H("item"),
	QUEUED = __H("queued"),
	SPAWN_AMOUNT = __H("SpawnAmount"),
	SPAWN_ORE = __H("SpawnOre"),
	START_TIME = __H("StartTime"),
};

//enum class Hash_SpawnSystem : hash_t {
//	// partial
//};

enum class Hash_Tameable : hash_t {
	// partial

	NAME = __H("TamedName"),
	TAME_LAST_FEEDING = __H("TameLastFeeding"),
	TAME_TIME_LEFT = __H("TameTimeLeft"),
};


enum class Hash_TeleportWorld : hash_t {
	TAG = __H("tag"),
};


enum class Hash_TerrainComp : hash_t {
	DATA = __H("TCData"),
};


enum class Hash_TombStone : hash_t {
	IN_WATER = __H("inWater"),
	OWNER = __H("owner"),
	OWNER_NAME = __H("ownerName"),
	SPAWN_POINT = __H("spawnpoint"),
	TIME_OF_DEATH = __H("timeOfDeath"),
};

enum class Hash_TreeBase : hash_t {
	HEALTH = __H("health"),
};

enum class Hash_TreeLog : hash_t {
	HEALTH = __H("health"),
};


enum class Hash_VisEquipment : hash_t {
	ITEM_BEARD = __H("BeardItem"),
	ITEM_CHEST = __H("ChestItem"),
	COLOR_HAIR = __H("HairColor"),
	ITEM_HAIR = __H("HairItem"),
	ITEM_HELMET = __H("HelmetItem"),
	ITEM_LEFT_BACK = __H("LeftBackItem"),
	ITEMV_LEFT_BACK = __H("LeftBackItemVariant"),
	ITEM_LEFT = __H("LeftItem"),
	ITEMV_LEFT= __H("LeftItemVariant"),
	ITEM_LEG = __H("LegItem"),
	MODEL_INDEX = __H("ModelIndex"),
	ITEM_RIGHT_BACK = __H("RightBackItem"),
	ITEM_RIGHT = __H("RightItem"),
	ITEM_SHOULDER = __H("ShoulderItem"),
	ITEMV_SHOULDER = __H("ShoulderItemVariant"),
	COLOR_SKIN = __H("SkinColor"),
	ITEM_UTILITY = __H("UtilityItem"),
};


enum class Hash_WearNTear : hash_t {
	HEALTH = __H("health"),
	SUPPORT = __H("support"),
};

enum class Hash_ZNetView : hash_t {
	SCALE = __H("scale"),
};

//enum class Hash_ZSyncAnimation : hash_t {
// partial
//};

//enum class Hash_ZSyncTransform : hash_t {
// partial
//};
