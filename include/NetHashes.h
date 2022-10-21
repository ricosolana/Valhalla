#pragma once

#include "Utils.h"

// Recursive method






/*
* 
* Rpc manager methods
* as of 0.211.9
* 
*/

enum class Rpc_Hash : HASH_t {
	Disconnect = __H("Disconnect"),
	ServerHandshake = __H("ServerHandshake"),
	Save = __H("Save"),
	PrintBanned = __H("PrintBanned"),
	ZDOData = __H("ZDOData"),
	PeerInfo = __H("PeerInfo"),
	Error = __H("Error"),
	PlayerList = __H("PlayerList"),
	RemotePrint = __H("RemotePrint"),
	CharacterID = __H("CharacterID"),
	Kick = __H("Kick"),
	Ban = __H("Ban"),
	Unban = __H("Unban"),
	NetTime = __H("NetTime"),
	RoutedRPC = __H("RoutedRPC"),
	ClientHandshake = __H("ClientHandshake"),
	RefPos = __H("RefPos"),
};

// Chat calls Talker methods
// Chat also calls RPC ChatMessage (ZRoutedRPC)
// Talker is a thin wrapper around Chat send and receiving, which are heavy handled by Chat
//		Talker.Say is ran by Client by Chat.SendText
//		Talker.RPC_Say appears to be used ONLY by client (due to localPlayer null check)

enum class RoutedRpc_Hash : HASH_t {
	SleepStart = __H("SleepStart"),
	SleepStop = __H("SleepStop"),
	DamageText = __H("DamageText"),
	Ping = __H("Ping"),
	Pong = __H("Pong"),
	DestroyZDO = __H("DestroyZDO"),
	RequestZDO = __H("RequestZDO"),
	SetGlobalKey = __H("SetGlobalKey"),
	RemoveGlobalKey = __H("RemoveGlobalKey"),
	GlobalKeys = __H("GlobalKeys"),
	LocationIcons = __H("LocationIcons"),
	ShowMessage = __H("ShowMessage"),
	SetEvent = __H("SetEvent"),
	SpawnObject = __H("SpawnObject"),
	DiscoverLocationCallback = __H("DiscoverLocationRespons"),
	ChatMessage = __H("ChatMessage"),
	GetClosestStructure = __H("DiscoverClosestLocation"),
};



/*
* 
* ZNetView object methods
* 
*/

enum class ArmorStand_RPC : HASH_t {	
	DestroyAttachment = __H("RPC_DestroyAttachment"),
	DropItem = __H("RPC_DropItem"),
	DropItemByName = __H("RPC_DropItemByName"),
	RequestOwn = __H("RPC_RequestOwn"),
	SetPose = __H("RPC_SetPose"),
	SetVisualItem = __H("RPC_SetVisualItem"),
};

enum class BaseAI_RPC : HASH_t {
	Alert = __H("Alert"),
	OnNearProjectileHit = __H("OnNearProjectileHit"),
};

enum class Bed_RPC : HASH_t {
	SetOwner = __H("SetOwner"),
};

enum class Beehive_RPC : HASH_t {
	Extract = __H("Extract"),
};

enum class Character_RPC : HASH_t {
	AddNoise = __H("AddNoise"),
	Damage = __H("Damage"),
	Heal = __H("Heal"),
	ResetCloth = __H("ResetCloth"),
	Teleport = __H("RPC_TeleportTo"),
	SetTamed = __H("SetTamed"),
	Stagger = __H("Stagger"),
};

enum class Container_RPC : HASH_t {
	OpenCallback = __H("OpenRespons"),
	Open = __H("RequestOpen"),
	TakeAll = __H("RequestTakeAll"),
	TakeAllCallback = __H("TakeAllRespons"),
};

enum class CookingStation_RPC : HASH_t {
	AddFuel = __H("AddFuel"),
	AddItem = __H("AddItem"),
	RemoveItem = __H("RemoveDoneItem"),
	SetSlotVisual = __H("SetSlotVisual"),
};

enum class Destructible_RPC : HASH_t {
	CreateFragments = __H("CreateFragments"),
	Damage = __H("Damage"),
};

enum class Door_RPC : HASH_t {
	Use = __H("UseDoor"),
};

enum class Fermenter_RPC : HASH_t {
	Add = __H("AddItem"),
	Take = __H("Tap"),
};

enum class Fireplace_RPC : HASH_t {
	Add = __H("AddFuel"),
};

enum class Fish_RPC : HASH_t {
	PickupCallback = __H("Pickup"),
	Pickup = __H("RequestPickup"),
};

