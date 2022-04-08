#pragma once

#include <robin_hood.h>
#include <vector>
#include "ZDOID.hpp"
#include "ZDO.hpp"
#include "Vector2.hpp"
#include "HashUtils.hpp"

class ZDOMan {
	// why static
	static int64_t compareReceiver;

	robin_hood::unordered_map<ZDOID, ZDO, HashUtils::Hasher> m_objectsByID;
	std::unique_ptr<std::vector<ZDO>> m_objectsBySector; // array of vector
	//robin_hood::unordered_map<Vector2i, List<ZDO>> m_objectsByOutsideSector;
	//List<ZDOMan.ZDOPeer> m_peers = new List<ZDOMan.ZDOPeer>();
	const int m_maxDeadZDOs = 100000;
	robin_hood::unordered_map<ZDOID, long, HashUtils::Hasher> m_deadZDOs;
	std::vector<ZDOID> m_destroySendList;
	robin_hood::unordered_set<ZDOID, HashUtils::Hasher> m_clientChangeQueue;
	int64_t m_myid;
	uint32_t m_nextUid = 1U;
	int32_t m_width;
	int32_t m_halfWidth;
	float m_sendTimer;
	const float m_sendFPS = 20.f;
	float m_releaseZDOTimer;
	static ZDOMan m_instance;
	int32_t m_zdosSent;
	int32_t m_zdosRecv;
	int32_t m_zdosSentLastSec;
	int32_t m_zdosRecvLastSec;
	float m_statTimer;
	std::vector<ZDO> m_tempToSync;
	std::vector<ZDO> m_tempToSyncDistant;
	std::vector<ZDO> m_tempNearObjects;
	std::vector<ZDOID> m_tempRemoveList;
	//ZDOMan.SaveData m_saveData;

public:
	//Action<ZDO> m_onZDODestroyed;

	ZDOMan();
};

struct SaveData
{
	int64_t m_myid;

	uint32_t m_nextUid = 1U;

	std::vector<ZDO*> m_zdos;

	robin_hood::unordered_map<ZDOID, int64_t, HashUtils::Hasher> m_deadZDOs;
};