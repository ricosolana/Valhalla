#pragma once

#include "Types.h"
#include "VUtilsString.h"


/*
* 
* Rpc manager methods
* as of 0.211.9
* 
*/

namespace avledet::util::hashes {

    namespace Rpc {
        //
        // Clientbound
        //

        // Once
        static constexpr Hash S2C_Handshake = __H("ClientHandshake");
        static constexpr Hash S2C_Error = __H("Error");
        static constexpr Hash S2C_ResponseKicked = __H("Kicked");

        static constexpr Hash S2C_UpdateTime = __H("NetTime");
        static constexpr Hash S2C_UpdatePlayerList = __H("PlayerList");
        static constexpr Hash S2C_RequestSave = __H("SavePlayerProfile"); // dedicated server registers this method, why?



        //
        // Serverbound
        //

        // Once
        static constexpr Hash C2S_Handshake = __H("ServerHandshake");

        // Permissions
        static constexpr Hash C2S_RequestSave = __H("Save");
        static constexpr Hash C2S_RequestBanList = __H("PrintBanned");
        static constexpr Hash C2S_RequestKick = __H("Kick");
        static constexpr Hash C2S_RequestBan = __H("Ban");
        static constexpr Hash C2S_RequestUnban = __H("Unban");

        // Regularly called
        static constexpr Hash C2S_UpdateID = __H("CharacterID");
        static constexpr Hash C2S_PlayerData = __H("ServerSyncedPlayerData"); // dedicated server registers this method, why?
        static constexpr Hash C2S_RemoteCommand = __H("RPC_RemoteCommand");



        //
        // Client + Server
        //

        static constexpr Hash PeerInfo = __H("PeerInfo");
        static constexpr Hash Disconnect = __H("Disconnect");
        static constexpr Hash ZDOData = __H("ZDOData");
        static constexpr Hash ConsoleMessage = __H("RemotePrint");
        static constexpr Hash RoutedRPC = __H("RoutedRPC");
    }

    // Chat calls Talker methods
    // Chat also calls RPC ChatMessage (ZRoutedRPC)
    // Talker is a thin wrapper around Chat send and receiving, which are heavy handled by Chat
    //        Talker.Say is ran by Client by Chat.SendText
    //        Talker.RPC_Say appears to be used ONLY by client (due to localPlayer null check)

    // Talker gets sent by client to client
    // Server is only a middle man in this scenario, 
    // with no concern about the packet contents
    //    only the sender and receiver

    namespace Routed {

        //
        // Client
        // 

        // Meter
        static constexpr Hash S2C_RequestSleep = __H("SleepStart");
        static constexpr Hash S2C_RequestStopSleep = __H("SleepStop");
        static constexpr Hash DamageText = __H("DamageText");
        static constexpr Hash S2C_ResponsePing = __H("Ping");
        static constexpr Hash S2C_UpdateKeys = __H("GlobalKeys");
        static constexpr Hash S2C_UpdateIcons = __H("LocationIcons");
        static constexpr Hash S2C_UIMessage = __H("ShowMessage");
        static constexpr Hash S2C_SetEvent = __H("SetEvent");
        static constexpr Hash S2C_ResponseIcon = __H("RPC_DiscoverLocationResponse");
        static constexpr Hash S2C_RequestTeleport = __H("RPC_TeleportPlayer");



        // 
        // Client + Server
        //

        // Regularly called
        static constexpr Hash DestroyZDO = __H("DestroyZDO");
        static constexpr Hash C2S_RequestZDO = __H("RequestZDO");
        static constexpr Hash SpawnObject = __H("SpawnObject");

        // Permissions / Meter / Gamestate
        static constexpr Hash C2S_SetGlobalKey = __H("SetGlobalKey");
        static constexpr Hash C2S_RemoveGlobalKey = __H("RemoveGlobalKey");
        static constexpr Hash Pong = __H("Pong");
        static constexpr Hash ChatMessage = __H("ChatMessage");



        //
        // Server
        //

        static constexpr Hash C2S_RequestIcon = __H("RPC_DiscoverClosestLocation");
    }



    namespace View {
        /*
        *
        * ZNetView object methods
        *
        */

        namespace ArmorStand {
            static constexpr Hash DestroyAttachment = __H("RPC_DestroyAttachment");
            static constexpr Hash DropItem = __H("RPC_DropItem");
            static constexpr Hash DropItemByName = __H("RPC_DropItemByName");
            static constexpr Hash RequestOwn = __H("RPC_RequestOwn");
            static constexpr Hash SetPose = __H("RPC_SetPose");
            static constexpr Hash SetVisualItem = __H("RPC_SetVisualItem");
        };

        namespace BaseAI {
            static constexpr Hash Alert = __H("Alert");
            static constexpr Hash OnNearProjectileHit = __H("OnNearProjectileHit");
        };

        namespace Bed {
            static constexpr Hash SetOwner = __H("SetOwner");
        };

        namespace Beehive {
            static constexpr Hash Extract = __H("Extract");
        };

        namespace Character {
            static constexpr Hash AddNoise = __H("AddNoise");
            static constexpr Hash Damage = __H("Damage");
            static constexpr Hash Heal = __H("Heal");
            static constexpr Hash ResetCloth = __H("ResetCloth");
            static constexpr Hash Teleport = __H("RPC_TeleportTo");
            static constexpr Hash SetTamed = __H("SetTamed");
            static constexpr Hash Stagger = __H("Stagger");
        };

        namespace Container {
            static constexpr Hash OpenCallback = __H("OpenRespons");
            static constexpr Hash Open = __H("RequestOpen");
            static constexpr Hash TakeAll = __H("RequestTakeAll");
            static constexpr Hash TakeAllCallback = __H("TakeAllRespons");
            static constexpr Hash RequestStack = __H("RPC_RequestStack");
            static constexpr Hash RequestStackCallback = __H("RPC_StackResponse");
        };

        namespace CookingStation {
            static constexpr Hash AddFuel = __H("AddFuel");
            static constexpr Hash AddItem = __H("AddItem");
            static constexpr Hash RemoveItem = __H("RemoveDoneItem");
            static constexpr Hash SetSlotVisual = __H("SetSlotVisual");
        };

        namespace Destructible {
            static constexpr Hash CreateFragments = __H("CreateFragments");
            static constexpr Hash Damage = __H("Damage");
        };

        namespace Door {
            static constexpr Hash Use = __H("UseDoor");
        };

        namespace Fermenter {
            static constexpr Hash Add = __H("AddItem");
            static constexpr Hash Take = __H("Tap");
        };

        namespace Fireplace {
            static constexpr Hash Add = __H("AddFuel");
        };

        namespace Fish {
            static constexpr Hash PickupCallback = __H("Pickup");
            static constexpr Hash Pickup = __H("RequestPickup");
        };

        namespace FishingFloat {
            static constexpr Hash Nibble = __H("Nibble");
        };

        namespace Step {
            static constexpr Hash Step = __H("Step");
        };

        namespace Incinerator {
            static constexpr Hash LeverOn = __H("RPC_AnimateLever");
            static constexpr Hash LeverOff = __H("RPC_AnimateLeverReturn");
            static constexpr Hash IncinerateCallback = __H("RPC_IncinerateRespons");
            static constexpr Hash Incinerate = __H("RPC_RequestIncinerate");
        };

        namespace ItemDrop {
            static constexpr Hash RequestOwn = __H("RequestOwn");
        };

        namespace ItemStand {
            static constexpr Hash DestroyAttachment = __H("DestroyAttachment");
            static constexpr Hash DropItem = __H("DropItem");
            static constexpr Hash Own = __H("RequestOwn");
            static constexpr Hash SetVisualItem = __H("SetVisualItem");
        };

        namespace MapTable {
            static constexpr Hash Data = __H("MapData");
        };

        namespace MineRock {
            static constexpr Hash Hide = __H("Hide");
            static constexpr Hash Hit = __H("Hit");
        };

        namespace MineRock5 {
            static constexpr Hash Damage = __H("Damage");
            static constexpr Hash SetAreaHealth = __H("SetAreaHealth");
        };

        namespace MusicLocation {
            static constexpr Hash SetPlayed = __H("SetPlayed");
        };

        namespace MusicVolume {
            static constexpr Hash Play = __H("RPC_PlayMusic");
        };
        
        namespace Pick {
            static constexpr Hash Pick = __H("Pick");
        };

        namespace Pickable {
            static constexpr Hash Pick = __H("Pick");
            static constexpr Hash SetPicked = __H("SetPicked");
        };

        namespace Player {
            static constexpr Hash PopupText = __H("Message");
            static constexpr Hash OnDeath = __H("OnDeath");
            static constexpr Hash OnTargeted = __H("OnTargeted");
            static constexpr Hash UseStamina = __H("UseStamina");
        };

        namespace PrivateArea {
            static constexpr Hash FlashShield = __H("FlashShield");
            static constexpr Hash ToggleEnabled = __H("ToggleEnabled");
            static constexpr Hash TogglePermitted = __H("TogglePermitted");
        };

        namespace Projectile {
            static constexpr Hash OnHit = __H("OnHit");
        };

        namespace Saddle {
            static constexpr Hash Move = __H("Controls");
            static constexpr Hash Dismount = __H("ReleaseControl");
            static constexpr Hash RemoveSaddle = __H("RemoveSaddle");
            static constexpr Hash Mount = __H("RequestControl");
            static constexpr Hash MountCallback = __H("RequestRespons");
        };

        namespace StatusEffect {
            static constexpr Hash Add = __H("AddStatusEffect");
        };

        namespace Ship {
            static constexpr Hash Backward = __H("Backward");
            static constexpr Hash Forward = __H("Forward");
            static constexpr Hash Steer = __H("Rudder");
            static constexpr Hash Stop = __H("Stop");
        };

        namespace ShipController {
            static constexpr Hash Disengage = __H("ReleaseControl");
            static constexpr Hash Control = __H("RequestControl");
            static constexpr Hash ControlCallback = __H("RequestRespons");
        };

        namespace Smelter {
            static constexpr Hash AddFuel = __H("AddFuel");
            static constexpr Hash AddOre = __H("AddOre");
            static constexpr Hash Empty = __H("EmptyProcessed");
        };

        namespace Talker {
            static constexpr Hash Chat = __H("Say");
        };

        namespace Tameable {
            static constexpr Hash AddSaddle = __H("AddSaddle");
            static constexpr Hash Command = __H("Command");
            static constexpr Hash SetName = __H("SetName");
            static constexpr Hash SetSaddle = __H("SetSaddle");
        };

        namespace TeleportWorld {
            static constexpr Hash SetTag = __H("SetTag");
        };

        namespace TerrainComposer {
            static constexpr Hash Apply = __H("ApplyOperation");
        };

        namespace TreeBase {
            static constexpr Hash Damage = __H("Damage");
            static constexpr Hash Grow = __H("Grow");
            static constexpr Hash Shake = __H("Shake");
        };

        namespace TreeLog {
            static constexpr Hash Damage = __H("Damage");
        };

        namespace Vagon {
            static constexpr Hash DeniedCallback = __H("RequestDenied");
            static constexpr Hash Own = __H("RequestOwn");
        };

        namespace WearNTear {
            static constexpr Hash CreateFragments = __H("WNTCreateFragments");
            static constexpr Hash Damage = __H("WNTDamage");
            static constexpr Hash HealthChanged = __H("WNTHealthChanged");
            static constexpr Hash Remove = __H("WNTRemove");
            static constexpr Hash Repair = __H("WNTRepair");
        };

        namespace ZSyncAnim {
            static constexpr Hash SetTrigger = __H("SetTrigger");
        };

    } // namespace View

    namespace ZDO {

        /*
        * All ZDO's as of ~0.211.8
        */

        // https://github.com/Valheim-Modding/Wiki/wiki/ZDO-Hashes
        namespace ArmorStand {
            static constexpr Hash POSE = __H("pose");
        };

        namespace BaseAI {
            static constexpr Hash ALERT = __H("alert");
            static constexpr Hash HUNT_PLAYER = __H("huntplayer");
            static constexpr Hash PATROL = __H("patrol");
            static constexpr Hash PATROL_POINT = __H("patrolPoint");
            static constexpr Hash SPAWN_POINT = __H("spawnpoint");
            static constexpr Hash SPAWN_TIME = __H("spawntime");
        };

        namespace Bed {
            static constexpr Hash OWNER = __H("owner");
            static constexpr Hash OWNER_NAME = __H("ownerName");
        };

        namespace Beehive {
            static constexpr Hash ALERT = __H("health");
            static constexpr Hash LAST_TIME = __H("lastTime");
            static constexpr Hash LEVEL = __H("level");
            static constexpr Hash PRODUCT = __H("product");
        };

        namespace Character {
            static constexpr Hash ITEMS_ADDED = __H("addedDefaultItems");
            static constexpr Hash BODY_VELOCITY = __H("BodyVelocity");
            static constexpr Hash HEALTH = __H("health");
            static constexpr Hash LEVEL = __H("level");
            static constexpr Hash MAX_HEALTH = __H("max_health");
            static constexpr Hash NOISE = __H("noise");
            static constexpr Hash TAMED = __H("tamed");
            static constexpr Hash TILT_ROT = __H("tiltrot");
            static constexpr Hash BOSS_COUNT = __H("bosscount");
        };

        namespace CharacterAnimEvent {
            static constexpr Hash LOOK_TARGET = __H("LookTarget");
        };

        namespace Container {
            static constexpr Hash IN_USE = __H("InUse");
            static constexpr Hash ITEMS = __H("items");
        };

        namespace CookingStation {
            static constexpr Hash FUEL = __H("fuel");
            //static constexpr Hash SLOT = __H("slot"); // 2 types
            //static constexpr Hash SLOT_STATUS = __H("slotStatus");

            static constexpr Hash START_TIME = __H("StartTime");
        };

        namespace Corpse {
            static constexpr Hash CHEST_ITEM = __H("ChestItem");
            static constexpr Hash LEG_ITEM = __H("LegItem");
            static constexpr Hash TIME_OF_DEATH = __H("timeOfDeath");
        };

        namespace CreatureSpawner {
            static constexpr Hash ALIVE_TIME = __H("alive_time");
            //static constexpr Hash SPAWN_ID = __H("spawn_id");
        };

        namespace Destructible {
            static constexpr Hash HEALTH = __H("health");
        };

        namespace Door {
            static constexpr Hash STATE = __H("state");
        };

        namespace DungeonGenerator {
            // index hashes...

            static constexpr Hash ROOMS = __H("rooms");
            static constexpr Hash ROOM_DATA = __H("roomData");
        };

        namespace Fermenter {
            static constexpr Hash CONTENT = __H("Content");
            static constexpr Hash START_TIME = __H("StartTime");
        };

        namespace Fireplace {
            static constexpr Hash FUEL = __H("fuel");
            static constexpr Hash LAST_TIME = __H("lastTime");
        };

        namespace Fish {
            static constexpr Hash SPAWN_POINT = __H("spawnpoint");
        };

        namespace FishingFloat {
            static constexpr Hash CATCH_ID = __H("CatchID");
            static constexpr Hash ROD_OWNER = __H("rodOwner");
        };

        //namespace Game {
        //    static constexpr Hash TARGET = __H("target");
        //};

        namespace Gibber {
            static constexpr Hash HIT_DIR = __H("HitDir");
            static constexpr Hash HIT_POINT = __H("HitPoint");
        };

        namespace Humanoid {
            static constexpr Hash IS_BLOCKING = __H("IsBlocking");
        };

        namespace ItemDrop {
            // index hashes...

            static constexpr Hash CRAFTER_ID = __H("crafterID");
            static constexpr Hash CRAFTER_NAME = __H("crafterName");
            static constexpr Hash DURABILITY = __H("durability");
            static constexpr Hash QUALITY = __H("quality");
            static constexpr Hash SPAWN_TIME = __H("spawntime");
            static constexpr Hash STACK = __H("stack");
            static constexpr Hash VARIANT = __H("variant");
            static constexpr Hash WORLD_LEVEL = __H("worldLevel");
            static constexpr Hash PICKED_UP = __H("pickedUp");
        };

        namespace ItemStand {
            static constexpr Hash ITEMS = __H("items");
        };

        namespace LineConnect {
            static constexpr Hash LINE_PEER = __H("line_peer");
            static constexpr Hash LINE_SLACK = __H("line_slack");
        };

        namespace LiquidVolume {
            static constexpr Hash LIQUID_DATA = __H("LiquidData");
        };

        namespace LocationProxy {
            static constexpr Hash LOCATION = __H("location");
            static constexpr Hash SEED = __H("seed");
        };

        namespace LootSpawner {
            static constexpr Hash LOCATION = __H("location");
            static constexpr Hash SEED = __H("seed");
        };

        namespace MapTable {
            static constexpr Hash DATA = __H("data");
        };

        //namespace Hash_MineRock {
        //    // partials
        //    static constexpr Hash ITEMS_ADDED = __H("Health");
        //};

        namespace MineRock5 {
            static constexpr Hash HEALTH = __H("health");
        };

        namespace MonsterAI {
            static constexpr Hash DESPAWN_IN_DAY = __H("DespawnInDay");
            static constexpr Hash EVENT_CREATURE = __H("EventCreature");
            static constexpr Hash SLEEPING = __H("sleeping");
        };

        namespace MusicLocation {
            static constexpr Hash PLAYED = __H("played");
        };

        namespace MusicVolume {
            static constexpr Hash PLAYS = __H("plays");
        };

        namespace Pickable {
            static constexpr Hash PICKED = __H("picked");
            static constexpr Hash PICKED_TIME = __H("picked_time");
        };

        namespace PickableItem {
            static constexpr Hash ITEM_PREFAB = __H("itemPrefab");
            static constexpr Hash ITEMSTACK = __H("itemStack");
        };

        namespace Piece {
            static constexpr Hash CREATOR = __H("creator");
        };

        namespace Plant {
            static constexpr Hash PLANT_TIME = __H("plantTime");
        };

        namespace Player {
            static constexpr Hash BASE_VALUE = __H("baseValue");
            static constexpr Hash DEAD = __H("dead");
            static constexpr Hash DEBUG_FLY = __H("DebugFly");
            static constexpr Hash DODGE_INVULN = __H("dodgeinv");
            static constexpr Hash EMOTE = __H("emote");
            static constexpr Hash EMOTE_ONESHOT = __H("emote_oneshot");
            static constexpr Hash EMOTE_ID = __H("emoteID");
            static constexpr Hash IN_BED = __H("inBed");
            static constexpr Hash PLAYER_ID = __H("playerID");
            static constexpr Hash PLAYER_NAME = __H("playerName");
            static constexpr Hash PVP = __H("pvp");
            static constexpr Hash STAMINA = __H("stamina");
            static constexpr Hash STEALTH = __H("Stealth");
            static constexpr Hash WAKEUP = __H("wakeup");
        };

        namespace PrivateArea {
            static constexpr Hash CREATOR = __H("creatorName");
            static constexpr Hash ENABLED = __H("enabled");
            static constexpr Hash PERMITTED_COUNT = __H("permitted");
            //static constexpr Hash ID = __H("pu_id");
            //static constexpr Hash NAME = __H("pu_name");
        };

        namespace Procreation {
            static constexpr Hash LOVE_POINTS = __H("lovePoints");
            static constexpr Hash PREGNANT = __H("pregnant");
        };

        namespace Ragdoll {
            //static constexpr Hash DROP_AMOUNT = __H("drop_amount");
            //static constexpr Hash DROP_HASH = __H("drop_hash");

            static constexpr Hash DROPS = __H("drops");
            static constexpr Hash HUE = __H("Hue");
            static constexpr Hash INIT_VEL = __H("InitVel");
            static constexpr Hash SATURATION = __H("Saturation");
            static constexpr Hash VALUE = __H("Value");
        };

        //namespace Hash_RandomAnimation {
        //    // partial
        //};

        namespace RandomFlyingBird {
            static constexpr Hash LANDED = __H("landed");
            static constexpr Hash SPAWN_POINT = __H("spawnpoint");
        };

        namespace Saddle {
            // partial
            //ZDOID= __H("-empty");

            static constexpr Hash STAMINA = __H("stamina");
            //static constexpr Hash USER = __H("user");
        };

        namespace StatusEffectManager {
            static constexpr Hash ATTRIBUTE = __H("seAttrib");
        };

        namespace Ship {
            static constexpr Hash FORWARD = __H("forward");
            static constexpr Hash RUDDER = __H("rudder"); // 2 types
        };

        namespace ShipConstructor {
            static constexpr Hash DONE = __H("done");
            static constexpr Hash SPAWN_TIME = __H("spawntime");
        };

        namespace ShipControls {
            //static constexpr Hash USER = __H("user");
        };

        // ShipControls
        // Saddle
        static constexpr Hash USER = __H("user");

        namespace Sign {
            static constexpr Hash AUTHOR = __H("author");
            static constexpr Hash TEXT = __H("text");
        };

        namespace Smelter {
            // partial

            static constexpr Hash ACCUMULATED_TIME = __H("accTime");
            static constexpr Hash BAKE_TIMER = __H("bakeTimer");
            static constexpr Hash FUEL = __H("fuel");
            static constexpr Hash ITEM = __H("item");
            static constexpr Hash QUEUED = __H("queued");
            static constexpr Hash SPAWN_AMOUNT = __H("SpawnAmount");
            static constexpr Hash SPAWN_ORE = __H("SpawnOre");
            static constexpr Hash START_TIME = __H("StartTime");
        };