enum class FishingFloat_RPC : HASH_t {
	Nibble = __H("Nibble"),
};

enum class Step_RPC : HASH_t {
	Step = __H("Step"),
};

enum class Incinerator_RPC : HASH_t {
	LeverOn = __H("RPC_AnimateLever"),
	LeverOff = __H("RPC_AnimateLeverReturn"),
	IncinerateCallback = __H("RPC_IncinerateRespons"),
	Incinerate = __H("RPC_RequestIncinerate"),
};

enum class ItemDrop_RPC : HASH_t {
	RequestOwn = __H("RequestOwn"),
};

enum class ItemStand : HASH_t {
	DestroyAttachment = __H("DestroyAttachment"),
	DropItem = __H("DropItem"),
	Own = __H("RequestOwn"),
	SetVisualItem = __H("SetVisualItem"),
};

enum class MapTable_RPC : HASH_t {
	Data = __H("MapData"),
};

enum class MineRock_RPC : HASH_t {
	Hide = __H("Hide"),
	Hit = __H("Hit"),
};

enum class MineRock5_RPC : HASH_t {
	Damage = __H("Damage"),
	SetAreaHealth = __H("SetAreaHealth"),
};

enum class MusicLocation_RPC : HASH_t {
	SetPlayed = __H("SetPlayed"),
};

enum class Pick_RPC : HASH_t {
	Pick = __H("Pick"),
};

enum class Pickable : HASH_t {
	Pick = __H("Pick"),
	SetPicked = __H("SetPicked"),
};

enum class Player_RPC : HASH_t {
	PopupText = __H("Message"), // center text ("THE BEES ARE HAPPY")		top-left text (
	OnDeath = __H("OnDeath"),
	OnTargeted = __H("OnTargeted"),
	UseStamina = __H("UseStamina"),
};

enum class PrivateArea_RPC : HASH_t {
	FlashShield = __H("FlashShield"),
	ToggleEnabled = __H("ToggleEnabled"),
	TogglePermitted = __H("TogglePermitted"),
};

enum class Projectile_RPC : HASH_t {
	OnHit = __H("OnHit"),
};

enum class Saddle : HASH_t {
	Move = __H("Controls"),
	Dismount = __H("ReleaseControl"),
	RemoveSaddle = __H("RemoveSaddle"),
	Mount = __H("RequestControl"),
	MountCallback = __H("RequestRespons"),
};

enum class StatusEffect_RPC : HASH_t {
	Add = __H("AddStatusEffect"),
};

enum class Ship_RPC : HASH_t {
	Backward = __H("Backward"),
	Forward = __H("Forward"),
	Steer = __H("Rudder"),
	Stop = __H("Stop"),
};

enum class ShipController_RPC : HASH_t {
	Disengage = __H("ReleaseControl"),
	Control = __H("RequestControl"),
	ControlCallback = __H("RequestRespons"),
};

enum class Smelter_RPC : HASH_t {
	AddFuel = __H("AddFuel"),
	AddOre = __H("AddOre"),
	Empty = __H("EmptyProcessed"),
};

enum class Talker_RPC : HASH_t {
	Chat = __H("Say"),
};

enum class Tameable_RPC : HASH_t {
	AddSaddle = __H("AddSaddle"),
	Command = __H("Command"),
	SetName = __H("SetName"),
	SetSaddle = __H("SetSaddle"),
};

enum class TeleportWorld_RPC : HASH_t {
	SetTag = __H("SetTag"),
};

enum class TerrainComposer_RPC : HASH_t {
	Apply = __H("ApplyOperation"),
};

enum class TreeBase_RPC : HASH_t {
	Damage = __H("Damage"),
	Grow = __H("Grow"),
	Shake = __H("Shake"),
};

enum class TreeLog_RPC : HASH_t {
	Damage = __H("Damage"),
};

enum class Vagon_RPC : HASH_t {
	DeniedCallback = __H("RequestDenied"),
	Own = __H("RequestOwn"),
};

enum class WearNTear_RPC : HASH_t {
	CreateFragments = __H("WNTCreateFragments"),
	Damage = __H("WNTDamage"),
	HealthChanged = __H("WNTHealthChanged"),
	Remove = __H("WNTRemove"),
	Repair = __H("WNTRepair"),
};

enum class ZSyncAnim_RPC : HASH_t {
	SetTrigger = __H("SetTrigger"),
};

