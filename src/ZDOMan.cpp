#include "ZDOMan.hpp"

int64_t ZDOMan::compareReceiver;

ZDOMan::ZDOMan()
{
	//ZDOMan.m_instance = this;
	m_myid = 1234567891011; // Utils::GenerateUID();
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