        //namespace Hash_SpawnSystem {
        //    // partial
        //};

        namespace Tameable {
            // partial

            static constexpr Hash FOLLOW = __H("follow");
            static constexpr Hash NAME = __H("TamedName");
            static constexpr Hash SADDLED = __H("HaveSaddle");
            static constexpr Hash TAME_LAST_FEEDING = __H("TameLastFeeding");
            static constexpr Hash TAME_TIME_LEFT = __H("TameTimeLeft");
            static constexpr Hash AUTHOR = __H("TamedNameAuthor");
        };

        namespace TeleportWorld {
            static constexpr Hash AUTHOR = __H("tagauthor");
            static constexpr Hash TAG = __H("tag");
            //static constexpr std::pair<Hash, Hash> TARGET = 
            //static constexpr Hash TARGET = __H("target");
        };

        namespace TerrainComp {
            static constexpr Hash DATA = __H("TCData");
        };

        namespace TerrainModifier {
            static constexpr Hash TIME_CREATED = __H("terrainModifierTimeCreated");
        }

        namespace TombStone {
            static constexpr Hash IN_WATER = __H("inWater");
            static constexpr Hash OWNER = __H("owner");
            static constexpr Hash OWNER_NAME = __H("ownerName");
            static constexpr Hash SPAWN_POINT = __H("SpawnPoint");
            static constexpr Hash TIME_OF_DEATH = __H("timeOfDeath");
        };

        namespace Trap {
            static constexpr Hash STATE = __H("state");
            static constexpr Hash TRIGGERED_TIME = __H("triggered");
        }

        namespace TreeBase {
            static constexpr Hash HEALTH = __H("health");
        };

        namespace TreeLog {
            static constexpr Hash HEALTH = __H("health");
        };

        namespace TriggerSpawner {
            static constexpr Hash SPAWN_TIME = __H("spawn_time");
        };

        namespace Turret {
            static constexpr Hash AMMO = __H("ammo");
            static constexpr Hash AMMO_TYPE = __H("ammoType");
            static constexpr Hash LAST_ATTACK = __H("lastAttack");
            static constexpr Hash TARGET = __H("target");
            static constexpr Hash TARGETS = __H("targets");
        };

        namespace VisEquipment {
            static constexpr Hash ITEM_BEARD = __H("BeardItem");
            static constexpr Hash ITEM_CHEST = __H("ChestItem");
            static constexpr Hash COLOR_HAIR = __H("HairColor");
            static constexpr Hash ITEM_HAIR = __H("HairItem");
            static constexpr Hash ITEM_HELMET = __H("HelmetItem");
            static constexpr Hash ITEM_LEFT_BACK = __H("LeftBackItem");
            static constexpr Hash ITEMV_LEFT_BACK = __H("LeftBackItemVariant");
            static constexpr Hash ITEM_LEFT = __H("LeftItem");
            static constexpr Hash ITEMV_LEFT = __H("LeftItemVariant");
            static constexpr Hash ITEM_LEG = __H("LegItem");
            static constexpr Hash MODEL_INDEX = __H("ModelIndex");
            static constexpr Hash ITEM_RIGHT_BACK = __H("RightBackItem");
            static constexpr Hash ITEM_RIGHT = __H("RightItem");
            static constexpr Hash ITEM_SHOULDER = __H("ShoulderItem");
            static constexpr Hash ITEMV_SHOULDER = __H("ShoulderItemVariant");
            static constexpr Hash COLOR_SKIN = __H("SkinColor");
            static constexpr Hash ITEM_UTILITY = __H("UtilityItem");
        };

        namespace WearNTear {
            static constexpr Hash HEALTH = __H("health");
            static constexpr Hash SUPPORT = __H("support");
        };

        namespace WispSpawner {
            static constexpr Hash LAST_SPAWN = __H("LastSpawn");
        };

        namespace ZDO {
            static constexpr Hash PREFAB = __H("prefab");
        }

        namespace ZNetView {
            static constexpr Hash SCALE = __H("scale");
            static constexpr Hash SCALE_SCALAR = __H("scaleScalar");
            static constexpr Hash HAS_FIELDS = __H("HasFields");
        }

        namespace ZoneManager {
            static constexpr Hash LOCATION = __H("location");
            static constexpr Hash SEED = __H("seed");
        };

        namespace ZSyncAnimation {
            static constexpr Hash ALERT = 402913258;
            static constexpr Hash ANIM_SPEED = 1477933170;
            static constexpr Hash ATTACH_BED = -663230492;
            static constexpr Hash ATTACH_CHAIR = 1252953157;
            static constexpr Hash BLOCKING = 524979580;
            static constexpr Hash CRAFTING = 513754048;
            static constexpr Hash CROUCHING = 70500473;
            static constexpr Hash ENCUMBERED = -23605810;
            static constexpr Hash EQUIPPING = -50949212;
            static constexpr Hash FLAPPING = 23215934;
            static constexpr Hash FLYING = 1245391852;
            static constexpr Hash FORWARD_SPEED = -1489121593;
            static constexpr Hash INTRO = 437024329;
            static constexpr Hash IN_WATER = 1299391946;
            static constexpr Hash ON_GROUND = -1493684636;
            static constexpr Hash SIDEWAY_SPEED = -1344031744;
            static constexpr Hash SLEEPING = 110664935;
            static constexpr Hash STATE_F = 1546011855;
            static constexpr Hash STATE_I = -861454496;
            static constexpr Hash TURN_SPEED = -1488745797;
        };

        namespace ZSyncTransform {
            // zdoid hashpair
            //static constexpr Hash PARENT_ID =
        }
    }