enum class MusicVolume : HASH_t {
	Play = __H("RPC_PlayMusic"),
};



/*
* All ZDO's as of ~0.211.8
*/

// https://github.com/Valheim-Modding/Wiki/wiki/ZDO-Hashes
enum class ArmorStand_ZDO : HASH_t {
	POSE = __H("pose")
};

enum class BaseAI_ZDO : HASH_t {
	ALERT = __H("alert"),
	HUNT_PLAYER = __H("huntplayer"),
	PATROL = __H("patrol"),
	PATROL_POINT = __H("patrolPoint"),
	SPAWN_POINT = __H("spawnpoint"),
	SPAWN_TIME = __H("spawntime")
};

enum class Bed_ZDO : HASH_t {
	OWNER = __H("owner"),
	OWNER_NAME = __H("ownerName"),
};

enum class Beehive_ZDO : HASH_t {
	ALERT = __H("health"),
	LAST_TIME = __H("lastTime"),
	LEVEL = __H("level"),
	PRODUCT = __H("product"),
};

enum class Character_ZDO : HASH_t {
	ITEMS_ADDED = __H("addedDefaultItems"),
	BODY_VELOCITY = __H("BodyVelocity"),
	HEALTH = __H("health"),
	LEVEL = __H("level"),
	MAX_HEALTH = __H("max_health"),
	NOISE = __H("noise"),
	TAMED = __H("tamed"),
	TILT_ROT = __H("tiltrot")
};

enum class CharacterAnimEvent_ZDO : HASH_t {
	LOOK_TARGET = __H("LookTarget"),
};

enum class Container_ZDO : HASH_t {
	IN_USE = __H("inUse"),
	ITEMS = __H("items"),
};

enum class CookingStation_ZDO : HASH_t {
	FUEL = __H("fuel"),
	SLOT = __H("slot"), // 2 types
	SLOT_STATUS = __H("slotStatus"),
	START_TIME = __H("StartTime"),
};

enum class Corpse_ZDO : HASH_t {
	CHEST_ITEM = __H("ChestItem"),
	LEG_ITEM = __H("LegItem"),
	TIME_OF_DEATH = __H("timeOfDeath"),
};

enum class CreatureSpawner_ZDO : HASH_t {
	ALIVE_TIME = __H("alive_time"),
	SPAWN_ID = __H("spawn_id"),
};

enum class Destructible_ZDO : HASH_t {
	HEALTH = __H("health"),
};

enum class Door_ZDO : HASH_t {
	STATE = __H("state"),
};

enum class DungeonGenerator_ZDO : HASH_t {
	// other partials...

	ROOMS = __H("rooms"),
};

enum class Fermenter_ZDO : HASH_t {
	CONTENT = __H("Content"),
	START_TIME = __H("StartTime"),
};

enum class Fireplace_ZDO : HASH_t {
	FUEL = __H("fuel"),
	LAST_TIME = __H("lastTime"),
};

enum class Fish_ZDO : HASH_t {
	SPAWN_POINT = __H("spawnpoint"),
};

enum class FishingFloat_ZDO : HASH_t {
	CATCH_ID = __H("CatchID"),
	ROD_OWNER = __H("RodOwner"),
};

enum class Game_ZDO : HASH_t {
	TARGET = __H("target"),
};

enum class Gibber_ZDO : HASH_t {
	HIT_DIR = __H("HitDir"),
	HIT_POINT = __H("HitPoint"),
};

//enum class Hash_Humanoid : HASH_t {
// is blocking hash
//};

enum class ItemDrop_ZDO : HASH_t {
	// other partials

	CRAFTER_ID = __H("crafterID"),
	CRAFTER_NAME = __H("crafterName"),
	DURABILITY = __H("durability"),
	QUALITY = __H("quality"),
	SPAWN_TIME = __H("spawntime"),
	STACK = __H("stack"),
	VARIANT = __H("variant"),
};

enum class ItemStand_ZDO : HASH_t {
	ITEMS = __H("items"),
};

//enum class Hash_LineConnect : HASH_t {
//	// partials
//};

enum class LiquidVolume_ZDO : HASH_t {
	LIQUID_DATA = __H("LiquidData"),
};

enum class LocationProxy_ZDO : HASH_t {
	LOCATION = __H("location"),
	SEED = __H("seed"),
};

enum class LootSpawner_ZDO : HASH_t {
	LOCATION = __H("location"),
	SEED = __H("seed"),
};

enum class MapTable_ZDO : HASH_t {
	DATA = __H("data"),
};

