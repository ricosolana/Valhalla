#include "NetObject.h"
#include "VServer.h"

NetObject::NetObject() {
	/*
	if (ZNetView.m_forceDisableInit || ZDOMan.instance == null)
	{
		UnityEngine.Object.Destroy(this);
		return;
	}
	this.m_body = base.GetComponent<Rigidbody>();
	if (ZNetView.m_useInitZDO && ZNetView.m_initZDO == null)
	{
		ZLog.LogWarning("Double ZNetview when initializing object " + base.gameObject.name);
	}
	if (ZNetView.m_initZDO != null)
	{
		this.m_zdo = ZNetView.m_initZDO;
		ZNetView.m_initZDO = null;
		if (this.m_zdo.m_type != this.m_type && this.m_zdo.IsOwner())
		{
			this.m_zdo.SetType(this.m_type);
		}
		if (this.m_zdo.m_distant != this.m_distant && this.m_zdo.IsOwner())
		{
			this.m_zdo.SetDistant(this.m_distant);
		}
		if (this.m_syncInitialScale)
		{
			Vector3 vec = this.m_zdo.GetVec3("scale", base.transform.localScale);
			base.transform.localScale = vec;
		}
		if (this.m_body)
		{
			this.m_body.Sleep();
		}
	}
	else
	{
		string prefabName = this.GetPrefabName();
		this.m_zdo = ZDOMan.instance.CreateNewZDO(base.transform.position);
		this.m_zdo.m_persistent = this.m_persistent;
		this.m_zdo.m_type = this.m_type;
		this.m_zdo.m_distant = this.m_distant;
		this.m_zdo.SetPrefab(prefabName.GetStableHashCode());
		this.m_zdo.SetRotation(base.transform.rotation);
		if (this.m_syncInitialScale)
		{
			this.m_zdo.Set("scale", base.transform.localScale);
		}
		if (ZNetView.m_ghostInit)
		{
			this.m_ghost = true;
			return;
		}
	}
	ZNetScene.instance.AddInstance(this.m_zdo, this);*/
}

void NetObject::Register(HASH_t hash, IMethod<OWNER_t> *method) {
    assert(!m_functions.contains(hash));
    m_functions[hash] = std::unique_ptr<IMethod<OWNER_t>>(method);
}

void NetObject::SetLocalScale(Vector3 scale) {
	//base.transform.localScale = scale;
	if (m_sync && m_syncInitialScale && m_sync->Local()) {
		m_sync->Set("scale", scale); // base.transform.localScale);
	}
}
	
//void NetObject::SetPersistent(bool persistent) {
//	this.m_zdo.m_persistent = persistent;
//}

std::string NetObject::GetPrefabName() const {
	//return VUtils.GetPrefabName(base.gameObject);
	return "";
}

void NetObject::Destroy() {
	//ZNetScene.instance.Destroy(base.gameObject);
}

void NetObject::ResetNetSync() {
	m_sync = nullptr;
}

void NetObject::HandleRoutedRPC(NetRouteManager::Data rpcData) {
	auto&& find = m_functions.find(rpcData.m_methodHash);
	if (find != m_functions.end()) {
		find->second->Invoke(rpcData.m_senderPeerID, rpcData.m_parameters);
	}
	else {
		LOG(INFO) << "Failed to find rpc method " << rpcData.m_methodHash;
	}
}