    namespace Object {
        static constexpr Hash _eventzone_boss_base = -40120423;
        static constexpr Hash _TerrainCompiler = -367065113;
        static constexpr Hash _ZoneCtrl = 1703108136;
        static constexpr Hash Abomination = 2095123679;
        static constexpr Hash Abomination_attack1 = 1382552281;
        static constexpr Hash Abomination_attack2 = 1382552278;
        static constexpr Hash Abomination_attack3 = 1382552279;
        static constexpr Hash Abomination_ragdoll = -1559908641;
        static constexpr Hash Acorn = -2047937207;
        static constexpr Hash Amber = -1570747403;
        static constexpr Hash AmberPearl = -1844493069;
        static constexpr Hash ancient_skull = 81702400;
        static constexpr Hash ancientbarkspear_projectile = -588675353;
        static constexpr Hash AncientSeed = -500516295;
        static constexpr Hash aoe_nova = 391153472;
        static constexpr Hash arbalest_projectile_blackmetal = 1310585661;
        static constexpr Hash arbalest_projectile_bone = 1463749101;
        static constexpr Hash arbalest_projectile_carapace = 2053765929;
        static constexpr Hash ArmorBronzeChest = -524840022;
        static constexpr Hash ArmorBronzeLegs = -1070975544;
        static constexpr Hash ArmorCarapaceChest = -1330578868;
        static constexpr Hash ArmorCarapaceLegs = -1276324482;
        static constexpr Hash ArmorFenringChest = 349244329;
        static constexpr Hash ArmorFenringLegs = 1068454321;
        static constexpr Hash ArmorIronChest = -77418440;
        static constexpr Hash ArmorIronLegs = 855433562;
        static constexpr Hash ArmorLeatherChest = 1842977441;
        static constexpr Hash ArmorLeatherLegs = -129361023;
        static constexpr Hash ArmorMageChest = 916504644;
        static constexpr Hash ArmorMageLegs = -490399162;
        static constexpr Hash ArmorPaddedCuirass = -2102914493;
        static constexpr Hash ArmorPaddedGreaves = -1316976302;
        static constexpr Hash ArmorRagsChest = -1873790835;
        static constexpr Hash ArmorRagsLegs = -1807008579;
        static constexpr Hash ArmorRootChest = 973754414;
        static constexpr Hash ArmorRootLegs = 1839036508;
        static constexpr Hash ArmorStand = 1580161127;
        static constexpr Hash ArmorStand_Female = 826840908;
        static constexpr Hash ArmorStand_Male = 639316453;
        static constexpr Hash ArmorTrollLeatherChest = -1722809642;
        static constexpr Hash ArmorTrollLeatherLegs = -560834156;
        static constexpr Hash ArmorWolfChest = -914594978;
        static constexpr Hash ArmorWolfLegs = -1695215220;
        static constexpr Hash ArrowBronze = 2137518083;
        static constexpr Hash ArrowCarapace = -485682777;
        static constexpr Hash ArrowFire = -1917784535;
        static constexpr Hash ArrowFlint = -843447246;
        static constexpr Hash ArrowFrost = -641933831;
        static constexpr Hash ArrowIron = -1003810285;
        static constexpr Hash ArrowNeedle = 1981231808;
        static constexpr Hash ArrowObsidian = 343761714;
        static constexpr Hash ArrowPoison = -1215929287;
        static constexpr Hash ArrowSilver = 799199670;
        static constexpr Hash ArrowWood = -782094582;
        static constexpr Hash AtgeirBlackmetal = 275694258;
        static constexpr Hash AtgeirBronze = -971799304;
        static constexpr Hash AtgeirHimminAfl = -1297235939;
        static constexpr Hash AtgeirIron = -1697777810;
        static constexpr Hash AxeBlackMetal = 1694548656;
        static constexpr Hash AxeBronze = -533689078;
        static constexpr Hash AxeFlint = -1468314591;
        static constexpr Hash AxeIron = 1790496580;
        static constexpr Hash AxeJotunBane = 1583975460;
        static constexpr Hash AxeStone = -1779108881;
        static constexpr Hash Barley = 294912805;
        static constexpr Hash BarleyFlour = -1702506509;
        static constexpr Hash BarleyWine = -1380373382;
        static constexpr Hash BarleyWineBase = 113986677;
        static constexpr Hash barrell = 2034747966;
        static constexpr Hash Bat = 696030471;
        static constexpr Hash bat_melee = -1913972066;
        static constexpr Hash Battleaxe = 1943723324;
        static constexpr Hash BattleaxeCrystal = -1994118106;
        static constexpr Hash bed = -1273339005;
        static constexpr Hash bee_aoe = -1791758602;
        static constexpr Hash beech_log = 837895808;
        static constexpr Hash beech_log_half = -624406308;
        static constexpr Hash Beech_Sapling = 1170585794;
        static constexpr Hash Beech_small1 = 865557360;
        static constexpr Hash Beech_small2 = 1268841887;
        static constexpr Hash Beech_Stub = 747762302;
        static constexpr Hash Beech1 = -493262268;
        static constexpr Hash BeechSeeds = 1912302441;
        static constexpr Hash Beehive = 185590564;
        static constexpr Hash BeltStrength = -460902046;
        static constexpr Hash Bilebag = -111754826;
        static constexpr Hash bilebomb_explosion = 976604240;
        static constexpr Hash bilebomb_projectile = -1458915510;
        static constexpr Hash Birch_log = -1411911791;
        static constexpr Hash Birch_log_half = 1868061717;
        static constexpr Hash Birch_Sapling = -646762577;
        static constexpr Hash Birch1 = -427486029;
        static constexpr Hash Birch1_aut = 816500238;
        static constexpr Hash Birch2 = -1993569970;
        static constexpr Hash Birch2_aut = -789472993;
        static constexpr Hash BirchSeeds = -601117714;
        static constexpr Hash BirchStub = -1425208488;
        static constexpr Hash BlackCore = -566038322;
        static constexpr Hash BlackForestLocationMusic = -94314950;
        static constexpr Hash blackforge = -250560308;
        static constexpr Hash blackforge_ext1 = -797439003;
        static constexpr Hash BlackMarble = 1159653928;
        static constexpr Hash blackmarble_1x1 = -1195767551;
        static constexpr Hash blackmarble_2x1x1 = -1161852937;
        static constexpr Hash blackmarble_2x2_enforced = -502993694;
        static constexpr Hash blackmarble_2x2x1 = -1161852776;
        static constexpr Hash blackmarble_2x2x2 = -1161852777;
        static constexpr Hash blackmarble_altar_crystal = -556080110;
        static constexpr Hash blackmarble_altar_crystal_broken = -750057646;
        static constexpr Hash blackmarble_arch = -1262523563;
        static constexpr Hash blackmarble_base_1 = 1745123876;
        static constexpr Hash blackmarble_base_2 = 1341839349;
        static constexpr Hash blackmarble_basecorner = 232501507;
        static constexpr Hash blackmarble_column_1 = 262767717;
        static constexpr Hash blackmarble_column_2 = -140516810;
        static constexpr Hash blackmarble_column_3 = 1425567131;
        static constexpr Hash blackmarble_creep_4x1x1 = 1341577551;
        static constexpr Hash blackmarble_creep_4x2x1 = 1341577584;
        static constexpr Hash blackmarble_creep_slope_inverted_1x1x2 = 285426183;
        static constexpr Hash blackmarble_creep_slope_inverted_2x2x1 = -978646442;
        static constexpr Hash blackmarble_creep_stair = -822720420;
        static constexpr Hash blackmarble_floor = 747145;
        static constexpr Hash blackmarble_floor_large = -640423049;
        static constexpr Hash blackmarble_floor_triangle = 955526822;
        static constexpr Hash blackmarble_head_big01 = -613415933;
        static constexpr Hash blackmarble_head_big02 = -1016700460;
        static constexpr Hash blackmarble_head01 = -125049440;
        static constexpr Hash blackmarble_head02 = -1691133381;
        static constexpr Hash blackmarble_out_1 = -1881176611;
        static constexpr Hash blackmarble_out_2 = -1881176610;
        static constexpr Hash blackmarble_outcorner = 2097111366;
        static constexpr Hash blackmarble_pile = 592534089;
        static constexpr Hash blackmarble_post01 = -1962134668;
        static constexpr Hash blackmarble_slope_1x2 = 547477602;
        static constexpr Hash blackmarble_slope_inverted_1x2 = -2051475058;
        static constexpr Hash blackmarble_stair = 1580391858;
        static constexpr Hash blackmarble_stair_corner = 1280010482;
        static constexpr Hash blackmarble_stair_corner_left = 1115581890;
        static constexpr Hash blackmarble_tile_floor_1x1 = -473850883;
        static constexpr Hash blackmarble_tile_floor_2x2 = 991476509;
        static constexpr Hash blackmarble_tile_wall_1x1 = 298976879;
        static constexpr Hash blackmarble_tile_wall_2x2 = 298977039;
        static constexpr Hash blackmarble_tile_wall_2x4 = 298977045;
        static constexpr Hash blackmarble_tip = 1082779386;
        static constexpr Hash BlackMetal = -535733020;
        static constexpr Hash BlackMetalScrap = 1762081833;
        static constexpr Hash BlackSoup = 388599790;
        static constexpr Hash blastfurnace = 1048742812;
        static constexpr Hash Blob = 829393023;
        static constexpr Hash blob_aoe = 1122333731;
        static constexpr Hash BlobElite = -249968000;
        static constexpr Hash BlobTar = 1909245568;
        static constexpr Hash blobtar_projectile_tarball = -1079891863;
        static constexpr Hash Bloodbag = 248299036;
        static constexpr Hash BloodPudding = -1293151425;
        static constexpr Hash Blueberries = 583402574;
        static constexpr Hash BlueberryBush = 1998010392;
        static constexpr Hash Boar = -1670867714;
        static constexpr Hash Boar_piggy = 1076559013;
        static constexpr Hash boar_ragdoll = 832544534;
        static constexpr Hash BoarJerky = 150066261;
        static constexpr Hash BoltBlackmetal = -1129651973;
        static constexpr Hash BoltBone = -1853193749;
        static constexpr Hash BoltCarapace = 1700229387;
        static constexpr Hash BoltIron = 1863383;
        static constexpr Hash BombBile = 1590333336;
        static constexpr Hash BombOoze = -96710263;
        static constexpr Hash BoneFragments = 1190462933;
        static constexpr Hash Bonemass = -146537656;
        static constexpr Hash bonemass_aoe = -741676700;
        static constexpr Hash bonemass_throw_projectile = 1289546541;
        static constexpr Hash BonePileSpawner = 1577361568;
        static constexpr Hash bonfire = -999191493;
        static constexpr Hash BossStone_Bonemass = -1521109453;
        static constexpr Hash BossStone_DragonQueen = 1216631312;
        static constexpr Hash BossStone_Eikthyr = -1399780559;
        static constexpr Hash BossStone_TheElder = -1784287464;
        static constexpr Hash BossStone_TheQueen = -774724014;
        static constexpr Hash BossStone_Yagluth = -636214073;
        static constexpr Hash Bow = 1502599522;
        static constexpr Hash bow_projectile = 1261559868;
        static constexpr Hash bow_projectile_carapace = -1438008887;
        static constexpr Hash bow_projectile_fire = -1686607761;
        static constexpr Hash bow_projectile_frost = -755455001;
        static constexpr Hash bow_projectile_needle = -1829892894;
        static constexpr Hash bow_projectile_poison = 752202755;
        static constexpr Hash BowDraugrFang = -1470815101;
        static constexpr Hash BowFineWood = 1304037785;
        static constexpr Hash BowHuntsman = 1410944776;
        static constexpr Hash BowSpineSnap = -49112013;
        static constexpr Hash Bread = 1185250548;
        static constexpr Hash BreadDough = -787066183;
        static constexpr Hash Bronze = -630222754;
        static constexpr Hash BronzeNails = -1729390917;
        static constexpr Hash BronzeScrap = 4675961;
        static constexpr Hash bronzespear_projectile = 2013693133;
        static constexpr Hash bucket = -29074708;
        static constexpr Hash BugMeat = 528336477;
        static constexpr Hash Bush01 = 548904977;
        static constexpr Hash Bush01_heath = -1798873102;
        static constexpr Hash Bush02_en = -136568224;
        static constexpr Hash CapeDeerHide = 1624045309;
        static constexpr Hash CapeFeather = 2134397716;
        static constexpr Hash CapeLinen = -1893623813;
        static constexpr Hash CapeLox = 1726435208;
        static constexpr Hash CapeOdin = -320326905;
        static constexpr Hash CapeTest = 1790203521;
        static constexpr Hash CapeTrollHide = -1068315380;
        static constexpr Hash CapeWolf = -2108127691;
        static constexpr Hash Carapace = 662706540;
        static constexpr Hash CargoCrate = -366938831;
        static constexpr Hash Carrot = 1209145989;
        static constexpr Hash CarrotSeeds = -743867699;
        static constexpr Hash CarrotSoup = 248039118;
        static constexpr Hash Cart = 49675204;
        static constexpr Hash CastleKit_braided_box01 = -1807421665;
        static constexpr Hash CastleKit_brazier = 709785924;
        static constexpr Hash CastleKit_groundtorch = 1477467946;
        static constexpr Hash CastleKit_groundtorch_green = -524464010;
        static constexpr Hash CastleKit_groundtorch_unlit = 1814971431;
        static constexpr Hash CastleKit_metal_groundtorch_unlit = 1312507201;
        static constexpr Hash CastleKit_pot03 = 206059549;
        static constexpr Hash cauldron_ext1_spice = -1284852450;
        static constexpr Hash cauldron_ext3_butchertable = -1932932113;
        static constexpr Hash cauldron_ext4_pots = 1470713869;
        static constexpr Hash cauldron_ext5_mortarandpestle = 250710815;
        static constexpr Hash caverock_ice_pillar_wall = -150393364;
        static constexpr Hash caverock_ice_stalagmite = -2053541920;
        static constexpr Hash caverock_ice_stalagmite_broken = -1349061296;
        static constexpr Hash caverock_ice_stalagmite_destruction = 1935032255;
        static constexpr Hash caverock_ice_stalagtite = -494364525;
        static constexpr Hash caverock_ice_stalagtite_destruction = -691410382;
        static constexpr Hash caverock_ice_stalagtite_falling = -15162285;
        static constexpr Hash caverock_ice_wall_destruction = -481743960;
        static constexpr Hash Chain = 1534575709;
        static constexpr Hash charcoal_kiln = 816862660;
        static constexpr Hash Chest = -696914899;
        static constexpr Hash Chicken = 1534661025;
        static constexpr Hash ChickenEgg = -361224582;
        static constexpr Hash ChickenMeat = 1828514878;
        static constexpr Hash Chitin = -91716071;
        static constexpr Hash cliff_mistlands1 = -154208029;
        static constexpr Hash cliff_mistlands1_creep = 1350859097;
        static constexpr Hash cliff_mistlands1_creep_frac = -1580587000;
        static constexpr Hash cliff_mistlands1_frac = -1437351074;
        static constexpr Hash cliff_mistlands2 = 1411875912;
        static constexpr Hash cliff_mistlands2_frac = 1049096569;
        static constexpr Hash cloth_hanging_door = -1049634774;
        static constexpr Hash cloth_hanging_door_double = -691115746;
        static constexpr Hash cloth_hanging_long = 1773356668;
        static constexpr Hash Cloudberry = -1899504733;
        static constexpr Hash CloudberryBush = 940750515;
        static constexpr Hash Club = 829393066;
        static constexpr Hash Coal = 204392453;
        static constexpr Hash coal_pile = 1803366038;
        static constexpr Hash Coins = 1554778960;
        static constexpr Hash CookedBugMeat = -1431804988;
        static constexpr Hash CookedChickenMeat = -68620523;
        static constexpr Hash CookedDeerMeat = -1559009424;
        static constexpr Hash CookedEgg = 694889784;
        static constexpr Hash CookedHareMeat = 264964714;
        static constexpr Hash CookedLoxMeat = -687491193;
        static constexpr Hash CookedMeat = -90783900;
        static constexpr Hash CookedWolfMeat = -1357090580;
        static constexpr Hash Copper = 1428605659;
        static constexpr Hash CopperOre = 1868171867;
        static constexpr Hash CopperScrap = -2143218796;
        static constexpr Hash CreepProp_egg_hanging01 = 1253270340;
        static constexpr Hash CreepProp_egg_hanging02 = 1253270339;
        static constexpr Hash CreepProp_entrance1 = 1628302764;
        static constexpr Hash CreepProp_entrance2 = 1628302761;
        static constexpr Hash CreepProp_hanging01 = 166421754;
        static constexpr Hash CreepProp_wall01 = 388304576;
        static constexpr Hash CrossbowArbalest = -259499256;
        static constexpr Hash Crow = 903556745;
        static constexpr Hash CryptKey = 1014536855;
        static constexpr Hash Crystal = 908712672;
        static constexpr Hash crystal_wall_1x1 = 650075310;
        static constexpr Hash cultivate = -1005613019;
        static constexpr Hash Cultivator = -1130499279;
        static constexpr Hash Dandelion = 1288940892;
        static constexpr Hash darkwood_arch = 463424268;
        static constexpr Hash darkwood_beam = -840537217;
        static constexpr Hash darkwood_beam_26 = -581430636;
        static constexpr Hash darkwood_beam_45 = 2147452713;
        static constexpr Hash darkwood_beam4x4 = -791142009;
        static constexpr Hash darkwood_decowall = 1429703979;
        static constexpr Hash darkwood_gate = 1276642083;
        static constexpr Hash darkwood_pole = -1351226314;
        static constexpr Hash darkwood_pole4 = -1211998438;
        static constexpr Hash darkwood_raven = 1646817558;
        static constexpr Hash darkwood_roof = 255263578;
        static constexpr Hash darkwood_roof_45 = -1271098328;
        static constexpr Hash darkwood_roof_icorner = -1737626015;
        static constexpr Hash darkwood_roof_icorner_45 = 740898699;
        static constexpr Hash darkwood_roof_ocorner = -1737832933;
        static constexpr Hash darkwood_roof_ocorner_45 = 734070413;
        static constexpr Hash darkwood_roof_top = 360656326;
        static constexpr Hash darkwood_roof_top_45 = 1645551772;
        static constexpr Hash darkwood_wolf = -403494760;
        static constexpr Hash dead_deer = 1619070209;
        static constexpr Hash Deathsquito = -1609638819;
        static constexpr Hash Deathsquito_sting = -361803535;
        static constexpr Hash Deer = 291594142;
        static constexpr Hash deer_ragdoll = -555931738;
        static constexpr Hash DeerGodExplosion = 648911439;
        static constexpr Hash DeerHide = 705284766;
        static constexpr Hash DeerMeat = -1519557673;
        static constexpr Hash DeerStew = -1170233273;
        static constexpr Hash Demister = 23489701;
        static constexpr Hash demister_ball = -1034606963;
        static constexpr Hash DG_Cave = 497418925;
        static constexpr Hash DG_DvergrBoss = -985216323;
        static constexpr Hash DG_DvergrTown = 90381292;
        static constexpr Hash DG_ForestCrypt = -1277205767;
        static constexpr Hash DG_GoblinCamp = 322846882;
        static constexpr Hash DG_MeadowsFarm = -1118485742;
        static constexpr Hash DG_MeadowsVillage = 797712294;
        static constexpr Hash DG_SunkenCrypt = -446811472;
        static constexpr Hash digg = -2000248195;
        static constexpr Hash digg_v2 = -1544763428;
        static constexpr Hash Dragon = -408509179;
        static constexpr Hash dragon_ice_projectile = -1680081557;
        static constexpr Hash DragonEgg = -1736688790;
        static constexpr Hash dragoneggcup = 1178524842;
        static constexpr Hash DragonTear = -1719994617;
        static constexpr Hash Draugr = 505464631;
        static constexpr Hash draugr_arrow = 1799389161;
        static constexpr Hash draugr_axe = -896645116;
        static constexpr Hash draugr_bow = -1488146288;
        static constexpr Hash draugr_bow_projectile = 1671731874;
        static constexpr Hash Draugr_Elite = 437915453;
        static constexpr Hash Draugr_elite_ragdoll = 124978337;
        static constexpr Hash Draugr_ragdoll = 1394133807;
        static constexpr Hash Draugr_Ranged = 515141415;
        static constexpr Hash Draugr_ranged_ragdoll = -2052189689;
        static constexpr Hash draugr_sword = 1986762565;
        static constexpr Hash dungeon_forestcrypt_door = -1481786749;
        static constexpr Hash dungeon_queen_door = 2080330686;
        static constexpr Hash dungeon_sunkencrypt_irongate = -647334815;
        static constexpr Hash Dverger = 1283200037;
        static constexpr Hash dverger_demister = -834617117;
        static constexpr Hash dverger_demister_broken = -508728573;
        static constexpr Hash dverger_demister_large = -411796343;
        static constexpr Hash dverger_demister_ruins = 617126599;
        static constexpr Hash dverger_guardstone = -1176046110;
        static constexpr Hash Dverger_ragdoll = -1261409471;
        static constexpr Hash DvergerArbalest = -704394069;
        static constexpr Hash DvergerArbalest_projectile = -307893503;
        static constexpr Hash DvergerArbalest_shoot = -1462438309;
        static constexpr Hash DvergerHairFemale = 970414861;
        static constexpr Hash DvergerHairMale = -584341638;
        static constexpr Hash DvergerMage = 1911030203;
        static constexpr Hash DvergerMageFire = -798778629;
        static constexpr Hash DvergerMageIce = -1727895168;
        static constexpr Hash DvergerMageSupport = 1379665520;
        static constexpr Hash DvergerStaffBlocker_blockCircle = 1299032657;
        static constexpr Hash DvergerStaffBlocker_blockCircleBig = 275601489;
        static constexpr Hash DvergerStaffBlocker_blockHemisphere = -1230384245;
        static constexpr Hash DvergerStaffBlocker_blockU = 2105931020;
        static constexpr Hash DvergerStaffBlocker_blockWall = -981014735;
        static constexpr Hash DvergerStaffBlocker_projectile = -936573297;
        static constexpr Hash DvergerStaffFire = 701604739;
        static constexpr Hash DvergerStaffFire_clusterbomb_aoe = 1163892070;
        static constexpr Hash DvergerStaffFire_clusterbomb_projectile = -1589283964;
        static constexpr Hash DvergerStaffFire_fire_aoe = 1452519224;
        static constexpr Hash DvergerStaffFire_fireball_projectile = -1933363631;
        static constexpr Hash DvergerStaffHeal = 1703038983;
        static constexpr Hash DvergerStaffHeal_aoe = 1931377867;
        static constexpr Hash DvergerStaffIce = 24503566;
        static constexpr Hash DvergerStaffIce_projectile = 1141102516;
        static constexpr Hash DvergerStaffNova_aoe = -1543403955;
        static constexpr Hash DvergerStaffSupport = 2132636350;
        static constexpr Hash DvergerStaffSupport_aoe = 2122935794;
        static constexpr Hash DvergerSuitArbalest = -1728158308;
        static constexpr Hash DvergerSuitFire = -993981138;
        static constexpr Hash DvergerSuitIce = -1862079955;
        static constexpr Hash DvergerSuitSupport = 901023621;
        static constexpr Hash DvergerTest = 1924585071;
        static constexpr Hash DvergrKey = 1896448127;
        static constexpr Hash DvergrKeyFragment = -2105977201;
        static constexpr Hash DvergrNeedle = 676297687;
        static constexpr Hash dvergrprops_banner = -1896999675;
        static constexpr Hash dvergrprops_barrel = -1977294001;
        static constexpr Hash dvergrprops_bed = -1504935776;
        static constexpr Hash dvergrprops_chair = -489276534;
        static constexpr Hash dvergrprops_crate = -1255181328;
        static constexpr Hash dvergrprops_crate_long = -2055368539;
        static constexpr Hash dvergrprops_curtain = -268842449;
        static constexpr Hash dvergrprops_hooknchain = -617286971;
        static constexpr Hash dvergrprops_lantern = 1842886963;
        static constexpr Hash dvergrprops_lantern_standing = 1592482536;
        static constexpr Hash dvergrprops_pickaxe = 52464828;
        static constexpr Hash dvergrprops_shelf = 270255633;
        static constexpr Hash dvergrprops_stool = 1883651752;
        static constexpr Hash dvergrprops_table = 808046987;
        static constexpr Hash dvergrprops_wood_beam = -2068097986;
        static constexpr Hash dvergrprops_wood_floor = -461418815;
        static constexpr Hash dvergrprops_wood_pole = 2038910655;
        static constexpr Hash dvergrprops_wood_stair = 918842606;
        static constexpr Hash dvergrprops_wood_stake = -196902639;
        static constexpr Hash dvergrprops_wood_stakewall = 872999015;
        static constexpr Hash dvergrprops_wood_wall = -185931429;
        static constexpr Hash dvergrtown_arch = 912102389;
        static constexpr Hash dvergrtown_creep_door = -800886699;
        static constexpr Hash dvergrtown_secretdoor = 1683665181;
        static constexpr Hash dvergrtown_slidingdoor = 766620697;
        static constexpr Hash dvergrtown_stair_corner_wood_left = 227217680;
        static constexpr Hash dvergrtown_wood_beam = 1857996392;
        static constexpr Hash dvergrtown_wood_crane = 1151373408;
        static constexpr Hash dvergrtown_wood_pole = -2073834717;
        static constexpr Hash dvergrtown_wood_stake = -213214859;
        static constexpr Hash dvergrtown_wood_stakewall = 2008887427;
        static constexpr Hash dvergrtown_wood_support = 747816726;
        static constexpr Hash dvergrtown_wood_wall01 = 775933644;
        static constexpr Hash dvergrtown_wood_wall02 = -1952949711;
        static constexpr Hash dvergrtown_wood_wall03 = -386865770;
        static constexpr Hash Eikthyr = -938588874;
        static constexpr Hash eikthyr_ragdoll = -82703974;
        static constexpr Hash Eitr = 1985543946;
        static constexpr Hash eitrrefinery = 731966626;
        static constexpr Hash ElderBark = 938726216;
        static constexpr Hash Entrails = -1985632376;
        static constexpr Hash eventzone_bonemass = -1796573617;
        static constexpr Hash eventzone_eikthyr = 641614345;
        static constexpr Hash eventzone_gdking = -1815933211;
        static constexpr Hash eventzone_goblinking = -630804645;
        static constexpr Hash eventzone_moder = -264209140;
        static constexpr Hash eventzone_queen = -479547905;
        static constexpr Hash EvilHeart_Forest = -2106312224;
        static constexpr Hash EvilHeart_Swamp = 552253847;
        static constexpr Hash Eyescream = 1317085082;
        static constexpr Hash Feathers = -1792723966;
        static constexpr Hash Fenring = 1388670159;
        static constexpr Hash Fenring_attack_flames_aoe = -2089404025;
        static constexpr Hash Fenring_Cultist = -1237836442;
        static constexpr Hash Fenring_cultist_ragdoll = 1571536938;
        static constexpr Hash Fenring_ragdoll = -870478745;
        static constexpr Hash fenrirhide_hanging = 388131667;
        static constexpr Hash fenrirhide_hanging_door = -1305303832;
        static constexpr Hash fermenter = -931391920;
        static constexpr Hash FineWood = 212315135;
        static constexpr Hash FirCone = 183471488;
        static constexpr Hash fire_pit = 399713608;
        static constexpr Hash FireFlies = -1931516595;
        static constexpr Hash FirTree = 1185163063;
        static constexpr Hash FirTree_log = -1147276818;
        static constexpr Hash FirTree_log_half = -755748398;
        static constexpr Hash FirTree_oldLog = 1051479931;
        static constexpr Hash FirTree_Sapling = -2139814576;
        static constexpr Hash FirTree_small = 888684615;
        static constexpr Hash FirTree_small_dead = -367866354;
        static constexpr Hash FirTree_Stub = -2025597564;
        static constexpr Hash Fish1 = 109649205;
        static constexpr Hash Fish10 = -1859318427;
        static constexpr Hash Fish11 = -293234486;
        static constexpr Hash Fish12 = 1272849455;
        static constexpr Hash Fish2 = 109649206;
        static constexpr Hash Fish3 = 109649207;
        static constexpr Hash Fish4_cave = 58693868;
        static constexpr Hash Fish5 = 109649209;
        static constexpr Hash Fish6 = 109649210;
        static constexpr Hash Fish7 = 109649211;
        static constexpr Hash Fish8 = 109649212;
        static constexpr Hash Fish9 = 109649213;
        static constexpr Hash FishAndBread = -1490899117;
        static constexpr Hash FishAndBreadUncooked = 897133405;
        static constexpr Hash FishAnglerRaw = 1399738207;
        static constexpr Hash FishCooked = -1292410483;
        static constexpr Hash FishingBait = 2120058096;
        static constexpr Hash FishingBaitAshlands = 459717098;
        static constexpr Hash FishingBaitCave = 1592185747;
        static constexpr Hash FishingBaitDeepNorth = -772726327;
        static constexpr Hash FishingBaitForest = -1630963917;
        static constexpr Hash FishingBaitMistlands = -550229671;
        static constexpr Hash FishingBaitOcean = -1724877534;
        static constexpr Hash FishingBaitPlains = 1796124627;
        static constexpr Hash FishingBaitSwamp = 1812383840;
        static constexpr Hash FishingRod = 2095458909;
        static constexpr Hash FishingRodFloat = -2128965567;
        static constexpr Hash FishingRodFloatProjectile = 1255402074;
        static constexpr Hash FishRaw = -535377778;
        static constexpr Hash FishWraps = -1650135981;
        static constexpr Hash FistFenrirClaw = -1721646831;
        static constexpr Hash Flametal = -1619429742;
        static constexpr Hash FlametalOre = -22163406;
        static constexpr Hash Flax = 1635961943;
        static constexpr Hash Flies = -1711908779;
        static constexpr Hash Flint = -952393887;
        static constexpr Hash flintspear_projectile = 1600360364;
        static constexpr Hash flying_core = 342810095;
        static constexpr Hash forge = -1530372843;
        static constexpr Hash forge_ext1 = -18266612;
        static constexpr Hash forge_ext2 = -421551139;
        static constexpr Hash forge_ext3 = 1144532802;
        static constexpr Hash forge_ext4 = 385017915;
        static constexpr Hash forge_ext5 = 1951101856;
        static constexpr Hash forge_ext6 = 1547817329;
        static constexpr Hash FreezeGland = -2107335129;
        static constexpr Hash FrostCavesShrineReveal = 1491287432;
        static constexpr Hash fx_Abomination_arise = -863359251;
        static constexpr Hash fx_Abomination_arise_end = 500888217;
        static constexpr Hash fx_Abomination_attack_hit = 1086573447;
        static constexpr Hash fx_Abomination_attack1 = 1753165102;
        static constexpr Hash fx_Abomination_attack1_start = -431820145;
        static constexpr Hash fx_Abomination_attack1_trailon = 840131048;
        static constexpr Hash fx_Abomination_attack2 = 1349880575;
        static constexpr Hash fx_Abomination_attack2_lift = 1651023775;
        static constexpr Hash fx_Abomination_attack2_start = -27091948;
        static constexpr Hash fx_Abomination_attack3 = -1379002780;
        static constexpr Hash fx_Abomination_attack3_start = -812819815;
        static constexpr Hash fx_altar_crystal_destruction = 562073451;
        static constexpr Hash fx_ArmorStand_pick = -1668628040;
        static constexpr Hash fx_babyseeker_death = 2064722325;
        static constexpr Hash fx_babyseeker_hurt = -1897893742;
        static constexpr Hash fx_backstab = 513540838;
        static constexpr Hash fx_bat_death = 2112410193;
        static constexpr Hash fx_bat_hit = -1854382242;
        static constexpr Hash fx_blobtar_tarball_hit = 2138920922;
        static constexpr Hash fx_boar_pet = 1284548697;
        static constexpr Hash fx_Bonemass_aoe_start = -132707708;
        static constexpr Hash fx_chicken_birth = 1713303714;
        static constexpr Hash fx_chicken_death = 786808599;
        static constexpr Hash fx_chicken_lay_egg = 1503489357;
        static constexpr Hash fx_chicken_pet = -2055972266;
        static constexpr Hash fx_creature_tamed = 1367755680;
        static constexpr Hash fx_crit = 1083439213;
        static constexpr Hash fx_crystal_destruction = -1115480558;
        static constexpr Hash fx_deathsquito_hit = 521449968;
        static constexpr Hash fx_deatsquito_death = -957960149;
        static constexpr Hash fx_dragon_land = 1159596902;
        static constexpr Hash fx_drown = 1358135697;
        static constexpr Hash fx_Dverger_death = 556448455;
        static constexpr Hash fx_Dverger_hit = 50725268;
        static constexpr Hash fx_DvergerMage_Fire_hit = -129031101;
        static constexpr Hash fx_DvergerMage_Ice_hit = -240630674;
        static constexpr Hash fx_DvergerMage_Mistile_attack = 1476940549;
        static constexpr Hash fx_DvergerMage_Mistile_die = 149607269;
        static constexpr Hash fx_DvergerMage_MistileSpawn = -1741307523;
        static constexpr Hash fx_DvergerMage_Nova_ring = 1696607086;
        static constexpr Hash fx_DvergerMage_Support_hit = -1381385898;
        static constexpr Hash fx_DvergerMage_Support_start = 636683973;
        static constexpr Hash fx_egg_splash = -1559239972;
        static constexpr Hash fx_eikthyr_forwardshockwave = 96144440;
        static constexpr Hash fx_eikthyr_stomp = -311152809;
        static constexpr Hash fx_fenring_flames = 27262405;
        static constexpr Hash fx_fireball_staff_explosion = 978263589;
        static constexpr Hash fx_float_hitwater = 1054502638;
        static constexpr Hash fx_float_nibble = -1790688672;
        static constexpr Hash fx_gdking_rootspawn = 727170115;
        static constexpr Hash fx_gjall_death = -98943274;
        static constexpr Hash fx_gjall_egg_splat = 673467202;
        static constexpr Hash fx_gjall_taunt = 304272980;
        static constexpr Hash fx_goblinbrute_groundslam = -1070004643;
        static constexpr Hash fx_goblinking_beam_hit = 917183013;
        static constexpr Hash fx_goblinking_death = -1899360684;
        static constexpr Hash fx_goblinking_hit = 1274496653;
        static constexpr Hash fx_goblinking_meteor_hit = -882794760;
        static constexpr Hash fx_goblinking_nova = 977083296;
        static constexpr Hash fx_GoblinShieldBreak = -29779998;
        static constexpr Hash fx_GoblinShieldHit = 1378191352;
        static constexpr Hash fx_GP_Activation = 1210376161;
        static constexpr Hash fx_GP_Player = -1064502302;
        static constexpr Hash fx_GP_Stone = -964066384;
        static constexpr Hash fx_guardstone_activate = 504146489;
        static constexpr Hash fx_guardstone_deactivate = -854606842;
        static constexpr Hash fx_guardstone_permitted_add = -842656812;
        static constexpr Hash fx_guardstone_permitted_removed = 763516691;
        static constexpr Hash fx_hare_death = -2038752798;
        static constexpr Hash fx_hen_death = -904156699;
        static constexpr Hash fx_hen_love = -1454256847;
        static constexpr Hash fx_himminafl_aoe = 1958096728;
        static constexpr Hash fx_himminafl_hit = 116336006;
        static constexpr Hash fx_hottub_addwood = 168056958;
        static constexpr Hash fx_icefloor_destruction = -1417255809;
        static constexpr Hash fx_iceshard_hit = 547029702;
        static constexpr Hash fx_iceshard_launch = -617943502;
        static constexpr Hash fx_icicle_destruction = -469159059;
        static constexpr Hash fx_Immobilize = -957886916;
        static constexpr Hash fx_jelly_pickup = -722558772;
        static constexpr Hash fx_jotunbane_hit = -2021885435;
        static constexpr Hash fx_jotunbane_swing = -217769200;
        static constexpr Hash fx_leviathan_leave = 1907076913;
        static constexpr Hash fx_leviathan_reaction = -1908485725;
        static constexpr Hash fx_Lightning = -590558407;
        static constexpr Hash fx_lox_death = -1556292783;
        static constexpr Hash fx_lox_hit = -13297202;
        static constexpr Hash fx_lox_pet = 504556970;
        static constexpr Hash fx_lox_tamed = -117748084;
        static constexpr Hash fx_loxcalf_death = 1238354849;
        static constexpr Hash fx_oven_add_food = 463296512;
        static constexpr Hash fx_oven_add_wood = 463297073;
        static constexpr Hash fx_oven_produce = -1157562916;
        static constexpr Hash fx_pheromonebomb_explode = 1032921632;
        static constexpr Hash fx_Potion_frostresist = -1445195853;
        static constexpr Hash fx_Puke = -1094090678;
        static constexpr Hash fx_Queen_Death = 822590676;
        static constexpr Hash fx_QueenPierceGround = -1760540482;
        static constexpr Hash fx_radiation_hit = 2010899246;
        static constexpr Hash fx_refinery_addfuel = -232765807;
        static constexpr Hash fx_refinery_addtissue = 1721144258;
        static constexpr Hash fx_refinery_destroyed = 1805728041;
        static constexpr Hash fx_refinery_produce = -320069762;
        static constexpr Hash fx_seeker_death = -921279507;
        static constexpr Hash fx_seeker_hurt = -1243786294;
        static constexpr Hash fx_seeker_melee_hit = -137185637;
        static constexpr Hash fx_seeker_spawn = 516948170;
        static constexpr Hash fx_seekerbrute_death = 1416166045;
        static constexpr Hash fx_shaman_fireball_expl = 1140086335;
        static constexpr Hash fx_shaman_protect = -430233955;
        static constexpr Hash fx_shield_start = -989584787;
        static constexpr Hash fx_skeleton_pet = 2004573612;
        static constexpr Hash fx_sledge_demolisher_hit = 624080642;
        static constexpr Hash fx_StaffShield_Break = -951090096;
        static constexpr Hash fx_StaffShield_Hit = 412281882;
        static constexpr Hash fx_summon_skeleton = -1063035972;
        static constexpr Hash fx_summon_skeleton_spawn = -741750904;
        static constexpr Hash fx_summon_start = 99167765;
        static constexpr Hash fx_sw_addflax = 778331988;
        static constexpr Hash fx_sw_produce = 26707384;
        static constexpr Hash fx_tar_bubbles = -293996892;
        static constexpr Hash fx_tentaroot_death = -1301911090;
        static constexpr Hash fx_tick_death = 621146823;
        static constexpr Hash fx_TickBloodHit = 1335934803;
        static constexpr Hash fx_tombstone_destroyed = -612587032;
        static constexpr Hash fx_totem_destroyed = -550679782;
        static constexpr Hash fx_trap_arm = -779606389;
        static constexpr Hash fx_trap_trigger = 470924985;
        static constexpr Hash fx_turret_addammo = -896000907;
        static constexpr Hash fx_turret_fire = -328764516;
        static constexpr Hash fx_turret_newtarget = 1118601191;
        static constexpr Hash fx_turret_notarget = 1881957582;
        static constexpr Hash fx_turret_reload = -36239653;
        static constexpr Hash fx_turret_warmup = -434429456;
        static constexpr Hash fx_vines_hit = 1213446712;
        static constexpr Hash fx_WaterImpact_Big = 350984073;
        static constexpr Hash fx_wolf_pet = 287754685;
        static constexpr Hash gd_king = -2101309657;
        static constexpr Hash gdking_Ragdoll = -811560274;
        static constexpr Hash gdking_root_projectile = 696105667;
        static constexpr Hash Ghost = -696918929;
        static constexpr Hash giant_arm = -1566278426;
        static constexpr Hash giant_brain = -1337230376;
        static constexpr Hash giant_brain_frac = 1854887461;
        static constexpr Hash giant_helmet1 = 1217240684;
        static constexpr Hash giant_helmet1_destruction = -54060277;
        static constexpr Hash giant_helmet2 = 1217240681;
        static constexpr Hash giant_helmet2_destruction = -709355702;
        static constexpr Hash giant_ribs = -960615190;
        static constexpr Hash giant_ribs_frac = -1590213047;
        static constexpr Hash giant_skull = -1505268321;
        static constexpr Hash giant_skull_frac = -1348490946;
        static constexpr Hash giant_sword1 = -1493410116;
        static constexpr Hash giant_sword1_destruction = -60725897;
        static constexpr Hash giant_sword2 = 72673825;
        static constexpr Hash giant_sword2_destruction = 619420902;
        static constexpr Hash GiantBloodSack = -1523551927;
        static constexpr Hash Gjall = 8893374;
        static constexpr Hash gjall_egg_projectile = -2008396786;
        static constexpr Hash gjall_spit_projectile = 713085343;
        static constexpr Hash GlowingMushroom = -1457187493;
        static constexpr Hash Goblin = -137741679;
        static constexpr Hash goblin_banner = 2045908432;
        static constexpr Hash goblin_bed = -1998259239;
        static constexpr Hash Goblin_Dragdoll = 608207097;
        static constexpr Hash goblin_fence = 950265897;
        static constexpr Hash goblin_pole = 133115456;
        static constexpr Hash goblin_pole_small = 440394388;
        static constexpr Hash goblin_roof_45d = 654372746;
        static constexpr Hash goblin_roof_45d_corner = -696037094;
        static constexpr Hash goblin_roof_cap = 842587907;
        static constexpr Hash goblin_stairs = -1788665696;
        static constexpr Hash goblin_stepladder = 70596228;
        static constexpr Hash goblin_totempole = -164406293;
        static constexpr Hash goblin_woodwall_1m = -216645858;
        static constexpr Hash goblin_woodwall_2m = -216645857;
        static constexpr Hash goblin_woodwall_2m_ribs = 2020886964;
        static constexpr Hash GoblinArcher = -1508843442;
        static constexpr Hash GoblinArmband = 1639499022;
        static constexpr Hash GoblinBrute = -939999423;
        static constexpr Hash GoblinBrute_ArmGuard = 801523509;
        static constexpr Hash GoblinBrute_Attack = 1626038296;
        static constexpr Hash GoblinBrute_Backbones = -1141183846;
        static constexpr Hash GoblinBrute_ExecutionerCap = 1057188125;
        static constexpr Hash GoblinBrute_HipCloth = -1678144185;
        static constexpr Hash GoblinBrute_LegBones = -1023139695;
        static constexpr Hash GoblinBrute_ragdoll = 195844421;
        static constexpr Hash GoblinBrute_RageAttack = 1750141103;
        static constexpr Hash GoblinBrute_ShoulderGuard = 886640797;
        static constexpr Hash GoblinBrute_Taunt = 422634212;
        static constexpr Hash GoblinClub = 1243528541;
        static constexpr Hash GoblinHelmet = -1372598740;
        static constexpr Hash GoblinKing = -221799126;
        static constexpr Hash GoblinKing_ragdoll = -1183704758;
        static constexpr Hash goblinking_totemholder = -1299706766;
        static constexpr Hash GoblinLegband = 606128236;
        static constexpr Hash GoblinLoin = -1182826951;
        static constexpr Hash GoblinShaman = -315180887;
        static constexpr Hash GoblinShaman_attack_poke = 562187776;
        static constexpr Hash GoblinShaman_Headdress_antlers = 454410833;
        static constexpr Hash GoblinShaman_Headdress_feathers = -776894186;
        static constexpr Hash GoblinShaman_projectile_fireball = 58584771;
        static constexpr Hash GoblinShaman_protect_aoe = 693601149;
        static constexpr Hash GoblinShaman_ragdoll = -1610435819;
        static constexpr Hash GoblinShaman_Staff_Bones = 1167933866;
        static constexpr Hash GoblinShaman_Staff_Feathers = -1055192031;
        static constexpr Hash GoblinShoulders = -132972994;
        static constexpr Hash GoblinSpear = 552197258;
        static constexpr Hash GoblinSpear_projectile = 740613604;
        static constexpr Hash GoblinSword = -1000332366;
        static constexpr Hash GoblinTorch = 1923162683;
        static constexpr Hash GoblinTotem = -1209005522;
        static constexpr Hash Greydwarf = 1126707611;
        static constexpr Hash Greydwarf_Elite = -1374218359;
        static constexpr Hash Greydwarf_elite_ragdoll = -1011209571;
        static constexpr Hash Greydwarf_ragdoll = 605908579;
        static constexpr Hash Greydwarf_Root = -1510980060;
        static constexpr Hash Greydwarf_Shaman = 762782418;
        static constexpr Hash Greydwarf_Shaman_ragdoll = -792591186;
        static constexpr Hash Greydwarf_throw_projectile = -340666050;
        static constexpr Hash GreydwarfEye = -826622060;
        static constexpr Hash Greyling = -50960667;
        static constexpr Hash Greyling_ragdoll = 1354298625;
        static constexpr Hash guard_stone = -1024209535;
        static constexpr Hash guard_stone_test = 783188550;
        static constexpr Hash Guck = 177541128;
        static constexpr Hash GuckSack = -2040480890;
        static constexpr Hash GuckSack_small = -914926950;
        static constexpr Hash Haldor = 375476424;
        static constexpr Hash Hammer = 200814284;
        static constexpr Hash hanging_hairstrands = -1418004192;
        static constexpr Hash HardAntler = 1712371231;
        static constexpr Hash Hare = -1966747132;
        static constexpr Hash Hare_ragdoll = -132344452;
        static constexpr Hash HareMeat = 1790522819;
        static constexpr Hash Hatchling = -729507656;
        static constexpr Hash hatchling_cold_projectile = -835209289;
        static constexpr Hash Hatchling_ragdoll = -789524728;
        static constexpr Hash HealthUpgrade_Bonemass = -1717780267;
        static constexpr Hash HealthUpgrade_GDKing = -1387001121;
        static constexpr Hash hearth = 656210618;
        static constexpr Hash HeathRockPillar = -2132038161;
        static constexpr Hash HeathRockPillar_frac = -328261538;
        static constexpr Hash HelmetBronze = 1813259955;
        static constexpr Hash HelmetCarapace = 239278391;
        static constexpr Hash HelmetDrake = 1408519140;
        static constexpr Hash HelmetDverger = 703889544;
        static constexpr Hash HelmetFenring = -178897214;
        static constexpr Hash HelmetIron = 2113728123;
        static constexpr Hash HelmetLeather = 70008852;
        static constexpr Hash HelmetMage = -1744197025;
        static constexpr Hash HelmetMidsummerCrown = -1421560593;
        static constexpr Hash HelmetOdin = -204964095;
        static constexpr Hash HelmetPadded = 1885444557;
        static constexpr Hash HelmetRoot = -1119196061;
        static constexpr Hash HelmetTrollLeather = 1503400855;
        static constexpr Hash HelmetYule = 1213051262;
        static constexpr Hash Hen = -1273337693;
        static constexpr Hash highstone = -1383558263;
        static constexpr Hash highstone_frac = -1305625596;
        static constexpr Hash Hive = -837447128;
        static constexpr Hash hive_throw_projectile = -695630227;
        static constexpr Hash Hoe = 1502599834;
        static constexpr Hash Honey = 1601842181;
        static constexpr Hash HoneyGlazedChicken = 931607157;
        static constexpr Hash HoneyGlazedChickenUncooked = -1256110725;
        static constexpr Hash horizontal_web = 193150005;
        static constexpr Hash HugeRoot1 = -832142516;
        static constexpr Hash Ice_floor = -1448864844;
        static constexpr Hash Ice_floor_fractured = -1169366101;
        static constexpr Hash ice_rock1 = 343637466;
        static constexpr Hash ice_rock1_frac = 1448015437;
        static constexpr Hash ice1 = 1017608796;
        static constexpr Hash IceBlocker = 492655805;
        static constexpr Hash Imp_fireball_projectile = -1372761134;
        static constexpr Hash incinerator = -1876813566;
        static constexpr Hash Iron = 1400949664;
        static constexpr Hash iron_floor_1x1 = -1904833886;
        static constexpr Hash iron_floor_2x2 = -479912446;
        static constexpr Hash iron_grate = -2019670596;
        static constexpr Hash iron_wall_1x1 = 424208618;
        static constexpr Hash iron_wall_2x2 = 424208778;
        static constexpr Hash IronNails = 584343451;
        static constexpr Hash IronOre = 411615136;
        static constexpr Hash IronScrap = 1511877305;
        static constexpr Hash itemstand = -1235119491;
        static constexpr Hash itemstandh = 1822362821;
        static constexpr Hash jute_carpet = -1313065786;
        static constexpr Hash jute_carpet_blue = 321998505;
        static constexpr Hash JuteBlue = 59425464;
        static constexpr Hash JuteRed = 502052093;
        static constexpr Hash Karve = -925528333;
        static constexpr Hash KnifeBlackMetal = 1961661821;
        static constexpr Hash KnifeButcher = 1365582486;
        static constexpr Hash KnifeChitin = -505018634;
        static constexpr Hash KnifeCopper = 1410911030;
        static constexpr Hash KnifeFlint = -1567646802;
        static constexpr Hash KnifeSilver = -1378519326;
        static constexpr Hash KnifeSkollAndHati = -1987106861;
        static constexpr Hash Lantern = 1659590678;
        static constexpr Hash Larva = -925531532;
        static constexpr Hash LeatherScraps = 1490625731;
        static constexpr Hash Leech = -1537236269;
        static constexpr Hash Leech_cave = -98355895;
        static constexpr Hash LevelTerrain = __H("LevelTerrain");
        static constexpr Hash Leviathan = 1332964068;
        static constexpr Hash lightningAOE = -1940000293;
        static constexpr Hash LinenThread = -1376039418;
        static constexpr Hash LocationProxy = -313992107;
        static constexpr Hash loot_chest_stone = -323903440;
        static constexpr Hash loot_chest_wood = 143869018;
        static constexpr Hash LootSpawner_pineforest = 2067969702;
        static constexpr Hash Lox = 1502599715;
        static constexpr Hash Lox_Calf = 2032626552;
        static constexpr Hash lox_ragdoll = 886454387;
        static constexpr Hash lox_ribs = -1181455604;
        static constexpr Hash lox_stomp_aoe_OLD = 433303229;
        static constexpr Hash loxcalf_ragdoll = 128389279;
        static constexpr Hash LoxMeat = 1273492894;
        static constexpr Hash LoxPelt = 702453006;
        static constexpr Hash LoxPie = 16727443;
        static constexpr Hash LoxPieUncooked = 2035010361;
        static constexpr Hash LuredWisp = 628311579;
        static constexpr Hash MaceBronze = 1042173684;
        static constexpr Hash MaceIron = -2074455458;
        static constexpr Hash MaceNeedle = 1646123621;
        static constexpr Hash MaceSilver = -589925683;
        static constexpr Hash MagicallyStuffedShroom = 1736129798;
        static constexpr Hash MagicallyStuffedShroomUncooked = 508319580;
        static constexpr Hash Mandible = 802493318;
        static constexpr Hash marker01 = -1738390869;
        static constexpr Hash marker02 = -1335106342;
        static constexpr Hash MeadBaseEitrMinor = 1246556425;
        static constexpr Hash MeadBaseFrostResist = 1001949628;
        static constexpr Hash MeadBaseHealthMajor = 251116763;
        static constexpr Hash MeadBaseHealthMedium = -337119751;
        static constexpr Hash MeadBaseHealthMinor = 2092877623;
        static constexpr Hash MeadBasePoisonResist = 196470766;
        static constexpr Hash MeadBaseStaminaLingering = 1129221382;
        static constexpr Hash MeadBaseStaminaMedium = 279948666;
        static constexpr Hash MeadBaseStaminaMinor = -1061353886;
        static constexpr Hash MeadBaseTasty = 1894904111;
        static constexpr Hash MeadEitrMinor = 915055238;
        static constexpr Hash MeadFrostResist = 647021439;
        static constexpr Hash MeadHealthMajor = -921729736;
        static constexpr Hash MeadHealthMedium = -1416734660;
        static constexpr Hash MeadHealthMinor = 920031124;
        static constexpr Hash MeadPoisonResist = -1382234189;
        static constexpr Hash MeadStaminaLingering = -717547805;
        static constexpr Hash MeadStaminaMedium = -1060658883;
        static constexpr Hash MeadStaminaMinor = -1149314721;
        static constexpr Hash MeadTasty = -996243444;
        static constexpr Hash MeatPlatter = 696240715;
        static constexpr Hash MeatPlatterUncooked = 625870585;
        static constexpr Hash MechanicalSpring = 1846396992;
        static constexpr Hash metalbar_1x2 = -400101552;
        static constexpr Hash MinceMeatSauce = -1219034992;
        static constexpr Hash MineRock_Copper = 723343212;
        static constexpr Hash MineRock_Iron = -922930389;
        static constexpr Hash MineRock_Meteorite = -971628607;
        static constexpr Hash MineRock_Obsidian = 820355464;
        static constexpr Hash MineRock_Stone = -1789907722;
        static constexpr Hash MineRock_Tin = -1882492588;
        static constexpr Hash MistArea = -459035306;
        static constexpr Hash MistArea_edge = -1500069970;
        static constexpr Hash MistArea_small = 676739018;
        static constexpr Hash MisthareSupreme = 1085766256;
        static constexpr Hash MisthareSupremeUncooked = 229705406;
        static constexpr Hash Mistile = -1247468837;
        static constexpr Hash Mistile_kamikaze = 1054459491;
        static constexpr Hash mistvolume = -1863961073;
        static constexpr Hash MountainGraveStone01 = 268544908;
        static constexpr Hash MountainKit_brazier = -42805769;
        static constexpr Hash mountainkit_chair = -1770394629;
        static constexpr Hash mountainkit_table = 972026940;
        static constexpr Hash MountainKit_wood_gate = 1604827893;
        static constexpr Hash mud_road = 463677683;
        static constexpr Hash mudpile = -165023748;
        static constexpr Hash mudpile_beacon = -42517997;
        static constexpr Hash mudpile_frac = -1189190447;
        static constexpr Hash mudpile_old = -1480591642;
        static constexpr Hash mudpile2 = 1756993846;
        static constexpr Hash mudpile2_frac = -47303797;
        static constexpr Hash Mushroom = -1961898040;
        static constexpr Hash MushroomBlue = 660592112;
        static constexpr Hash MushroomJotunPuffs = -1620577410;
        static constexpr Hash MushroomMagecap = -1474822598;
        static constexpr Hash MushroomOmelette = -1143960049;
        static constexpr Hash MushroomYellow = -115856560;
        static constexpr Hash Music_FulingCamp = -1627403090;
        static constexpr Hash Music_GreydwarfCamp = 80755174;
        static constexpr Hash Music_MeadowsVillageFarm = -1659806456;
        static constexpr Hash Music_MountainCottage = -773537486;
        static constexpr Hash Music_StoneHenge = -2044334466;
        static constexpr Hash Neck = -2081058657;
        static constexpr Hash Neck_Ragdoll = -152497;
        static constexpr Hash NeckTail = -1398896717;
        static constexpr Hash NeckTailGrilled = -1094347100;
        static constexpr Hash Needle = 1523164967;
        static constexpr Hash Oak_log = -1822460498;
        static constexpr Hash Oak_log_half = 719041002;
        static constexpr Hash Oak_Sapling = -172929820;
        static constexpr Hash Oak1 = 735284842;
        static constexpr Hash OakStub = 1457518369;
        static constexpr Hash Obsidian = -215580535;
        static constexpr Hash odin = 1313230286;
        static constexpr Hash OLD_wood_roof = -1278775552;
        static constexpr Hash OLD_wood_roof_icorner = -495402031;
        static constexpr Hash OLD_wood_roof_ocorner = -495338349;
        static constexpr Hash OLD_wood_roof_top = -1083140140;
        static constexpr Hash OLD_wood_wall_roof = 1928745705;
        static constexpr Hash Onion = -1315263071;
        static constexpr Hash OnionSeeds = 1180314225;
        static constexpr Hash OnionSoup = 1592016380;
        static constexpr Hash Ooze = 1414246171;
        static constexpr Hash oozebomb_explosion = -551548701;
        static constexpr Hash oozebomb_projectile = -272518761;
        static constexpr Hash path = -1919693987;
        static constexpr Hash paved_road = 456394941;
        static constexpr Hash Pickable_Barley = -739875783;
        static constexpr Hash Pickable_Barley_Wild = -494395474;
        static constexpr Hash Pickable_BlackCoreStand = -958543452;
        static constexpr Hash Pickable_BogIronOre = 1644571806;
        static constexpr Hash Pickable_Branch = 2035819856;
        static constexpr Hash Pickable_Carrot = 600686073;
        static constexpr Hash Pickable_Dandelion = 16756586;
        static constexpr Hash Pickable_DolmenTreasure = 1500324956;
        static constexpr Hash Pickable_DragonEgg = -321593056;
        static constexpr Hash Pickable_DvergerThing = -2077136871;
        static constexpr Hash Pickable_DvergrLantern = -1170674848;
        static constexpr Hash Pickable_DvergrMineTreasure = -803714842;
        static constexpr Hash Pickable_DvergrStein = -2043959483;
        static constexpr Hash Pickable_Fishingrod = -524508767;
        static constexpr Hash Pickable_Flax = -1182933197;
        static constexpr Hash Pickable_Flax_Wild = 1325033780;
        static constexpr Hash Pickable_Flint = -1217738099;
        static constexpr Hash Pickable_ForestCryptRandom = -2081011024;
        static constexpr Hash Pickable_ForestCryptRemains01 = 449644523;
        static constexpr Hash Pickable_ForestCryptRemains02 = 449644526;
        static constexpr Hash Pickable_ForestCryptRemains03 = 449644525;
        static constexpr Hash Pickable_ForestCryptRemains04 = 449644520;
        static constexpr Hash Pickable_Hairstrands01 = -397428878;
        static constexpr Hash Pickable_Hairstrands02 = 5855649;
        static constexpr Hash Pickable_Item = -477120423;
        static constexpr Hash Pickable_MeatPile = 445972681;
        static constexpr Hash Pickable_Meteorite = -1081635004;
        static constexpr Hash Pickable_MountainCaveCrystal = 299318408;
        static constexpr Hash Pickable_MountainCaveObsidian = -1115391601;
        static constexpr Hash Pickable_MountainCaveRandom = 478105205;
        static constexpr Hash Pickable_MountainRemains01_buried = -1816579317;
        static constexpr Hash Pickable_Mushroom = 1520650186;
        static constexpr Hash Pickable_Mushroom_blue = -1843622859;
        static constexpr Hash Pickable_Mushroom_JotunPuffs = -1419806457;
        static constexpr Hash Pickable_Mushroom_Magecap = -1313626337;
        static constexpr Hash Pickable_Mushroom_yellow = -1342135541;
        static constexpr Hash Pickable_Obsidian = -1991911291;
        static constexpr Hash Pickable_Onion = 959601677;
        static constexpr Hash Pickable_RandomFood = -1117014999;
        static constexpr Hash Pickable_RoyalJelly = 1661049019;
        static constexpr Hash Pickable_SeedCarrot = 1405222666;
        static constexpr Hash Pickable_SeedOnion = 539751898;
        static constexpr Hash Pickable_SeedTurnip = 1947547595;
        static constexpr Hash Pickable_Stone = -1471593253;
        static constexpr Hash Pickable_SunkenCryptRandom = 849594011;
        static constexpr Hash Pickable_SurtlingCoreStand = 2014622831;
        static constexpr Hash Pickable_Tar = 908600739;
        static constexpr Hash Pickable_TarBig = 412021611;
        static constexpr Hash Pickable_Thistle = 1175276079;
        static constexpr Hash Pickable_Tin = -2129458801;
        static constexpr Hash Pickable_Turnip = 412034074;
        static constexpr Hash PickaxeAntler = -961424307;
        static constexpr Hash PickaxeBlackMetal = 1840385895;
        static constexpr Hash PickaxeBronze = -1585937997;
        static constexpr Hash PickaxeIron = 915836683;
        static constexpr Hash PickaxeStone = 1083748678;
        static constexpr Hash piece_artisanstation = 1642546285;
        static constexpr Hash piece_banner01 = 420204856;
        static constexpr Hash piece_banner02 = 823489383;
        static constexpr Hash piece_banner03 = -742594558;
        static constexpr Hash piece_banner04 = 16920329;
        static constexpr Hash piece_banner05 = -1549163612;
        static constexpr Hash piece_banner06 = -1145879085;
        static constexpr Hash piece_banner07 = 1583004270;
        static constexpr Hash piece_banner08 = -1952448139;
        static constexpr Hash piece_banner09 = 776435216;
        static constexpr Hash piece_banner10 = 1986288798;
        static constexpr Hash piece_banner11 = 420204857;
        static constexpr Hash piece_bathtub = 2109981969;
        static constexpr Hash piece_bed02 = 2024210682;
        static constexpr Hash piece_beehive = 28120085;
        static constexpr Hash piece_bench01 = -2122807554;
        static constexpr Hash piece_blackmarble_bench = -909098104;
        static constexpr Hash piece_blackmarble_table = 1705458814;
        static constexpr Hash piece_blackmarble_throne = -2139837784;
        static constexpr Hash piece_brazierceiling01 = 997718274;
        static constexpr Hash piece_brazierfloor01 = -1758232271;
        static constexpr Hash piece_cartographytable = -164515435;
        static constexpr Hash piece_cauldron = -1652490355;
        static constexpr Hash piece_chair = -637414342;
        static constexpr Hash piece_chair02 = -1717421956;
        static constexpr Hash piece_chair03 = -1717421957;
        static constexpr Hash piece_chest = -1443983522;
        static constexpr Hash piece_chest_blackmetal = 1417076401;
        static constexpr Hash piece_chest_private = 698259470;
        static constexpr Hash piece_chest_treasure = -475222070;
        static constexpr Hash piece_chest_wood = 328745978;
        static constexpr Hash piece_cloth_hanging_door = -1046192253;
        static constexpr Hash piece_cloth_hanging_door_blue = 124607798;
        static constexpr Hash piece_cloth_hanging_door_blue2 = 1074246236;
        static constexpr Hash piece_cookingstation = 1430888227;
        static constexpr Hash piece_cookingstation_iron = 778038248;
        static constexpr Hash piece_dvergr_lantern = 1765483382;
        static constexpr Hash piece_dvergr_lantern_pole = 463374813;
        static constexpr Hash piece_dvergr_metal_wall_2x2 = -957080763;
        static constexpr Hash piece_dvergr_pole = 1212932712;
        static constexpr Hash piece_dvergr_sharpstakes = -193320087;
        static constexpr Hash piece_dvergr_spiralstair = 1904537040;
        static constexpr Hash piece_dvergr_spiralstair_right = 1347147117;
        static constexpr Hash piece_dvergr_stake_wall = -2144306477;
        static constexpr Hash piece_dvergr_wood_door = 394713400;
        static constexpr Hash piece_dvergr_wood_wall = 985956632;
        static constexpr Hash piece_gift1 = -1181865690;
        static constexpr Hash piece_gift2 = -1181865689;
        static constexpr Hash piece_gift3 = -1181865688;
        static constexpr Hash piece_groundtorch = 261914232;
        static constexpr Hash piece_groundtorch_blue = 1441955863;
        static constexpr Hash piece_groundtorch_green = -1296288316;
        static constexpr Hash piece_groundtorch_mist = -1575901004;
        static constexpr Hash piece_groundtorch_wood = 1744483228;
        static constexpr Hash piece_jackoturnip = 232336033;
        static constexpr Hash piece_logbench01 = -1214978310;
        static constexpr Hash piece_magetable = -832507735;
        static constexpr Hash piece_magetable_ext = -799092397;
        static constexpr Hash piece_maypole = -780564436;
        static constexpr Hash piece_mistletoe = 1816215321;
        static constexpr Hash piece_oven = 2024246663;
        static constexpr Hash piece_sapcollector = -2013355542;
        static constexpr Hash piece_sharpstakes = 1059480620;
        static constexpr Hash piece_spinningwheel = 411883698;
        static constexpr Hash piece_stonecutter = -34430377;
        static constexpr Hash piece_table = 915094071;
        static constexpr Hash piece_table_oak = -1159820015;
        static constexpr Hash piece_table_round = 441267892;
        static constexpr Hash piece_throne01 = 743791676;
        static constexpr Hash piece_throne02 = 340507149;
        static constexpr Hash piece_trap_troll = 1294771866;
        static constexpr Hash piece_turret = -816396091;
        static constexpr Hash piece_walltorch = 1038085717;
        static constexpr Hash piece_wisplure = 1469046252;
        static constexpr Hash piece_workbench = -958010034;
        static constexpr Hash piece_workbench_ext1 = 1017582937;
        static constexpr Hash piece_workbench_ext2 = 1420867464;
        static constexpr Hash piece_workbench_ext3 = -145216477;
        static constexpr Hash piece_workbench_ext4 = 614298410;
        static constexpr Hash piece_xmascrown = 1544093477;
        static constexpr Hash piece_xmasgarland = 1114342237;
        static constexpr Hash piece_xmastree = -1443700678;
        static constexpr Hash PineCone = 1779180581;
        static constexpr Hash PineTree = 1295600320;
        static constexpr Hash Pinetree_01 = 797319082;
        static constexpr Hash Pinetree_01_Stub = 364904293;
        static constexpr Hash PineTree_log = 1111243907;
        static constexpr Hash PineTree_log_half = 1016303223;
        static constexpr Hash PineTree_log_halfOLD = 749341040;
        static constexpr Hash PineTree_logOLD = -1636733362;
        static constexpr Hash PineTree_Sapling = -1134441643;
        static constexpr Hash Player = 1875862075;
        static constexpr Hash Player_ragdoll = -1124029525;
        static constexpr Hash Player_ragdoll_old = 751687535;
        static constexpr Hash Player_tombstone = -1558312669;
        static constexpr Hash PlayerUnarmed = -442047277;
        static constexpr Hash portal = -1495166496;
        static constexpr Hash portal_wood = -661882940;
        static constexpr Hash projectile_beam = -225359483;
        static constexpr Hash projectile_chitinharpoon = -570735290;
        static constexpr Hash projectile_meteor = 286479006;
        static constexpr Hash projectile_wolffang = -870216900;
        static constexpr Hash Pukeberries = -836947401;
        static constexpr Hash QueenBee = -1533933068;
        static constexpr Hash QueenDrop = 1064174259;
        static constexpr Hash QueensJam = -474798661;
        static constexpr Hash radiation = 2065490179;
        static constexpr Hash Raft = 49675681;
        static constexpr Hash raise = -1685068370;
        static constexpr Hash Raspberry = 1479220028;
        static constexpr Hash RaspberryBush = 1809630058;
        static constexpr Hash RawMeat = -666181511;
        static constexpr Hash replant = -2120318468;
        static constexpr Hash Resin = -730656777;
        static constexpr Hash Rock_3 = 1172889253;
        static constexpr Hash Rock_3_frac = 1649426798;
        static constexpr Hash Rock_4 = 1576173780;
        static constexpr Hash Rock_4_plains = -2040381836;
        static constexpr Hash Rock_7 = -1152709575;
        static constexpr Hash Rock_destructible = 112360646;
        static constexpr Hash Rock_destructible_test = -447136041;
        static constexpr Hash rock_mistlands1 = 1499269640;
        static constexpr Hash rock_mistlands1_frac = 33020421;
        static constexpr Hash rock_mistlands2 = 1499269639;
        static constexpr Hash rock1_mistlands = 1606082792;
        static constexpr Hash rock1_mountain = -1396233392;
        static constexpr Hash rock1_mountain_frac = -496513311;
        static constexpr Hash rock2_heath = -1865631054;
        static constexpr Hash rock2_heath_frac = -1268964283;
        static constexpr Hash rock2_mountain = -1397136109;
        static constexpr Hash rock2_mountain_frac = 1422273664;
        static constexpr Hash rock3_mountain = -1398465710;
        static constexpr Hash rock3_mountain_frac = 884909855;
        static constexpr Hash rock3_silver = 297427804;
        static constexpr Hash rock3_silver_frac = -990678761;
        static constexpr Hash rock4_coast = -790658592;
        static constexpr Hash rock4_coast_frac = 1039259597;
        static constexpr Hash rock4_copper = -1552092171;
        static constexpr Hash rock4_copper_frac = 912150108;
        static constexpr Hash rock4_forest = 1707947207;
        static constexpr Hash rock4_forest_frac = 1082202028;
        static constexpr Hash rock4_heath = -1865985940;
        static constexpr Hash rock4_heath_frac = -1655434869;
        static constexpr Hash RockDolmen_1 = 1396764178;
        static constexpr Hash RockDolmen_2 = 1800048705;
        static constexpr Hash RockDolmen_3 = 233964764;
        static constexpr Hash RockFinger = 571199914;
        static constexpr Hash RockFinger_frac = -171548199;
        static constexpr Hash RockFingerBroken = 414646403;
        static constexpr Hash RockFingerBroken_frac = 1140737056;
        static constexpr Hash rockformation1 = 1524190963;
        static constexpr Hash RockThumb = -1854838197;
        static constexpr Hash RockThumb_frac = -233084190;
        static constexpr Hash Root = -864298130;
        static constexpr Hash root07 = 144829813;
        static constexpr Hash root08 = -258454714;
        static constexpr Hash root11 = 951398868;
        static constexpr Hash root12 = 548114341;
        static constexpr Hash RottenMeat = -476284625;
        static constexpr Hash RoundLog = -413278252;
        static constexpr Hash RoyalJelly = -442216113;
        static constexpr Hash Ruby = 2052801890;
        static constexpr Hash rug_deer = 1761585513;
        static constexpr Hash rug_fur = 1569410630;
        static constexpr Hash rug_hare = 1472612417;
        static constexpr Hash rug_straw = -1194194392;
        static constexpr Hash rug_wolf = -698527959;
        static constexpr Hash SaddleLox = -671861700;
        static constexpr Hash Salad = 190225431;
        static constexpr Hash Sap = 696031002;
        static constexpr Hash sapling_barley = 967280542;
        static constexpr Hash sapling_carrot = 1780505228;
        static constexpr Hash sapling_flax = 1079411440;
        static constexpr Hash sapling_jotunpuffs = -989751415;
        static constexpr Hash sapling_magecap = 1585193021;
        static constexpr Hash sapling_onion = -364092232;
        static constexpr Hash sapling_seedcarrot = 219934075;
        static constexpr Hash sapling_seedonion = -1390589197;
        static constexpr Hash sapling_seedturnip = -1250573360;
        static constexpr Hash sapling_turnip = 310026689;
        static constexpr Hash Sausages = 1580132118;
        static constexpr Hash ScaleHide = -569089572;
        static constexpr Hash Seagal = 1469476749;
        static constexpr Hash Seeker = 515096851;
        static constexpr Hash SeekerAspic = 452860339;
        static constexpr Hash SeekerBrood = -851106125;
        static constexpr Hash SeekerBrute = -1516513665;
        static constexpr Hash SeekerBrute_Taunt = 789849948;
        static constexpr Hash SeekerEgg = 1515859210;
        static constexpr Hash SeekerEgg_alwayshatch = -63152736;
        static constexpr Hash SeekerQueen = -185712007;
        static constexpr Hash SeekerQueen_Call = -1828334774;
        static constexpr Hash SeekerQueen_projectile_spit = -1897578788;
        static constexpr Hash SeekerQueen_projectile_teleport = -1932604943;
        static constexpr Hash SeekerQueen_spithit = 1195333873;
        static constexpr Hash SeekerQueen_Teleport = 218841687;
        static constexpr Hash Serpent = 1671717323;
        static constexpr Hash SerpentMeat = -370330082;
        static constexpr Hash SerpentMeatCooked = 865616331;
        static constexpr Hash SerpentScale = -1569517817;
        static constexpr Hash SerpentStew = -1781955618;
        static constexpr Hash sfx_Abomination_attack = 318619260;
        static constexpr Hash sfx_Abomination_Attack2_slam_whoosh = -472153373;
        static constexpr Hash sfx_Abomination_swing = 85954002;
        static constexpr Hash sfx_arbalest_fire = -349627309;
        static constexpr Hash sfx_arrow_hit = 2116918345;
        static constexpr Hash sfx_atgeir_attack = -9823443;
        static constexpr Hash sfx_atgeir_attack_secondary = 1119253240;
        static constexpr Hash sfx_axe_flint_hit = -1785312200;
        static constexpr Hash sfx_axe_hit = 1674650800;
        static constexpr Hash sfx_axe_swing = 1525903873;
        static constexpr Hash sfx_barley_hit = 2017903409;
        static constexpr Hash sfx_barnacle_destroyed = -287013000;
        static constexpr Hash sfx_bat_alerted = -1056847255;
        static constexpr Hash sfx_bat_attack = 1641770234;
        static constexpr Hash sfx_bat_idle = -1204919892;
        static constexpr Hash sfx_battleaxe_hit = -282875050;
        static constexpr Hash sfx_battleaxe_swing_start = -445258974;
        static constexpr Hash sfx_battleaxe_swing_wosh = 1475499163;
        static constexpr Hash sfx_beehive_destroyed = 1742233836;
        static constexpr Hash sfx_beehive_hit = 1096880206;
        static constexpr Hash sfx_blob_alerted = 1632142469;
        static constexpr Hash sfx_blob_attack = 148104278;
        static constexpr Hash sfx_blob_death = -1615338232;
        static constexpr Hash sfx_blob_hit = 1859362075;
        static constexpr Hash sfx_blob_idle = 442506800;
        static constexpr Hash sfx_blob_jump = 139978274;
        static constexpr Hash sfx_blobtar_attack_spit = -1602192272;
        static constexpr Hash sfx_blobtar_idle = 679310017;
        static constexpr Hash sfx_boar_alerted = 1128738514;
        static constexpr Hash sfx_boar_attack = 2121021919;
        static constexpr Hash sfx_boar_birth = -287580596;
        static constexpr Hash sfx_boar_death = 1768214527;
        static constexpr Hash sfx_boar_hit = -2014915412;
        static constexpr Hash sfx_boar_idle = 601266887;
        static constexpr Hash sfx_boar_love = -2013304865;
        static constexpr Hash sfx_bomb_throw = 185328793;
        static constexpr Hash sfx_Bonemass_alert = 1294883521;
        static constexpr Hash sfx_Bonemass_death = 2046458303;
        static constexpr Hash sfx_Bonemass_Hit = 980760610;
        static constexpr Hash sfx_Bonemass_idle = -491656505;
        static constexpr Hash sfx_bonepile_destroyed = -1605529788;
        static constexpr Hash sfx_bones_pick = -643696903;
        static constexpr Hash sfx_bow_draw = -1901670101;
        static constexpr Hash sfx_bow_fire = -1888631953;
        static constexpr Hash sfx_bow_fire_silent = -2097698275;
        static constexpr Hash sfx_bowl_AddItem = -1515693285;
        static constexpr Hash sfx_build_cultivator = 1257287840;
        static constexpr Hash sfx_build_hammer_crystal = -1278010020;
        static constexpr Hash sfx_build_hammer_default = 2097750009;
        static constexpr Hash sfx_build_hammer_metal = 280157711;
        static constexpr Hash sfx_build_hammer_stone = -1311934539;
        static constexpr Hash sfx_build_hammer_wood = -2011991903;
        static constexpr Hash sfx_build_hoe = -699454731;
        static constexpr Hash sfx_bush_hit = -1065983654;
        static constexpr Hash sfx_cart_hit = 227256914;
        static constexpr Hash sfx_chest_close = -72591688;
        static constexpr Hash sfx_chest_open = -1129500684;
        static constexpr Hash sfx_chick_hurt = 1569470562;
        static constexpr Hash sfx_chicken_death = 1145854360;
        static constexpr Hash sfx_chicken_eat = -333526740;
        static constexpr Hash sfx_chicken_footstep = 1332483512;
        static constexpr Hash sfx_chicken_hurt = -1310422729;
        static constexpr Hash sfx_chicken_idle_vocal = 151773982;
        static constexpr Hash sfx_chicken_idle_wingflap = 1875429571;
        static constexpr Hash sfx_claw_swing = 296086892;
        static constexpr Hash sfx_club_hit = 1859348788;
        static constexpr Hash sfx_club_swing = 180601353;
        static constexpr Hash sfx_coins_destroyed = 956240876;
        static constexpr Hash sfx_coins_pile_destroyed = -1890597475;
        static constexpr Hash sfx_coins_pile_placed = -1022715103;
        static constexpr Hash sfx_coins_placed = 1987656624;
        static constexpr Hash sfx_cooking_station_burnt = 1597637767;
        static constexpr Hash sfx_cooking_station_done = -1771031886;
        static constexpr Hash sfx_cooking_station_take = -440219305;
        static constexpr Hash sfx_creature_consume = -331212374;
        static constexpr Hash sfx_crow_death = -442618672;
        static constexpr Hash sfx_deathsquito_attack = 1259899960;
        static constexpr Hash sfx_deer_alerted = -357044922;
        static constexpr Hash sfx_deer_death = 786261919;
        static constexpr Hash sfx_deer_idle = 184170615;
        static constexpr Hash sfx_demister_start = -163199496;
        static constexpr Hash sfx_dodge = 2120243119;
        static constexpr Hash sfx_dragon_alerted = -812720353;
        static constexpr Hash sfx_dragon_coldball_explode = -2106959121;
        static constexpr Hash sfx_dragon_coldball_launch = 448545629;
        static constexpr Hash sfx_dragon_coldball_start = -626665322;
        static constexpr Hash sfx_dragon_coldbreath_start = 2128287383;
        static constexpr Hash sfx_dragon_coldbreath_trailon = -238966068;
        static constexpr Hash sfx_dragon_death = 1090343548;
        static constexpr Hash sfx_dragon_hurt = -891344597;
        static constexpr Hash sfx_dragon_idle = -1650858748;
        static constexpr Hash sfx_dragon_melee_hit = -1804404888;
        static constexpr Hash sfx_dragon_melee_start = 1936230299;
        static constexpr Hash sfx_dragon_scream = 1061380211;
        static constexpr Hash sfx_dragonegg_destroy = 52560671;
        static constexpr Hash sfx_draugr_alerted = 1448680921;
        static constexpr Hash sfx_draugr_death = -1989076202;
        static constexpr Hash sfx_draugr_hit = -112770717;
        static constexpr Hash sfx_draugr_idle = -419194450;
        static constexpr Hash sfx_draugrpile_destroyed = -1876585691;
        static constexpr Hash sfx_draugrpile_hit = -920114813;
        static constexpr Hash sfx_DraugrSpawn = -1626280682;
        static constexpr Hash sfx_dverger_ball_start = 1027054650;
        static constexpr Hash sfx_dverger_fireball_rain_shot = -1476164013;
        static constexpr Hash sfx_dverger_fireball_rain_start = -2007471813;
        static constexpr Hash sfx_dverger_footsteps = 1029945383;
        static constexpr Hash sfx_dverger_heal_finish = 145545524;
        static constexpr Hash sfx_dverger_heal_start = 1623111531;
        static constexpr Hash sfx_dverger_heavyattack_launch = -441567607;
        static constexpr Hash sfx_dverger_ice_aoe_start = -2138868324;
        static constexpr Hash sfx_dverger_ice_projectile_start = 1851626712;
        static constexpr Hash sfx_dverger_staff_baseattack_foley = -490299558;
        static constexpr Hash sfx_dverger_staff_kolvattack_foley = 1587788081;
        static constexpr Hash sfx_dverger_staff_poke_foley = -1780822866;
        static constexpr Hash sfx_dverger_vo_alerted = 1577646117;
        static constexpr Hash sfx_dverger_vo_attack = 1236880372;
        static constexpr Hash sfx_dverger_vo_death = 801550882;
        static constexpr Hash sfx_dverger_vo_hurt = 133703909;
        static constexpr Hash sfx_dverger_vo_idle = 987327610;
        static constexpr Hash sfx_eat = -1371474352;
        static constexpr Hash sfx_eikthyr_alert = -1120079577;
        static constexpr Hash sfx_eikthyr_death = -138843163;
        static constexpr Hash sfx_eikthyr_hit = 789326860;
        static constexpr Hash sfx_eikthyr_idle = -1976095907;
        static constexpr Hash sfx_equip = 1387578946;
        static constexpr Hash sfx_equip_start = 1204475865;
        static constexpr Hash sfx_fenring_alerted = -1862261727;
        static constexpr Hash sfx_fenring_claw_hit = 342632397;
        static constexpr Hash sfx_fenring_claw_start = 1227596662;
        static constexpr Hash sfx_fenring_claw_trailstart = 590949502;
        static constexpr Hash sfx_fenring_death = 726801436;
        static constexpr Hash sfx_fenring_fireclaw = -780440161;
        static constexpr Hash sfx_fenring_howl = -1210864460;
        static constexpr Hash sfx_fenring_idle = 536788148;
        static constexpr Hash sfx_fenring_jump_start = -2025415501;
        static constexpr Hash sfx_fenring_jump_trailstart = 42596973;
        static constexpr Hash sfx_fenring_jump_trigger = -227508653;
        static constexpr Hash sfx_fermenter_add = -1504196720;
        static constexpr Hash sfx_fermenter_tap = -1907481662;
        static constexpr Hash sfx_FireAddFuel = -544513927;
        static constexpr Hash sfx_firestaff_launch = -91247242;
        static constexpr Hash sfx_fishingrod_linebreak = 769769393;
        static constexpr Hash sfx_fishingrod_swing = 1243570260;
        static constexpr Hash sfx_fist_metal_blocked = 1977532003;
        static constexpr Hash sfx_Frost_Start = 919660713;
        static constexpr Hash sfx_gdking_alert = -1849978965;
        static constexpr Hash sfx_gdking_death = 286111049;
        static constexpr Hash sfx_gdking_idle = 603150001;
        static constexpr Hash sfx_gdking_punch = -371166203;
        static constexpr Hash sfx_gdking_rock_destroyed = -2047078594;
        static constexpr Hash sfx_gdking_scream = -669580346;
        static constexpr Hash sfx_gdking_shoot_start = 1621948229;
        static constexpr Hash sfx_gdking_shoot_trigger = -1858890103;
        static constexpr Hash sfx_gdking_spawn = -2000700602;
        static constexpr Hash sfx_gdking_stomp = -1268036788;
        static constexpr Hash sfx_ghost_alert = -1797734528;
        static constexpr Hash sfx_ghost_attack = -698369758;
        static constexpr Hash sfx_ghost_attack_hit = 1668762708;
        static constexpr Hash sfx_ghost_death = -1293429652;
        static constexpr Hash sfx_ghost_hurt = 62237157;
        static constexpr Hash sfx_ghost_idle = 122846068;
        static constexpr Hash sfx_gjall_alerted = -232314874;
        static constexpr Hash sfx_gjall_attack_shake = -1948064278;
        static constexpr Hash sfx_gjall_attack_tick_drop = -192269159;
        static constexpr Hash sfx_gjall_death = -1919066995;
        static constexpr Hash sfx_gjall_idle_blowout = 862501002;
        static constexpr Hash sfx_gjall_idle_shiver = 1279745369;
        static constexpr Hash sfx_gjall_idle_vocals = 794354548;
        static constexpr Hash sfx_gjall_spit = 2119395747;
        static constexpr Hash sfx_gjall_spit_fx = 572773820;
        static constexpr Hash sfx_gjall_spit_impact = 1357883520;
        static constexpr Hash sfx_gjall_tick_drop = -1372830262;
        static constexpr Hash sfx_goblin_alerted = 957066015;
        static constexpr Hash sfx_goblin_death = -1484016104;
        static constexpr Hash sfx_goblin_hit = 92356725;
        static constexpr Hash sfx_goblin_idle = 298715040;
        static constexpr Hash sfx_goblinbrute_alerted = 209539363;
        static constexpr Hash sfx_goblinbrute_clubhit = -436006683;
        static constexpr Hash sfx_goblinbrute_clubswing = -790848394;
        static constexpr Hash sfx_goblinbrute_death = 1813676922;
        static constexpr Hash sfx_goblinbrute_hit = -1694032143;
        static constexpr Hash sfx_goblinking_beam = 1731195420;
        static constexpr Hash sfx_goblinking_taunt = 363874217;
        static constexpr Hash sfx_GoblinShaman_alerted = 324164379;
        static constexpr Hash sfx_GoblinShaman_death = -1202814254;
        static constexpr Hash sfx_GoblinShaman_fireball_launch = -512484755;
        static constexpr Hash sfx_GoblinShaman_hurt = -840257425;
        static constexpr Hash sfx_GoblinShaman_idle = 1888625450;
        static constexpr Hash sfx_greydwarf_alerted = 869677353;
        static constexpr Hash sfx_greydwarf_attack = 1675311800;
        static constexpr Hash sfx_greydwarf_attack_hit = -1632421286;
        static constexpr Hash sfx_greydwarf_death = -1447958386;
        static constexpr Hash sfx_greydwarf_elite_alerted = 1560640839;
        static constexpr Hash sfx_greydwarf_elite_attack = -1245172204;
        static constexpr Hash sfx_greydwarf_elite_death = -1424600366;
        static constexpr Hash sfx_greydwarf_elite_idle = -272788038;
        static constexpr Hash sfx_greydwarf_hit = 768797863;
        static constexpr Hash sfx_greydwarf_idle = 518810534;
        static constexpr Hash sfx_greydwarf_shaman_attack = 698141549;
        static constexpr Hash sfx_greydwarf_shaman_heal = -921150635;
        static constexpr Hash sfx_greydwarf_stone_hit = -1806562545;
        static constexpr Hash sfx_greydwarf_throw = -1038008944;
        static constexpr Hash sfx_greydwarfnest_destroyed = -2107531651;
        static constexpr Hash sfx_greydwarfnest_hit = 1009431615;
        static constexpr Hash sfx_greyling_alerted = 36140811;
        static constexpr Hash sfx_greyling_attack = -309914534;
        static constexpr Hash sfx_greyling_death = -160017500;
        static constexpr Hash sfx_greyling_hit = 933333521;
        static constexpr Hash sfx_greyling_idle = -1568237828;
        static constexpr Hash sfx_GuckSackDestroyed = 1741169793;
        static constexpr Hash sfx_GuckSackHit = -233871765;
        static constexpr Hash sfx_gui_craftitem = 362390805;
        static constexpr Hash sfx_gui_craftitem_cauldron = 1448794450;
        static constexpr Hash sfx_gui_craftitem_cauldron_end = -1871015160;
        static constexpr Hash sfx_gui_craftitem_end = -1488968663;
        static constexpr Hash sfx_gui_craftitem_forge = -1252768741;
        static constexpr Hash sfx_gui_craftitem_forge_end = -1845774265;
        static constexpr Hash sfx_gui_craftitem_workbench = 443759629;
        static constexpr Hash sfx_gui_craftitem_workbench_end = 879403129;
        static constexpr Hash sfx_gui_repairitem_forge = -1701226344;
        static constexpr Hash sfx_gui_repairitem_workbench = 922799634;
        static constexpr Hash sfx_haldor_greet = 35942974;
        static constexpr Hash sfx_haldor_laugh = -1346573826;
        static constexpr Hash sfx_haldor_yea = -111516482;
        static constexpr Hash sfx_hare_alerted = -590252532;
        static constexpr Hash sfx_hare_death_vocal = 766344711;
        static constexpr Hash sfx_hare_idle = 1121438395;
        static constexpr Hash sfx_hare_idle_eating = -1705482;
        static constexpr Hash sfx_hatchling_alerted = 1199232568;
        static constexpr Hash sfx_hatchling_coldball_explode = -1823965796;
        static constexpr Hash sfx_hatchling_coldball_launch = -1782709912;
        static constexpr Hash sfx_hatchling_coldball_start = 1547577083;
        static constexpr Hash sfx_hatchling_death = -558295565;
        static constexpr Hash sfx_hatchling_idle = -11215269;
        static constexpr Hash sfx_hatcling_hurt = -2057165472;
        static constexpr Hash sfx_hit = -1015244101;
        static constexpr Hash sfx_HiveQueen_acitspit = -427360716;
        static constexpr Hash sfx_HiveQueen_alerted = -115049582;
        static constexpr Hash sfx_HiveQueen_backslam = 747352637;
        static constexpr Hash sfx_HiveQueen_bite = 169548283;
        static constexpr Hash sfx_HiveQueen_burrow = -1850504928;
        static constexpr Hash sfx_HiveQueen_callout = 55346721;
        static constexpr Hash sfx_HiveQueen_doubleslash = -297293675;
        static constexpr Hash sfx_HiveQueen_idle = 1722335429;
        static constexpr Hash sfx_HiveQueen_move = 1856591514;
        static constexpr Hash sfx_HiveQueen_pierce = -1200378047;
        static constexpr Hash sfx_HiveQueen_rush = -764886347;
        static constexpr Hash sfx_HiveQueen_slash = 369873936;
        static constexpr Hash sfx_HiveQueen_slashcombo = -1819784222;
        static constexpr Hash sfx_HiveQueen_spitimpact = 400225113;
        static constexpr Hash sfx_HiveQueen_turn = -1927685696;
        static constexpr Hash sfx_ice_destroyed = 1212214977;
        static constexpr Hash sfx_ice_hit = 1099069099;
        static constexpr Hash sfx_icestaff_start = -958668374;
        static constexpr Hash sfx_imp_alerted = -228445012;
        static constexpr Hash sfx_imp_death = 518456693;
        static constexpr Hash sfx_imp_fireball_explode = 232946994;
        static constexpr Hash sfx_imp_fireball_launch = -310863970;
        static constexpr Hash sfx_imp_hit = 1565498190;
        static constexpr Hash sfx_jump = -1081772602;
        static constexpr Hash sfx_kiln_addore = 512263924;
        static constexpr Hash sfx_kiln_produce = -664898105;
        static constexpr Hash sfx_knife_swing = 21597614;
        static constexpr Hash sfx_kromsword_swing = -1364851665;
        static constexpr Hash sfx_land = 484569737;
        static constexpr Hash sfx_land_water = 988384127;
        static constexpr Hash sfx_leech_alerted = -1181261327;
        static constexpr Hash sfx_leech_attack = 1501365952;
        static constexpr Hash sfx_leech_attack_hit = -2045834926;
        static constexpr Hash sfx_leech_death = 1479879878;
        static constexpr Hash sfx_leech_hit = 578078319;
        static constexpr Hash sfx_leech_idle = 102110366;
        static constexpr Hash sfx_lootspawn = 2020511185;
        static constexpr Hash sfx_lox_alerted = -1863847711;
        static constexpr Hash sfx_lox_attack_bite = 468474163;
        static constexpr Hash sfx_lox_attack_stomp = -145504576;
        static constexpr Hash sfx_lox_shout = 2072191495;
        static constexpr Hash sfx_loxcalf_alerted = -469793791;
        static constexpr Hash sfx_MeadBurp = 1571728298;
        static constexpr Hash sfx_metal_blocked = 1124696762;
        static constexpr Hash sfx_metal_blocked_overlay = -1760283719;
        static constexpr Hash sfx_metal_shield_blocked = -1772608940;
        static constexpr Hash sfx_metal_shield_blocked_overlay = -840864991;
        static constexpr Hash sfx_mill_add = 2057247232;
        static constexpr Hash sfx_mill_produce = -1118938029;
        static constexpr Hash sfx_MudDestroyed = 821843025;
        static constexpr Hash sfx_MudHit = 1067687063;
        static constexpr Hash sfx_neck_alerted = 952841829;
        static constexpr Hash sfx_neck_attack = 43892854;
        static constexpr Hash sfx_neck_attack_hit = -2127668968;
        static constexpr Hash sfx_neck_death = -1277955800;
        static constexpr Hash sfx_neck_hit = 654439419;
        static constexpr Hash sfx_neck_idle = -775797488;
        static constexpr Hash sfx_obliterator_close = 1274852592;
        static constexpr Hash sfx_obliterator_open = -682457896;
        static constexpr Hash sfx_offering = -1982248368;
        static constexpr Hash sfx_oozebomb_explode = -1817721841;
        static constexpr Hash sfx_OpenPortal = 732280816;
        static constexpr Hash sfx_oven_burnt = -1005547168;
        static constexpr Hash sfx_oven_done = -1547861929;
        static constexpr Hash sfx_oven_take = 307453894;
        static constexpr Hash sfx_perfectblock = -937050156;
        static constexpr Hash sfx_pickable_pick = 309311199;
        static constexpr Hash sfx_pickaxe_hit = 1303862709;
        static constexpr Hash sfx_pickaxe_swing = -2079789852;
        static constexpr Hash sfx_Poison_Start = -719076567;
        static constexpr Hash sfx_Potion_eitr_minor = -332886724;
        static constexpr Hash sfx_Potion_frostresist_Start = -343832221;
        static constexpr Hash sfx_Potion_health_large = 861580176;
        static constexpr Hash sfx_Potion_health_medium = -477618694;
        static constexpr Hash sfx_Potion_health_minor = -1947863526;
        static constexpr Hash sfx_Potion_health_Start = 679702861;
        static constexpr Hash sfx_Potion_stamina_Start = 513203996;
        static constexpr Hash sfx_Potion_stamina_Start_lingering = -1880311490;
        static constexpr Hash sfx_Potion_stamina_Start_medium = -2058822514;
        static constexpr Hash sfx_prespawn = 482237526;
        static constexpr Hash sfx_ProjectileHit = 933664922;
        static constexpr Hash sfx_Puke_female = -326315518;
        static constexpr Hash sfx_Puke_male = -90428465;
        static constexpr Hash sfx_reload_done = 1969967132;
        static constexpr Hash sfx_reload_dverger_done = -1326462186;
        static constexpr Hash sfx_reload_dverger_start = -525378698;
        static constexpr Hash sfx_reload_start = -320719252;
        static constexpr Hash sfx_rock_destroyed = 443205179;
        static constexpr Hash sfx_rock_hit = 1659574845;
        static constexpr Hash sfx_rooster_idle = -345161545;
        static constexpr Hash sfx_secretfound = 1862739074;
        static constexpr Hash sfx_secretfound_1 = -1529198509;
        static constexpr Hash sfx_serpent_alerted = -1937784263;
        static constexpr Hash sfx_serpent_attack = 1720244614;
        static constexpr Hash sfx_serpent_attack_hit = -53659712;
        static constexpr Hash sfx_serpent_attack_trigger = 1664749613;
        static constexpr Hash sfx_serpent_death = 664835072;
        static constexpr Hash sfx_serpent_hurt = 1296799745;
        static constexpr Hash sfx_serpent_idle = -1452028264;
        static constexpr Hash sfx_serpent_taunt = -1775091842;
        static constexpr Hash sfx_ship_destroyed = -1123631012;
        static constexpr Hash sfx_ship_impact = 108601737;
        static constexpr Hash sfx_silvermace_hit = 101705497;
        static constexpr Hash sfx_skeleton_alerted = -779148385;
        static constexpr Hash sfx_skeleton_attack = 1471010406;
        static constexpr Hash sfx_skeleton_big_alerted = -946127216;
        static constexpr Hash sfx_skeleton_big_death = -2047632425;
        static constexpr Hash sfx_skeleton_death = -632309576;
        static constexpr Hash sfx_skeleton_hit = 1166104533;
        static constexpr Hash sfx_skeleton_idle = -199334624;
        static constexpr Hash sfx_skeleton_mace_hit = -134414446;
        static constexpr Hash sfx_skeleton_rise = 782154305;
        static constexpr Hash sfx_skull_summon_skeleton = 1137816861;
        static constexpr Hash sfx_sledge_hit = 1234819210;
        static constexpr Hash sfx_sledge_iron_hit = 1218133189;
        static constexpr Hash sfx_sledge_swing = 1025052565;
        static constexpr Hash sfx_smelter_add = -1187773206;
        static constexpr Hash sfx_smelter_produce = 455267851;
        static constexpr Hash sfx_spawn = 1622864505;
        static constexpr Hash sfx_spear_flint_hit = -98825973;
        static constexpr Hash sfx_spear_hit = -639458691;
        static constexpr Hash sfx_spear_poke = 1418593097;
        static constexpr Hash sfx_spear_throw = -433840220;
        static constexpr Hash sfx_stonegolem_alerted = -1300399715;
        static constexpr Hash sfx_stonegolem_attack_hit = -56190528;
        static constexpr Hash sfx_stonegolem_attack_wosh = -573443490;
        static constexpr Hash sfx_stonegolem_death = -625942156;
        static constexpr Hash sfx_stonegolem_hurt = 1697953989;
        static constexpr Hash sfx_stonegolem_idle = 299883260;
        static constexpr Hash sfx_stonegolem_primary_start = -122588711;
        static constexpr Hash sfx_stonegolem_second_start = 1405284141;
        static constexpr Hash sfx_stonegolem_spikeattack_trailon = 1969679954;
        static constexpr Hash sfx_stonegolem_wakeup = 252173315;
        static constexpr Hash sfx_sword_hit = -908234027;
        static constexpr Hash sfx_sword_swing = 805484646;
        static constexpr Hash sfx_tentaroot_attack = 1547379821;
        static constexpr Hash sfx_tick_alerted = -235098033;
        static constexpr Hash sfx_tick_attack_drain = -224115537;
        static constexpr Hash sfx_tick_attack_jump = 1623629049;
        static constexpr Hash sfx_tick_hurt = -1349701893;
        static constexpr Hash sfx_tick_idle = 1735411342;
        static constexpr Hash sfx_torch_swing = -761428885;
        static constexpr Hash sfx_treasurechest_destroyed = 1141039220;
        static constexpr Hash sfx_tree_fall = -1874475854;
        static constexpr Hash sfx_tree_fall_abomination = -1256552928;
        static constexpr Hash sfx_tree_fall_hit = 990991588;
        static constexpr Hash sfx_tree_hit = 724468252;
        static constexpr Hash sfx_tree_hit_abomination = 1701579258;
        static constexpr Hash sfx_troll_alerted = 1062930251;
        static constexpr Hash sfx_troll_attack_hit = 236295284;
        static constexpr Hash sfx_troll_attacking = -524195358;
        static constexpr Hash sfx_troll_death = 1550152080;
        static constexpr Hash sfx_troll_hit = -919278487;
        static constexpr Hash sfx_troll_idle = 2138806728;
        static constexpr Hash sfx_troll_rock_destroyed = 108229521;
        static constexpr Hash sfx_ulv_death = -971560874;
        static constexpr Hash sfx_unarmed_blocked = -161316095;
        static constexpr Hash sfx_unarmed_hit = 1606312494;
        static constexpr Hash sfx_unarmed_swing = 715807293;
        static constexpr Hash sfx_UndeadBurn_Start = 406146685;
        static constexpr Hash sfx_WishbonePing_closer = -1045975014;
        static constexpr Hash sfx_WishbonePing_far = -1698132249;
        static constexpr Hash sfx_WishbonePing_further = -937629098;
        static constexpr Hash sfx_WishbonePing_med = 1534792178;
        static constexpr Hash sfx_WishbonePing_near = 393316638;
        static constexpr Hash sfx_wolf_alerted = -960574442;
        static constexpr Hash sfx_wolf_attack = 523244451;
        static constexpr Hash sfx_wolf_attack_hit = 163230065;
        static constexpr Hash sfx_wolf_birth = -1174064648;
        static constexpr Hash sfx_wolf_death = -1803198157;
        static constexpr Hash sfx_wolf_hit = -1555397680;
        static constexpr Hash sfx_wolf_love = 912084459;
        static constexpr Hash sfx_wood_blocked = -602397748;
        static constexpr Hash sfx_wood_blocked_overlay = -576574677;
        static constexpr Hash sfx_wood_break = 102817;
        static constexpr Hash sfx_wood_destroyed = -1560241923;
        static constexpr Hash sfx_wood_hit = -1717541353;
        static constexpr Hash sfx_wraith_alerted = -1928641073;
        static constexpr Hash sfx_wraith_attack = 711249930;
        static constexpr Hash sfx_wraith_attack_hit = 965283228;
        static constexpr Hash sfx_wraith_death = -1778014004;
        static constexpr Hash sfx_wraith_hit = 901120601;
        static constexpr Hash sfx_wraith_idle = 944718372;
        static constexpr Hash shaman_attack_aoe = 1961161115;
        static constexpr Hash shaman_heal_aoe = 1063409303;
        static constexpr Hash SharpeningStone = -750899644;
        static constexpr Hash ShieldBanded = 616678875;
        static constexpr Hash ShieldBlackmetal = -417136115;
        static constexpr Hash ShieldBlackmetalTower = -1283608710;
        static constexpr Hash ShieldBoneTower = 387580618;
        static constexpr Hash ShieldBronzeBuckler = 831128417;
        static constexpr Hash ShieldCarapace = -2124075815;
        static constexpr Hash ShieldCarapaceBuckler = -1416094899;
        static constexpr Hash ShieldIronBuckler = -1704092447;
        static constexpr Hash ShieldIronSquare = -66427558;
        static constexpr Hash ShieldIronTower = 1602392070;
        static constexpr Hash ShieldKnight = -1748101970;
        static constexpr Hash ShieldSerpentscale = -344082900;
        static constexpr Hash ShieldSilver = -1248409586;
        static constexpr Hash ShieldWood = 897989326;
        static constexpr Hash ShieldWoodTower = 1729114779;
        static constexpr Hash ship_construction = -1245442852; // seems to be an unused prefab (literally ship being constructed, spawns a ship after a while)
        static constexpr Hash shipwreck_karve_bottomboards = 121363955;
        static constexpr Hash shipwreck_karve_bow = -2111136205;
        static constexpr Hash shipwreck_karve_chest = -759801912;
        static constexpr Hash shipwreck_karve_dragonhead = 1103509118;
        static constexpr Hash shipwreck_karve_stern = -1183306829;
        static constexpr Hash shipwreck_karve_sternpost = -849022257;
        static constexpr Hash ShocklateSmoothie = -1079051988;
        static constexpr Hash ShootStump = -1558885352;
        static constexpr Hash shrub_2 = -1779355401;
        static constexpr Hash shrub_2_heath = -199539902;
        static constexpr Hash sign = 1084866395;
        static constexpr Hash sign_notext = 1316803472;
        static constexpr Hash Silver = -1906091609;
        static constexpr Hash SilverNecklace = -726203417;
        static constexpr Hash SilverOre = 2105582487;
        static constexpr Hash silvervein = 1611466255;
        static constexpr Hash silvervein_frac = 65008322;
        static constexpr Hash Skeleton = -1035090735;
        static constexpr Hash skeleton_bow = -955026550;
        static constexpr Hash skeleton_bow2 = -591647588;
        static constexpr Hash Skeleton_Friendly = 885507349;
        static constexpr Hash skeleton_mace = 598002992;
        static constexpr Hash Skeleton_NoArcher = 392606230;
        static constexpr Hash Skeleton_Poison = -1723003302;
        static constexpr Hash skeleton_sword = 141245155;
        static constexpr Hash skeleton_sword2 = -752145615;
        static constexpr Hash Skull1 = 757858766;
        static constexpr Hash Skull2 = 354574239;
        static constexpr Hash sledge_aoe = -8548736;
        static constexpr Hash SledgeCheat = 175903691;
        static constexpr Hash SledgeDemolisher = 427268604;
        static constexpr Hash SledgeIron = -228048586;
        static constexpr Hash SledgeStagbreaker = -698896327;
        static constexpr Hash smelter = 900941850;
        static constexpr Hash Softtissue = -20868409;
        static constexpr Hash Spawner_Bat = -1836156358;
        static constexpr Hash Spawner_Blob = 959730964;
        static constexpr Hash Spawner_BlobElite = -241244907;
        static constexpr Hash Spawner_BlobTar = -2140639629;
        static constexpr Hash Spawner_BlobTar_respawn_30 = 1490211126;
        static constexpr Hash Spawner_Boar = 1571435137;
        static constexpr Hash Spawner_Chicken = -1765986286;
        static constexpr Hash Spawner_Cultist = -1600033845;
        static constexpr Hash Spawner_Draugr = 1620622954;
        static constexpr Hash Spawner_Draugr_Elite = -2140484606;
        static constexpr Hash Spawner_Draugr_Noise = -1294801227;
        static constexpr Hash Spawner_Draugr_Ranged = 521057400;
        static constexpr Hash Spawner_Draugr_Ranged_Noise = -955292439;
        static constexpr Hash Spawner_Draugr_respawn_30 = 1006477195;
        static constexpr Hash Spawner_DraugrPile = -399756230;
        static constexpr Hash Spawner_DvergerArbalest = -1855532330;
        static constexpr Hash Spawner_DvergerMage = 726868954;
        static constexpr Hash Spawner_DvergerRandom = -1799783159;
        static constexpr Hash Spawner_Fenring = 705538066;
        static constexpr Hash Spawner_Fish4 = -524222393;
        static constexpr Hash Spawner_Ghost = -2043251898;
        static constexpr Hash Spawner_Goblin = -253282352;
        static constexpr Hash Spawner_GoblinArcher = -1332276175;
        static constexpr Hash Spawner_GoblinBrute = 1838267156;
        static constexpr Hash Spawner_GoblinShaman = -40747638;
        static constexpr Hash Spawner_Greydwarf = 544574264;
        static constexpr Hash Spawner_Greydwarf_Elite = 1287973336;
        static constexpr Hash Spawner_Greydwarf_Shaman = -1711995613;
        static constexpr Hash Spawner_GreydwarfNest = -2136750990;
        static constexpr Hash Spawner_Hatchling = -1720584471;
        static constexpr Hash Spawner_Hen = 133212030;
        static constexpr Hash Spawner_imp = 489441179;
        static constexpr Hash Spawner_imp_respawn = -778538652;
        static constexpr Hash Spawner_Leech_cave = -1702254152;
        static constexpr Hash Spawner_Location_Elite = -1825512432;
        static constexpr Hash Spawner_Location_Greydwarf = 1020298526;
        static constexpr Hash Spawner_Location_Shaman = 233788309;
        static constexpr Hash Spawner_Seeker = 722088364;
        static constexpr Hash Spawner_SeekerBrute = 408415796;
        static constexpr Hash Spawner_Skeleton = -346964912;
        static constexpr Hash Spawner_Skeleton_night_noarcher = 2087336966;
        static constexpr Hash Spawner_Skeleton_poison = -1948581159;
        static constexpr Hash Spawner_Skeleton_respawn_30 = 81405021;
        static constexpr Hash Spawner_StoneGolem = 710649036;
        static constexpr Hash Spawner_Tick = 1463772894;
        static constexpr Hash Spawner_Tick_stared = 1317363426;
        static constexpr Hash Spawner_Troll = -275632390;
        static constexpr Hash Spawner_Ulv = -1076640884;
        static constexpr Hash Spawner_Wraith = -268438304;
        static constexpr Hash SpearBronze = -1821145625;
        static constexpr Hash SpearCarapace = -1808454157;
        static constexpr Hash SpearChitin = 678850310;
        static constexpr Hash SpearElderbark = -1438354023;
        static constexpr Hash SpearFlint = -1352659250;
        static constexpr Hash SpearWolfFang = 228563121;
        static constexpr Hash staff_fireball_projectile = 2115010970;
        static constexpr Hash staff_iceshard_projectile = 1548540202;
        static constexpr Hash staff_shield_aoe = -1506685424;
        static constexpr Hash staff_skeleton_projectile = -1006706824;
        static constexpr Hash StaffFireball = -1657298159;
        static constexpr Hash StaffIceShards = 965145214;
        static constexpr Hash StaffShield = -1345985635;
        static constexpr Hash StaffSkeleton = -1318346397;
        static constexpr Hash stake_wall = 13326225;
        static constexpr Hash StaminaUpgrade_Greydwarf = -1439240287;
        static constexpr Hash StaminaUpgrade_Troll = -2070050825;
        static constexpr Hash StaminaUpgrade_Wraith = -1764358509;
        static constexpr Hash StatueCorgi = -1452599006;
        static constexpr Hash StatueDeer = 222646934;
        static constexpr Hash StatueEvil = 726946320;
        static constexpr Hash StatueHare = 1936541740;
        static constexpr Hash StatueSeed = 128538777;
        static constexpr Hash Stone = -535531945;
        static constexpr Hash stone_arch = -542996224;
        static constexpr Hash stone_floor = 1135745654;
        static constexpr Hash stone_floor_2x2 = 2048610715;
        static constexpr Hash stone_pile = -1094350308;
        static constexpr Hash stone_pillar = -1747173676;
        static constexpr Hash stone_stair = 389771597;
        static constexpr Hash stone_wall_1x1 = -1764426495;
        static constexpr Hash stone_wall_2x1 = -1623263994;
        static constexpr Hash stone_wall_4x2 = -2066954473;
        static constexpr Hash stoneblock_fracture = -714671739;
        static constexpr Hash stonechest = -1478464920;
        static constexpr Hash StoneGolem = 1814827443;
        static constexpr Hash stonegolem_attack1_spike = -1408658120;
        static constexpr Hash StoneGolem_clubs = -1360440141;
        static constexpr Hash StoneGolem_hat = -877622719;
        static constexpr Hash Stonegolem_ragdoll = -1554331797;
        static constexpr Hash StoneGolem_spikes = -856244001;
        static constexpr Hash stubbe = 595427151;
        static constexpr Hash stubbe_spawner = 1515889972;
        static constexpr Hash sunken_crypt_gate = -339866909;
        static constexpr Hash Surtling = 1370511288;
        static constexpr Hash SurtlingCore = -905433289;
        static constexpr Hash SwampTree1 = 2111235755;
        static constexpr Hash SwampTree1_log = 998340100;
        static constexpr Hash SwampTree1_Stub = -142007914;
        static constexpr Hash SwampTree2 = 1707951228;
        static constexpr Hash SwampTree2_darkland = 2052455688;
        static constexpr Hash SwampTree2_log = 270257971;
        static constexpr Hash SwordBlackmetal = 660487747;
        static constexpr Hash SwordBronze = 995331031;
        static constexpr Hash SwordCheat = 1586116462;
        static constexpr Hash SwordIron = -110263489;
        static constexpr Hash SwordIronFire = -114512825;
        static constexpr Hash SwordMistwalker = -22010056;
        static constexpr Hash SwordSilver = 891097842;
        static constexpr Hash Tankard = -430432277;
        static constexpr Hash Tankard_dvergr = -676449504;
        static constexpr Hash TankardAnniversary = 1115535871;
        static constexpr Hash TankardOdin = -467580905;
        static constexpr Hash Tar = 696030903;
        static constexpr Hash TarLiquid = 920931787;
        static constexpr Hash tarlump1 = -1406277430;
        static constexpr Hash tarlump1_frac = 2137671655;
        static constexpr Hash TentaRoot = 326210194;
        static constexpr Hash tentaroot_attack = 2038061211;
        static constexpr Hash TheHive = 1340509431;
        static constexpr Hash Thistle = -1711101149;
        static constexpr Hash THSwordKrom = 1891654770;
        static constexpr Hash Thunderstone = -1340310877;
        static constexpr Hash Tick = 325352405;
        static constexpr Hash Tin = 339800571;
        static constexpr Hash TinOre = 1742893963;
        static constexpr Hash tolroko_flyer = -1028130263;
        static constexpr Hash Torch = 795277336;
        static constexpr Hash TorchMist = -671973751;
        static constexpr Hash trader_wagon_destructable = 316161168;
        static constexpr Hash Trailership = 182695579;
        static constexpr Hash TrainingDummy = 892476008;
        static constexpr Hash treasure_pile = -2052101304;
        static constexpr Hash treasure_stack = -109499528;
        static constexpr Hash TreasureChest_blackforest = 1673474691;
        static constexpr Hash TreasureChest_dvergr_loose_stone = 1535347102;
        static constexpr Hash TreasureChest_dvergrtower = 1008822310;
        static constexpr Hash TreasureChest_dvergrtown = -819997397;
        static constexpr Hash TreasureChest_fCrypt = 1396451807;
        static constexpr Hash TreasureChest_forestcrypt = -494038940;
        static constexpr Hash TreasureChest_heath = -1283895483;
        static constexpr Hash TreasureChest_meadows = 600954715;
        static constexpr Hash TreasureChest_meadows_buried = 1182558977;
        static constexpr Hash TreasureChest_mountaincave = 637000287;
        static constexpr Hash TreasureChest_mountains = -161144371;
        static constexpr Hash TreasureChest_plains_stone = -922407394;
        static constexpr Hash TreasureChest_sunkencrypt = -190638207;
        static constexpr Hash TreasureChest_swamp = 887247677;
        static constexpr Hash TreasureChest_trollcave = 901211521;
        static constexpr Hash TriggerSpawner_Brood = -116387065;
        static constexpr Hash TriggerSpawner_Seeker = 343158804;
        static constexpr Hash Troll = 425751481;
        static constexpr Hash troll_groundslam_aoe = 1705853408;
        static constexpr Hash troll_log_swing_h = 724671782;
        static constexpr Hash troll_log_swing_v = 724671788;
        static constexpr Hash Troll_ragdoll = -1300355707;
        static constexpr Hash troll_throw_projectile = -1742068936;
        static constexpr Hash TrollHide = -1798613647;
        static constexpr Hash TrophyAbomination = 950444395;
        static constexpr Hash TrophyBlob = 744168779;
        static constexpr Hash TrophyBoar = 434992580;
        static constexpr Hash TrophyBonemass = -823608926;
        static constexpr Hash TrophyCultist = -275461056;
        static constexpr Hash TrophyDeathsquito = -178752771;
        static constexpr Hash TrophyDeer = 1281967652;
        static constexpr Hash TrophyDragonQueen = 1777024751;
        static constexpr Hash TrophyDraugr = 255610059;
        static constexpr Hash TrophyDraugrElite = 224753756;
        static constexpr Hash TrophyDraugrFem = 2113459109;
        static constexpr Hash TrophyDvergr = 388316152;
        static constexpr Hash TrophyEikthyr = -210135580;
        static constexpr Hash TrophyFenring = -1862041229;
        static constexpr Hash TrophyForestTroll = 1490357724;
        static constexpr Hash TrophyFrostTroll = 1334057821;
        static constexpr Hash TrophyGjall = 1951121652;
        static constexpr Hash TrophyGoblin = -1910628999;
        static constexpr Hash TrophyGoblinBrute = -1059419183;
        static constexpr Hash TrophyGoblinKing = -761824688;
        static constexpr Hash TrophyGoblinShaman = -1390649375;
        static constexpr Hash TrophyGreydwarf = 1438359047;
        static constexpr Hash TrophyGreydwarfBrute = 520692315;
        static constexpr Hash TrophyGreydwarfShaman = -1167222293;
        static constexpr Hash TrophyGrowth = -1545751767;
        static constexpr Hash TrophyHare = 730872782;
        static constexpr Hash TrophyHatchling = 1489900578;
        static constexpr Hash TrophyLeech = 2011746767;
        static constexpr Hash TrophyLox = 1418882551;
        static constexpr Hash TrophyNeck = -640346581;
        static constexpr Hash TrophySeeker = -1531883793;
        static constexpr Hash TrophySeekerBrute = -288469621;
        static constexpr Hash TrophySeekerQueen = 222489465;
        static constexpr Hash TrophySerpent = -659296385;
        static constexpr Hash TrophySGolem = 394875495;
        static constexpr Hash TrophySkeleton = 446596857;
        static constexpr Hash TrophySkeletonPoison = 319457105;
        static constexpr Hash TrophySurtling = 1253526026;
        static constexpr Hash TrophyTheElder = -7767225;
        static constexpr Hash TrophyTick = -1917457907;
        static constexpr Hash TrophyUlv = -1310001585;
        static constexpr Hash TrophyWolf = 1691899648;
        static constexpr Hash TrophyWraith = 1821410901;
        static constexpr Hash tunnel_web = 1473935011;
        static constexpr Hash turf_roof = -174377588;
        static constexpr Hash turf_roof_top = -1994750376;
        static constexpr Hash turf_roof_wall = -2117703061;
        static constexpr Hash Turnip = -1615314128;
        static constexpr Hash TurnipSeeds = 1883826970;
        static constexpr Hash TurnipStew = 131796573;
        static constexpr Hash Turret_projectile = 2021006706;
        static constexpr Hash TurretBolt = 1300114313;
        static constexpr Hash TurretBoltWood = -1340577042;
        static constexpr Hash Ulv = -63484205;
        static constexpr Hash Ulv_attack1_bite = -2106399042;
        static constexpr Hash Ulv_attack2_slash = -1857559904;
        static constexpr Hash Ulv_Ragdoll = 2013076875;
        static constexpr Hash Valkyrie = 1321415847;
        static constexpr Hash VegvisirShard_Bonemass = 2016758402;
        static constexpr Hash vertical_web = 547607065;
        static constexpr Hash vfx_arbalest_fire = 2034696918;
        static constexpr Hash vfx_arrowhit = -1733578089;
        static constexpr Hash vfx_auto_pickup = 1066770835;
        static constexpr Hash vfx_barley_destroyed = 1316049866;
        static constexpr Hash vfx_barnacle_destroyed = 1495888243;
        static constexpr Hash vfx_barnacle_hit = -365758259;
        static constexpr Hash vfx_barrle_destroyed = 331770029;
        static constexpr Hash vfx_beech_cut = 468348387;
        static constexpr Hash vfx_beech_small1_destroy = -127434936;
        static constexpr Hash vfx_beech_small2_destroy = 510000429;
        static constexpr Hash vfx_beechlog_destroyed = -35451204;
        static constexpr Hash vfx_beechlog_half_destroyed = -317910816;
        static constexpr Hash vfx_beehive_destroyed = -470808985;
        static constexpr Hash vfx_beehive_hit = 976946033;
        static constexpr Hash vfx_birch1_aut_cut = 859572588;
        static constexpr Hash vfx_birch1_cut = -1147555287;
        static constexpr Hash vfx_birch2_aut_cut = -851096239;
        static constexpr Hash vfx_birch2_cut = 1055017368;
        static constexpr Hash vfx_blastfurance_addfuel = 1343538387;
        static constexpr Hash vfx_blastfurnace_addore = 984877513;
        static constexpr Hash vfx_blastfurnace_produce = 2028884154;
        static constexpr Hash vfx_blob_attack = 18698171;
        static constexpr Hash vfx_blob_death = 593299331;
        static constexpr Hash vfx_blob_hit = 1665989430;
        static constexpr Hash vfx_blobelite_attack = 1436176688;
        static constexpr Hash vfx_blobtar_death = 145857790;
        static constexpr Hash vfx_blobtar_hit = 382852373;
        static constexpr Hash vfx_blocked = -886131209;
        static constexpr Hash vfx_BloodDeath = 1874304749;
        static constexpr Hash vfx_BloodHit = -512952038;
        static constexpr Hash vfx_boar_birth = 1921065415;
        static constexpr Hash vfx_boar_death = -318107006;
        static constexpr Hash vfx_boar_hit = 2086679503;
        static constexpr Hash vfx_boar_love = 195340900;
        static constexpr Hash vfx_BonemassDeath = -1941020771;
        static constexpr Hash vfx_BonemassHit = -1564202362;
        static constexpr Hash vfx_bonepile_destroyed = -1211919799;
        static constexpr Hash vfx_bones_pick = 1565156094;
        static constexpr Hash vfx_bonfire_AddFuel = 1894220146;
        static constexpr Hash vfx_bonfire_destroyed = -904512894;
        static constexpr Hash vfx_bow_fire = -2082004790;
        static constexpr Hash vfx_bowl_AddItem = -1645099656;
        static constexpr Hash vfx_Burning = -404786718;
        static constexpr Hash vfx_bush_destroyed = 608518267;
        static constexpr Hash vfx_bush_destroyed_heath = 358887432;
        static constexpr Hash vfx_bush_leaf_puff = 1138218448;
        static constexpr Hash vfx_bush_leaf_puff_heath = -2056072157;
        static constexpr Hash vfx_bush2_e_hit = 1106775641;
        static constexpr Hash vfx_bush2_en_destroyed = 1659973233;
        static constexpr Hash vfx_cartograpertable_write = -1811143589;
        static constexpr Hash vfx_cloth_hanging_destroyed = -395953198;
        static constexpr Hash vfx_clubhit = 1224459144;
        static constexpr Hash vfx_CoalDestroyed = -1724176891;
        static constexpr Hash vfx_CoalHit = 1378946595;
        static constexpr Hash vfx_coin_pile_destroyed = 625439651;
        static constexpr Hash vfx_coin_stack_destroyed = 1245329977;
        static constexpr Hash vfx_Cold = 121256471;
        static constexpr Hash vfx_ColdBall_Hit = 1008503002;
        static constexpr Hash vfx_ColdBall_launch = 1718661266;
        static constexpr Hash vfx_cooking_station_transform = -1234239539;
        static constexpr Hash vfx_corpse_destruction_large = 1848449852;
        static constexpr Hash vfx_corpse_destruction_medium = 510941886;
        static constexpr Hash vfx_corpse_destruction_small = 1555670196;
        static constexpr Hash vfx_crate_destroyed = 624138058;
        static constexpr Hash vfx_creature_soothed = -416794571;
        static constexpr Hash vfx_creep_hangingdetroyed = 1741667805;
        static constexpr Hash vfx_crow_death = 1768661069;
        static constexpr Hash vfx_damaged_cart = -284562001;
        static constexpr Hash vfx_Damaged_Karve = -1049460462;
        static constexpr Hash vfx_Damaged_Raft = 884897364;
        static constexpr Hash vfx_Damaged_VikingShip = -2045988123;
        static constexpr Hash vfx_darkland_groundfog = -1896715824;
        static constexpr Hash vfx_deer_death = -1290868190;
        static constexpr Hash vfx_deer_hit = -1914944657;
        static constexpr Hash vfx_Destroyed_Karve = 168794906;
        static constexpr Hash vfx_Destroyed_Raft = -2044113848;
        static constexpr Hash vfx_Destroyed_VikingShip = 1181203005;
        static constexpr Hash vfx_dragon_coldbreath = 1034819573;
        static constexpr Hash vfx_dragon_death = 1273669409;
        static constexpr Hash vfx_dragon_hurt = -708018736;
        static constexpr Hash vfx_dragon_ice_hit = 1662860094;
        static constexpr Hash vfx_dragonegg_destroy = 2102462628;
        static constexpr Hash vfx_draugr_death = -1805750341;
        static constexpr Hash vfx_draugr_hit = 2105343528;
        static constexpr Hash vfx_draugrpile_destroyed = -786333438;
        static constexpr Hash vfx_draugrpile_hit = 836768320;
        static constexpr Hash vfx_DraugrSpawn = -1452023749;
        static constexpr Hash vfx_dvergpost_destroyed = 1343642985;
        static constexpr Hash vfx_dvergr_beam_destroyed = 1526019331;
        static constexpr Hash vfx_dvergr_curtain_banner_destroyed = -1714554851;
        static constexpr Hash vfx_dvergr_curtain_banner_destroyed_horisontal = -1998342887;
        static constexpr Hash vfx_dvergr_pole_destroyed = -434038054;
        static constexpr Hash vfx_dvergr_stake_destroyed = 1383713634;
        static constexpr Hash vfx_dvergr_stakewall_destroyed = 892716570;
        static constexpr Hash vfx_dvergr_wood_wall02_destroyed = 574648488;
        static constexpr Hash vfx_dvergr_wood_wall03_destroyed = 635991149;
        static constexpr Hash vfx_dvergr_wood_wall04_destroyed = 172353202;
        static constexpr Hash vfx_dvergrchair_destroyed = 1637567696;
        static constexpr Hash vfx_dvergrcreep_beam_destroyed = -757807274;
        static constexpr Hash vfx_dvergrcreep_pole_destroyed = 1852603155;
        static constexpr Hash vfx_dvergrcreep_support_destroyed = -732360912;
        static constexpr Hash vfx_dvergrcreep_wood_wall02_destroyed = 396597529;
        static constexpr Hash vfx_dvergrcreep_wood_wall03_destroyed = 433641784;
        static constexpr Hash vfx_dvergrstool_destroyed = 1265877422;
        static constexpr Hash vfx_dvergrtable_destroyed = -158022795;
        static constexpr Hash vfx_edge_clouds = -602293851;
        static constexpr Hash vfx_eikthyr_death = -87169824;
        static constexpr Hash vfx_fenring_cultist_death = -12616560;
        static constexpr Hash vfx_fenring_death = 2101883415;
        static constexpr Hash vfx_fenring_hurt = -837531426;
        static constexpr Hash vfx_fenrirhide_hanging_destroyed = -540699250;
        static constexpr Hash vfx_fermenter_add = 174557517;
        static constexpr Hash vfx_fermenter_tap = -228727875;
        static constexpr Hash vfx_fir_oldlog = 550787196;
        static constexpr Hash vfx_FireAddFuel = -59626026;
        static constexpr Hash vfx_FireballHit = -1734590895;
        static constexpr Hash vfx_firetree_regrow = 1403184098;
        static constexpr Hash vfx_firetreecut = -1699433389;
        static constexpr Hash vfx_firetreecut_dead = -139036264;
        static constexpr Hash vfx_firetreecut_dead_abomination = -127726790;
        static constexpr Hash vfx_firlogdestroyed = 151805409;
        static constexpr Hash vfx_firlogdestroyed_half = 1880682397;
        static constexpr Hash vfx_foresttroll_hit = 1497395801;
        static constexpr Hash vfx_ForgeAddFuel = 1337401413;
        static constexpr Hash vfx_Freezing = 982281717;
        static constexpr Hash vfx_Frost = -990890393;
        static constexpr Hash vfx_frostarrow_hit = -225851562;
        static constexpr Hash vfx_frosttroll_hit = -327544276;
        static constexpr Hash vfx_gdking_stomp = -1006512207;
        static constexpr Hash vfx_ghost_death = -1040762809;
        static constexpr Hash vfx_ghost_hit = -565995904;
        static constexpr Hash vfx_GiantMetal_destroyed = -749809465;
        static constexpr Hash vfx_gjall_spit = 44635296;
        static constexpr Hash vfx_goblin_death = -1229125003;
        static constexpr Hash vfx_goblin_hit = -1982327694;
        static constexpr Hash vfx_goblin_woodwall_destroyed = -2075528908;
        static constexpr Hash vfx_goblinbrute_death = -1461997707;
        static constexpr Hash vfx_goblinbrute_hit = 939866254;
        static constexpr Hash vfx_goblinking_beam_OLD = 580593833;
        static constexpr Hash vfx_GoblinShield = 354834569;
        static constexpr Hash vfx_GodExplosion = -811131418;
        static constexpr Hash vfx_greydwarf_death = 1697016051;
        static constexpr Hash vfx_greydwarf_elite_death = 1673835157;
        static constexpr Hash vfx_greydwarf_hit = 213347242;
        static constexpr Hash vfx_greydwarf_root_destroyed = 1210601465;
        static constexpr Hash vfx_greydwarf_shaman_pray = -997579616;
        static constexpr Hash vfx_greydwarfnest_destroyed = 1285232666;
        static constexpr Hash vfx_greydwarfnest_hit = 1714298628;
        static constexpr Hash vfx_groundtorch_addFuel = 234310646;
        static constexpr Hash vfx_GuckSackDestroyed = -1900611834;
        static constexpr Hash vfx_GuckSackHit = -285005240;
        static constexpr Hash vfx_GuckSackSmall_Destroyed = -502557894;
        static constexpr Hash vfx_Harpooned = 281556695;
        static constexpr Hash vfx_hatchling_death = -665211050;
        static constexpr Hash vfx_hatchling_hurt = 1791984981;
        static constexpr Hash vfx_HealthUpgrade = 528844679;
        static constexpr Hash vfx_HearthAddFuel = 713592094;
        static constexpr Hash vfx_HitSparks = 1969967266;
        static constexpr Hash vfx_hjall_spit_hit = 232713811;
        static constexpr Hash vfx_ice_destroyed = -477844346;
        static constexpr Hash vfx_ice_destroyed_shelf = 1893942415;
        static constexpr Hash vfx_ice_hit = 905768520;
        static constexpr Hash vfx_iceblocker_destroyed = -1671524600;
        static constexpr Hash vfx_ImpDeath = -1147224145;
        static constexpr Hash vfx_kiln_addore = 463220889;
        static constexpr Hash vfx_kiln_produce = -713949142;
        static constexpr Hash vfx_leech_death = 1653866731;
        static constexpr Hash vfx_leech_hit = -1499057998;
        static constexpr Hash vfx_lever = 393840463;
        static constexpr Hash vfx_lootspawn = -56897708;
        static constexpr Hash vfx_lox_groundslam = 911060721;
        static constexpr Hash vfx_lox_love = 169761379;
        static constexpr Hash vfx_lox_soothed = 247941897;
        static constexpr Hash vfx_MarbleDestroyed = 1799120133;
        static constexpr Hash vfx_MarbleHit = 1251606243;
        static constexpr Hash vfx_MeadSplash = -993136993;
        static constexpr Hash vfx_mill_add = 1864219229;
        static constexpr Hash vfx_mill_produce = -873027154;
        static constexpr Hash vfx_mistlands_mist = 1852800952;
        static constexpr Hash vfx_mountainkit_chair_destroyed = 1273663262;
        static constexpr Hash vfx_mountainkit_table_destroyed = -1844426309;
        static constexpr Hash vfx_MudDestroyed = 1067466478;
        static constexpr Hash vfx_MudHit = 1061837716;
        static constexpr Hash vfx_neck_death = 939881635;
        static constexpr Hash vfx_neck_hit = 461345558;
        static constexpr Hash vfx_oak_cut = -1049327509;
        static constexpr Hash vfx_oaklogdestroyed = 810109753;
        static constexpr Hash vfx_oaklogdestroyed_half = -1918736987;
        static constexpr Hash vfx_ocean_clouds = 267788734;
        static constexpr Hash vfx_odin_despawn = 139151798;
        static constexpr Hash vfx_offering = 2119690483;
        static constexpr Hash vfx_perfectblock = -1068553607;
        static constexpr Hash vfx_pick_wisp = -548233248;
        static constexpr Hash vfx_pickable_pick = 30264026;
        static constexpr Hash vfx_pinelogdestroyed = 1271903464;
        static constexpr Hash vfx_pinelogdestroyed_half = 606433836;
        static constexpr Hash vfx_pinetree_regrow = -1912975828;
        static constexpr Hash vfx_pinetreecut = -1028062451;
        static constexpr Hash vfx_Place_bed = -1132904122;
        static constexpr Hash vfx_Place_beehive = -1425267801;
        static constexpr Hash vfx_Place_brazierceiling01 = -188774892;
        static constexpr Hash vfx_Place_cart = 98119083;
        static constexpr Hash vfx_Place_cauldron = -1835506573;
        static constexpr Hash vfx_Place_charcoalkiln = 793204114;
        static constexpr Hash vfx_Place_chest = 233679104;
        static constexpr Hash vfx_Place_cookingstation_iron = 2095980090;
        static constexpr Hash vfx_Place_darkwood_gate = -414032288;
        static constexpr Hash vfx_Place_digg = 1247363526;
        static constexpr Hash vfx_Place_forge = -882071934;
        static constexpr Hash vfx_Place_Karve = -1990881174;
        static constexpr Hash vfx_Place_mud_road = -986664346;
        static constexpr Hash vfx_Place_oven = -1931600359;
        static constexpr Hash vfx_Place_portal = 1833898339;
        static constexpr Hash vfx_Place_Raft = 98119528;
        static constexpr Hash vfx_Place_raise = -1587639081;
        static constexpr Hash vfx_Place_refinery = -13607197;
        static constexpr Hash vfx_Place_replant = -148711009;
        static constexpr Hash vfx_Place_smelter = -844306953;
        static constexpr Hash vfx_Place_spinningwheel = 2083408048;
        static constexpr Hash vfx_Place_stone_floor = 2082355887;
        static constexpr Hash vfx_Place_stone_floor_2x2 = -17539746;
        static constexpr Hash vfx_Place_stone_wall_2x1 = 460429117;
        static constexpr Hash vfx_Place_stone_wall_4x2 = 1179538052;
        static constexpr Hash vfx_Place_throne02 = -869703693;
        static constexpr Hash vfx_Place_turret = -1979681737;
        static constexpr Hash vfx_Place_VikingShip = 485939669;
        static constexpr Hash vfx_Place_windmill = 470607579;
        static constexpr Hash vfx_Place_wood_beam = 1407536740;
        static constexpr Hash vfx_Place_wood_floor = -752405273;
        static constexpr Hash vfx_Place_wood_pole = 298697721;
        static constexpr Hash vfx_Place_wood_roof = 177738195;
        static constexpr Hash vfx_Place_wood_stair = -2070887160;
        static constexpr Hash vfx_Place_wood_wall = 1804172929;
        static constexpr Hash vfx_Place_wood_wall_half = -1695999899;
        static constexpr Hash vfx_Place_wood_wall_roof = -849025734;
        static constexpr Hash vfx_Place_workbench = 1112160676;
        static constexpr Hash vfx_player_death = 1600378975;
        static constexpr Hash vfx_player_hit = 2114241932;
        static constexpr Hash vfx_Poison = 1225290243;
        static constexpr Hash vfx_poisonarrow_hit = -1642631106;
        static constexpr Hash vfx_Potion_eitr_minor = -1778433855;
        static constexpr Hash vfx_Potion_health_medium = -1239285227;
        static constexpr Hash vfx_Potion_stamina_medium = 1971950416;
        static constexpr Hash vfx_prespawn = 288873395;
        static constexpr Hash vfx_ProjectileHit = 1252888223;
        static constexpr Hash vfx_RockDestroyed = 395694737;
        static constexpr Hash vfx_RockDestroyed_large = -631382181;
        static constexpr Hash vfx_RockDestroyed_marble = 145745747;
        static constexpr Hash vfx_RockDestroyed_Obsidian = -871962805;
        static constexpr Hash vfx_RockHit = -730764453;
        static constexpr Hash vfx_RockHit_Marble = 24110301;
        static constexpr Hash vfx_RockHit_Obsidian = 661975945;
        static constexpr Hash vfx_SawDust = -1195108480;
        static constexpr Hash vfx_SawDust_abomination = 1575895354;
        static constexpr Hash vfx_seagull_death = 1241359129;
        static constexpr Hash vfx_seekerbrute_groundslam = 1951621125;
        static constexpr Hash vfx_serpent_attack_trigger = 1487320808;
        static constexpr Hash vfx_serpent_death = -1248245181;
        static constexpr Hash vfx_serpent_hurt = 1238827358;
        static constexpr Hash vfx_shrub_2_destroyed = -408284028;
        static constexpr Hash vfx_shrub_2_heath_destroyed = -1172819289;
        static constexpr Hash vfx_shrub_2_hit = -1619361458;
        static constexpr Hash vfx_shrub_heath_hit = -355458734;
        static constexpr Hash vfx_silvermace_hit = -1516628266;
        static constexpr Hash vfx_skeleton_big_death = 1464964316;
        static constexpr Hash vfx_skeleton_death = 1966038773;
        static constexpr Hash vfx_skeleton_hit = 1114691826;
        static constexpr Hash vfx_skeleton_mace_hit = -916784691;
        static constexpr Hash vfx_sledge_hit = -849145139;
        static constexpr Hash vfx_sledge_iron_hit = 1136200682;
        static constexpr Hash vfx_Slimed = -728978671;
        static constexpr Hash vfx_smelter_addfuel = 613386663;
        static constexpr Hash vfx_smelter_addore = -236669799;
        static constexpr Hash vfx_smelter_produce = 524856430;
        static constexpr Hash vfx_Smoked = -1730929208;
        static constexpr Hash vfx_spawn = 1617006654;
        static constexpr Hash vfx_spawn_large = -1665118250;
        static constexpr Hash vfx_spawn_small = 1628388642;
        static constexpr Hash vfx_StaffShield = 1967588846;
        static constexpr Hash vfx_StaminaUpgrade = -1295196504;
        static constexpr Hash vfx_standing_brazier_destroyed = 362897581;
        static constexpr Hash vfx_stone_floor_2x2_destroyed = -207598898;
        static constexpr Hash vfx_stone_floor_destroyed = 744571047;
        static constexpr Hash vfx_stone_stair_destroyed = 884180074;
        static constexpr Hash vfx_stone_wall_4x2_destroyed = 487704284;
        static constexpr Hash vfx_stonegolem_attack_hit = -497397627;
        static constexpr Hash vfx_stonegolem_death = 649138327;
        static constexpr Hash vfx_stonegolem_hurt = -1321932824;
        static constexpr Hash vfx_stonegolem_wakeup = -619835584;
        static constexpr Hash vfx_stubbe = -1129264550;
        static constexpr Hash vfx_swamp_mist = 1008438155;
        static constexpr Hash vfx_swamptree_cut = -907217478;
        static constexpr Hash vfx_Tared = 1314778849;
        static constexpr Hash vfx_torch_hit = 998359095;
        static constexpr Hash vfx_tree_fall_hit = -1850635673;
        static constexpr Hash vfx_troll_attack_hit = -398437737;
        static constexpr Hash vfx_troll_death = 1715200685;
        static constexpr Hash vfx_troll_groundslam = 1279620163;
        static constexpr Hash vfx_troll_log_hitground = -795702158;
        static constexpr Hash vfx_troll_rock_destroyed = -1036826514;
        static constexpr Hash vfx_turnip_grow = 229884025;
        static constexpr Hash vfx_ulv_death = 1248443675;
        static constexpr Hash vfx_UndeadBurn = -146689421;
        static constexpr Hash vfx_vines_destroyed = 1253313884;
        static constexpr Hash vfx_wagon_destroyed = 164468187;
        static constexpr Hash vfx_walltorch_addFuel = 109708603;
        static constexpr Hash vfx_WaterImpact_Karve = 1612136656;
        static constexpr Hash vfx_WaterImpact_Raft = -837489954;
        static constexpr Hash vfx_WaterImpact_VikingShip = 2135074485;
        static constexpr Hash vfx_Wet = 953948517;
        static constexpr Hash vfx_WishbonePing = 1002622938;
        static constexpr Hash vfx_wolf_death = 416803702;
        static constexpr Hash vfx_wolf_hit = -1748425933;
        static constexpr Hash vfx_wood_core_stack_destroyed = -1319659911;
        static constexpr Hash vfx_wood_fine_stack_destroyed = 511674592;
        static constexpr Hash vfx_wood_stack_destroyed = -1403761043;
        static constexpr Hash vfx_wood_yggdrasil_stack_destroyed = 123068292;
        static constexpr Hash vfx_wraith_death = -1525697551;
        static constexpr Hash vfx_wraith_hit = -1173641826;
        static constexpr Hash vfx_yggashoot_cut = 235644861;
        static constexpr Hash vfx_yggashoot_small1_destroy = -1134401048;
        static constexpr Hash VikingShip = 118230510;
        static constexpr Hash vines = -649881157;
        static constexpr Hash WaterLiquid = 1614819093;
        static constexpr Hash widestone = -2094909618;
        static constexpr Hash widestone_frac = -1462463015;
        static constexpr Hash windmill = -984198658;
        static constexpr Hash Wishbone = 1219941843;
        static constexpr Hash Wisp = -1146623251;
        static constexpr Hash WitheredBone = 1905991362;
        static constexpr Hash Wolf = 1010961914;
        static constexpr Hash Wolf_cub = -1969683915;
        static constexpr Hash Wolf_Ragdoll = -113693526;
        static constexpr Hash WolfClaw = 745645583;
        static constexpr Hash WolfFang = -927843224;
        static constexpr Hash WolfHairBundle = -237951114;
        static constexpr Hash WolfJerky = -1029078359;
        static constexpr Hash WolfMeat = 167698947;
        static constexpr Hash WolfMeatSkewer = -1653997440;
        static constexpr Hash WolfPelt = 167698835;
        static constexpr Hash Wood = -151837501;
        static constexpr Hash wood_beam = -1109248277;
        static constexpr Hash wood_beam_1 = -155126795;
        static constexpr Hash wood_beam_26 = 1332601932;
        static constexpr Hash wood_beam_45 = 1735886465;
        static constexpr Hash wood_core_stack = -1421478172;
        static constexpr Hash wood_door = 692106840;
        static constexpr Hash wood_dragon1 = 775398114;
        static constexpr Hash wood_fence = -2081117101;
        static constexpr Hash wood_fine_stack = 719595997;
        static constexpr Hash wood_floor = -1045925142;
        static constexpr Hash wood_floor_1x1 = -745132593;
        static constexpr Hash wood_gate = 1007930775;
        static constexpr Hash wood_ledge = 364666255;
        static constexpr Hash wood_log_26 = 581611607;
        static constexpr Hash wood_log_45 = -224957444;
        static constexpr Hash wood_pole = 1142703842;
        static constexpr Hash wood_pole_log = -1424233331;
        static constexpr Hash wood_pole_log_4 = 1024658248;
        static constexpr Hash wood_pole2 = 524618856;
        static constexpr Hash wood_roof = -1626585462;
        static constexpr Hash wood_roof_45 = -877641284;
        static constexpr Hash wood_roof_icorner = -1076788695;
        static constexpr Hash wood_roof_icorner_45 = 1132532351;
        static constexpr Hash wood_roof_ocorner = -1076581777;
        static constexpr Hash wood_roof_ocorner_45 = 1139360637;
        static constexpr Hash wood_roof_top = 2141320638;
        static constexpr Hash wood_roof_top_45 = -598122136;
        static constexpr Hash wood_stack = -2139848760;
        static constexpr Hash wood_stair = 945264965;
        static constexpr Hash wood_stepladder = -354552934;
        static constexpr Hash wood_wall_half = -1000346356;
        static constexpr Hash wood_wall_log = -886156835;
        static constexpr Hash wood_wall_log_4x0_5 = 214591025;
        static constexpr Hash wood_wall_quarter = 622849921;
        static constexpr Hash wood_wall_roof = 1882996769;
        static constexpr Hash wood_wall_roof_45 = -1218250357;
        static constexpr Hash wood_wall_roof_45_upsidedown = 618430194;
        static constexpr Hash wood_wall_roof_top = 437925541;
        static constexpr Hash wood_wall_roof_top_45 = -1199392921;
        static constexpr Hash wood_wall_roof_upsidedown = 1974160938;
        static constexpr Hash wood_window = -1138351504;
        static constexpr Hash wood_yggdrasil_stack = 1779884373;
        static constexpr Hash woodiron_beam = -1499117535;
        static constexpr Hash woodiron_beam_26 = -1403331882;
        static constexpr Hash woodiron_beam_45 = -1000047361;
        static constexpr Hash woodiron_pole = -2023619468;
        static constexpr Hash woodwall = -523056075;
        static constexpr Hash Wraith = 68955605;
        static constexpr Hash wraith_melee = -1764301166;
        static constexpr Hash YagluthDrop = -609016745;
        static constexpr Hash yggashoot_log = -1521034916;
        static constexpr Hash yggashoot_log_half = 354363928;
        static constexpr Hash YggaShoot_small1 = -62980316;
        static constexpr Hash YggaShoot1 = 2026655068;
        static constexpr Hash YggaShoot2 = -702228287;
        static constexpr Hash YggaShoot3 = 863855654;
        static constexpr Hash YggdrasilPorridge = -1850988268;
        static constexpr Hash YggdrasilRoot = 2069500716;
        static constexpr Hash YggdrasilWood = 1007457867;
        static constexpr Hash YmirRemains = 176995072;
        static constexpr Hash vfx_FireWorkTest = -360053482;
    };
}