//enum class Hash_MineRock : HASH_t {
//	// partials
//	ITEMS_ADDED = __H("Health"),
//};

enum class MineRock5_ZDO : HASH_t {
	HEALTH = __H("health"),
};

enum class MonsterAI_ZDO : HASH_t {
	DESPAWN_IN_DAY = __H("DespawnInDay"),
	EVENT_CREATURE = __H("EventCreature"),
	SLEEPING = __H("sleeping"),
};

enum class MusicLocation_ZDO : HASH_t {
	PLAYED = __H("played"),
};

enum class MusicVolume_ZDO : HASH_t {
	PLAYS = __H("plays"),
};

enum class Pickable_ZDO : HASH_t {
	PICKED = __H("picked"),
	PICKED_TIME = __H("picked_time"),
};

enum class PickableItem_ZDO : HASH_t {
	ITEM_PREFAB = __H("itemPrefab"),
	ITEMSTACK = __H("itemStack"),
};

//enum class Hash_Piece : HASH_t {
//	// partial
//};

enum class Plant_ZDO : HASH_t {
	PLANT_TIME = __H("plantTime"),
};

enum class Player_ZDO : HASH_t {
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

enum class PrivateArea_ZDO : HASH_t {
	CREATOR_NAME = __H("creatorName"),
	ENABLED = __H("enabled"),
	PERMITTED = __H("permitted"),
	NetID = __H("pu_id"),
	NAME = __H("pu_name"),
};

enum class Procreation_ZDO : HASH_t {
	LOVE_POINTS = __H("lovePoints"),
	PREGNANT = __H("pregnant"),
};

enum class Ragdoll_ZDO : HASH_t {
	DROP_AMOUNT = __H("drop_amount"),
	DROP_HASH = __H("drop_hash"),
	DROPS = __H("drops"),
	HUE = __H("Hue"),
	INIT_VEL = __H("InitVel"),
	SATURATION = __H("Saturation"),
	VALUE = __H("Value"),
};

//enum class Hash_RandomAnimation : HASH_t {
//	// partial
//};

enum class RandomFlyingBird_ZDO : HASH_t {
	LANDED = __H("landed"),
	SPAWN_POINT = __H("spawnpoint"),
};

enum class Saddle_ZDO : HASH_t {
	// partial
	//NetID= __H("-empty"),

	STAMINA = __H("stamina"),
	USER = __H("user"),
};

enum class StatusEffectManager_ZDO : HASH_t {
	ATTRIBUTE = __H("seAttrib"),
};

enum class Ship_ZDO : HASH_t {
	FORWARD = __H("forward"),
	RUDDER = __H("rudder"), // 2 types
};

enum class ShipConstructor_ZDO : HASH_t {
	DONE = __H("done"),
	SPAWN_TIME = __H("spawntime"),
};

enum class ShipControls_ZDO : HASH_t {
	USER = __H("user"),
};

enum class Sign_ZDO : HASH_t {
	AUTHOR = __H("author"),
	TEXT = __H("text"),
};

enum class Smelter_ZDO : HASH_t {
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

//enum class Hash_SpawnSystem : HASH_t {
//	// partial
//};

enum class Tameable_ZDO : HASH_t {
	// partial

	NAME = __H("TamedName"),
	TAME_LAST_FEEDING = __H("TameLastFeeding"),
	TAME_TIME_LEFT = __H("TameTimeLeft"),
};

enum class TeleportWorld_ZDO : HASH_t {
	TAG = __H("tag"),
};

enum class TerrainComp_ZDO : HASH_t {
	DATA = __H("TCData"),
};

enum class TombStone_ZDO : HASH_t {
	IN_WATER = __H("inWater"),
	OWNER = __H("owner"),
	OWNER_NAME = __H("ownerName"),
	SPAWN_POINT = __H("spawnpoint"),
	TIME_OF_DEATH = __H("timeOfDeath"),
};

enum class TreeBase_ZDO : HASH_t {
	HEALTH = __H("health"),
};

enum class TreeLog_ZDO : HASH_t {
	HEALTH = __H("health"),
};

enum class VisEquipment_ZDO : HASH_t {
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

enum class WearNTear_ZDO : HASH_t {
	HEALTH = __H("health"),
	SUPPORT = __H("support"),
};

enum class ZNetView_ZDO : HASH_t {
	SCALE = __H("scale"),
};

//enum class Hash_ZSyncAnimation : HASH_t {
// partial
//};

//enum class Hash_ZSyncTransform : HASH_t {
// partial
//};
