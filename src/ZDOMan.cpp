#include "ZDOMan.hpp"
#include "Game.hpp"

int64_t ZDOMan::compareReceiver;

ZDOMan::ZDOMan(ZNetPeer* peer)
{
	//ZDOMan.m_instance = this;
	m_peer = peer;
	m_myid = 1234567891011; // Utils::GenerateUID();
	Game::Get()->m_znet->m_routedRpc->Register("DestroyZDO", new ZMethod(this, &ZDOMan::RPC_DestroyZDO));
	Game::Get()->m_znet->m_routedRpc->Register("RequestZDO", new ZMethod(this, &ZDOMan::RPC_RequestZDO));
	//ZRoutedRpc.instance.Register<ZPackage>("DestroyZDO", new Action<long, ZPackage>(this.RPC_DestroyZDO));
	//ZRoutedRpc.instance.Register<ZDOID>("RequestZDO", new Action<long, ZDOID>(this.RPC_RequestZDO));
	m_width = 512;
	m_halfWidth = m_width / 2;
	//ResetSectorArray();
}

int64_t ZDOMan::GetMyID()
{
	return m_myid;
}

void ZDOMan::RPC_DestroyZDO(UID_t sender, ZPackage pkg) {
	int num = pkg.Read<int32_t>();
	for (int i = 0; i < num; i++)
	{
		ZDOID uid = pkg.Read<ZDOID>();
		//HandleDestroyedZDO(uid);
	}
}

void ZDOMan::RPC_RequestZDO(UID_t sender, ZDOID id)
{
	//m_peer->ForceSendZDO(id);	
}
