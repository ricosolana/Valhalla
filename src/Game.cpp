#include "Game.h"

#include "NetRouteManager.h"
#include "NetHashes.h"
#include "ZoneSystem.h"

namespace Game {

	void RPC_SleepStop(OWNER_t sender) {
		//if (this.m_saveTimer > 60f)
		//{
		//	this.SavePlayerProfile(false);
		//	if (ZNet.instance)
		//	{
		//		ZNet.instance.Save(false);
		//		return;
		//	}
		//}
		//else
		//{
		//	ZLog.Log("Saved recently, skipping sleep save.");
		//}
	}





	void Init() {
		
		NetRouteManager::Register(NetHashes::Routed::SleepStop, RPC_SleepStop);
		
	}

}
