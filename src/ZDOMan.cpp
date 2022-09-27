#include "ZDOMan.h"
#include "ValhallaServer.h"

int64_t ZDOMan::compareReceiver;

ZDOMan::ZDOMan(UUID uid)
{
	//ZDOMan.m_instance = this;
	m_myid = uid; // Utils::GenerateUID();
	Valhalla()->m_znet->m_routedRpc->Register("DestroyZDO", new ZMethod(this, &ZDOMan::RPC_DestroyZDO));
	Valhalla()->m_znet->m_routedRpc->Register("RequestZDO", new ZMethod(this, &ZDOMan::RPC_RequestZDO));
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

void ZDOMan::RPC_DestroyZDO(UUID sender, ZPackage pkg) {
	int num = pkg.Read<int32_t>();
	for (int i = 0; i < num; i++)
	{
		ZDOID uid = pkg.Read<ZDOID>();
		//HandleDestroyedZDO(uid);
	}
}

void ZDOMan::RPC_RequestZDO(UUID sender, ZDOID id)
{
	//m_peer->ForceSendZDO(id);	
}
