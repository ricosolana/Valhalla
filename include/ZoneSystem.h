#pragma once

#include "VUtils.h"
#include "NetPeer.h"

namespace ZoneSystem {
    static constexpr float WATER_LEVEL = 30;

	void OnNewPeer(NetPeer *peer);

	Vector2i GetZoneCoords(const Vector3 &point);

    Vector3 GetZonePos(const Vector2i& id);

	void Init();

	static constexpr int ACTIVE_AREA = 1;
	static constexpr int ACTIVE_DISTANT_AREA = 1;
	static constexpr int PGW_VERSION = 53;

    static const char* LOCATION_SPAWN = "StartTemple";
    static const char* LOCATION_EIKTHYR = "Eikthyrnir";
    static const char* LOCATION_MODER = "Dragonqueen";
    static const char* LOCATION_YAGLUTH = "GoblinKing";
    static const char* LOCATION_ELDER = "GDKing";
    static const char* LOCATION_BONEMASS = "Bonemass";
    static const char* LOCATION_HALDOR = "Vendor_BlackForest";
    //static const char* LOCATION_CRYPT4 = "SunkenCrypt4";
    // there are approxomately 50+ locations to extract

    static const char* KEY_BOSS_BONEMASS = "defeated_bonemass";
    static const char* KEY_BOSS_ELDER = "defeated_gdking";
    static const char* KEY_BOSS_YAGLUTH = "defeated_goblinking";
    static const char* KEY_BOSS_MODER = "defeated_dragon";
    static const char* KEY_BOSS_EIKTHYR = "defeated_eikthyr";
    static const char* KEY_MOB_TROLL = "KilledTroll";
    static const char* KEY_MOB_SURTLING = "killed_surtling";
    static const char* KEY_MOD_BAT = "KilledBat";
    static const char* KEY_DISABLE_MAP = "nomap";
    static const char* KEY_DISABLE_PORTALS = "noportals";
}














