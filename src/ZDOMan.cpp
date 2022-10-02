#include "ZDOMan.h"
#include "ValhallaServer.h"

int64_t ZDOMan::compareReceiver;

ZDOMan::ZDOMan()
{
	//ZDOMan.m_instance = this;
	//Valhalla()->m_znet->m_routedRpc->Register("DestroyZDO", new ZMethod(this, &ZDOMan::RPC_DestroyZDO));
	//Valhalla()->m_znet->m_routedRpc->Register("RequestZDO", new ZMethod(this, &ZDOMan::RPC_RequestZDO));
	//ZRoutedRpc.instance.Register<ZPackage>("DestroyZDO", new Action<long, ZPackage>(this.RPC_DestroyZDO));
	//ZRoutedRpc.instance.Register<ZDOID>("RequestZDO", new Action<long, ZDOID>(this.RPC_RequestZDO));
	m_width = 512;
	m_halfWidth = m_width / 2;
	//ResetSectorArray();
}

void ZDOMan::AddPeer(ZNetPeer::Ptr peer) {
	m_peers.push_back(std::make_unique<ZDOPeer>(peer));
	
	//REGISTER_RPC(peer->m_rpc, "ZDOData", ZDOMan::RPC_ZDOData);
}

int64_t ZDOMan::GetMyID()
{
	return m_myid;
}

void ZDOMan::RPC_ZDOData(ZRpc* rpc, ZPackage::Ptr pkg) {
	throw std::runtime_error("Not implemented");
	/*
	auto zdopeer = GetZDOPeer(rpc);

	float time = 0; //Time.time;
	//int num = 0; // zdos to be received
	auto invalid_sector_count = pkg->Read<int32_t>(); // invalid sector count
	for (int i = 0; i < invalid_sector_count; i++)
	{
		auto id = pkg->Read<ZDOID>();
		//ZDO zdo = this.GetZDO(id);
		//if (zdo != null)
		//{
		//	zdo.InvalidateSector();
		//}
	}

	int zdosRecv = 0;
	for (;;)
	{
		auto zdoid = pkg->Read<ZDOID>(); // uid
		if (zdoid == ZDOID::NONE)
			break;

		zdosRecv++;

		auto num3 = pkg->Read<uint32_t>(); // owner revision
		auto num4 = pkg->Read<uint32_t>(); // data revision
		auto owner = pkg->Read<int64_t>(); // owner
		auto vector = pkg->Read<Vector3>(); // position
		auto pkg2 = pkg->Read<ZPackage::Ptr>(); // 
		auto zdo2 = GetZDO(zdoid);
		bool flag = false;
		if (zdo2)
		{
			if (num4 <= zdo2.m_dataRevision)
			{
				if (num3 > zdo2.m_ownerRevision)
				{
					zdo2.m_owner = owner;
					zdo2.m_ownerRevision = num3;
					zdopeer.m_zdos[zdoid] = new ZDOMan.ZDOPeer.PeerZDOInfo(num4, num3, time);
					continue;
				}
				continue;
			}
		}
		else
		{
			zdo2 = this.CreateNewZDO(zdoid, vector);
			flag = true;
		}
		
		zdo2.m_ownerRevision = num3;
		zdo2.m_dataRevision = num4;
		zdo2.m_owner = owner;
		zdo2.InternalSetPosition(vector);
		zdopeer.m_zdos[zdoid] = new ZDOMan.ZDOPeer.PeerZDOInfo(zdo2.m_dataRevision, zdo2.m_ownerRevision, time);
		zdo2.Deserialize(pkg2);
		if (ZNet.instance.IsServer() && flag && this.m_deadZDOs.ContainsKey(zdoid))
		{
			zdo2.SetOwner(this.m_myid);
			this.DestroyZDO(zdo2);
		}
	}

	m_zdosRecv += zdosRecv;
	*/
}

void ZDOMan::RPC_DestroyZDO(uuid_t sender, ZPackage pkg) {
	int num = pkg.Read<int32_t>();
	for (int i = 0; i < num; i++)
	{
		ZDOID uid = pkg.Read<ZDOID>();
		//HandleDestroyedZDO(uid);
	}
}

void ZDOMan::RPC_RequestZDO(uuid_t sender, ZDOID id)
{
	//m_peer->ForceSendZDO(id);	
}
