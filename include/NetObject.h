#pragma once

/*
 * ZNetView equivalent
 * NetObject is a wrapper around a ZDO and certain exceptional ZRoutedRPC methods hosted here only
 */

// change name to something more obvious
// netobject
// netview is just a wrapper to represent an object with synchronized fields and method calls
class NetObject {

};

private void Awake()
{
	if (ZNetView.m_forceDisableInit || NetSyncMan.instance == null)
	{
		UnityEngine.Object.Destroy(this);
		return;
	}
	this.m_body = base.GetComponent<Rigidbody>();
	if (ZNetView.m_useInitNetSync && ZNetView.m_initNetSync == null)
	{
		ZLog.LogWarning("Double ZNetview when initializing object " + base.gameObject.name);
	}
	if (ZNetView.m_initNetSync != null)
	{
		this.m_NetSync = ZNetView.m_initNetSync;
		ZNetView.m_initNetSync = null;
		if (this.m_NetSync.m_type != this.m_type && this.m_NetSync.IsOwner())
		{
			this.m_NetSync.SetType(this.m_type);
		}
		if (this.m_NetSync.m_distant != this.m_distant && this.m_NetSync.IsOwner())
		{
			this.m_NetSync.SetDistant(this.m_distant);
		}
		if (this.m_syncInitialScale)
		{
			Vector3 vec = this.m_NetSync.GetVec3("scale", base.transform.localScale);
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
		this.m_NetSync = NetSyncMan.instance.CreateNewNetSync(base.transform.position);
		this.m_NetSync.m_persistent = this.m_persistent;
		this.m_NetSync.m_type = this.m_type;
		this.m_NetSync.m_distant = this.m_distant;
		this.m_NetSync.SetPrefab(prefabName.GetStableHashCode());
		this.m_NetSync.SetRotation(base.transform.rotation);
		if (this.m_syncInitialScale)
		{
			this.m_NetSync.Set("scale", base.transform.localScale);
		}
		if (ZNetView.m_ghostInit)
		{
			this.m_ghost = true;
			return;
		}
	}
	ZNetScene.instance.AddInstance(this.m_NetSync, this);
}

// Token: 0x06000C69 RID: 3177 RVA: 0x0005DC60 File Offset: 0x0005BE60
public void SetLocalScale(Vector3 scale)
{
	base.transform.localScale = scale;
	if (this.m_NetSync != null && this.m_syncInitialScale && this.IsOwner())
	{
		this.m_NetSync.Set("scale", base.transform.localScale);
	}
}

// Token: 0x06000C6A RID: 3178 RVA: 0x0000A156 File Offset: 0x00008356
private void OnDestroy()
{
	ZNetScene.instance;
}

// Token: 0x06000C6B RID: 3179 RVA: 0x0000A163 File Offset: 0x00008363
public void SetPersistent(bool persistent)
{
	this.m_NetSync.m_persistent = persistent;
}

// Token: 0x06000C6C RID: 3180 RVA: 0x0000A171 File Offset: 0x00008371
public string GetPrefabName()
{
	return Utils.GetPrefabName(base.gameObject);
}

// Token: 0x06000C6D RID: 3181 RVA: 0x0000A17E File Offset: 0x0000837E
public void Destroy()
{
	ZNetScene.instance.Destroy(base.gameObject);
}

// Token: 0x06000C6E RID: 3182 RVA: 0x0000A190 File Offset: 0x00008390

// this name is misleading
// what is owner? entity? player?
// owner here refers to client or server
// local vs remote
// more extensive testing is required
public bool IsOwner()
{
	return this.m_NetSync.IsOwner();
}

// Token: 0x06000C6F RID: 3183 RVA: 0x0000A19D File Offset: 0x0000839D
public bool HasOwner()
{
	return this.m_NetSync.HasOwner();
}

// Token: 0x06000C70 RID: 3184 RVA: 0x0000A1AA File Offset: 0x000083AA
public void ClaimOwnership()
{
	if (this.IsOwner())
	{
		return;
	}
	this.m_NetSync.SetOwner(NetSyncMan.instance.GetMyID());
}

// Token: 0x06000C71 RID: 3185 RVA: 0x0000A1CA File Offset: 0x000083CA
public NetSync GetNetSync()
{
	return this.m_NetSync;
}

// Token: 0x06000C72 RID: 3186 RVA: 0x0000A1D2 File Offset: 0x000083D2
public bool IsValid()
{
	return this.m_NetSync != null && this.m_NetSync.IsValid();
}

// Token: 0x06000C73 RID: 3187 RVA: 0x0000A1E9 File Offset: 0x000083E9
public void ResetNetSync()
{
	this.m_NetSync = null;
}

// Token: 0x06000C74 RID: 3188 RVA: 0x0000A1F2 File Offset: 0x000083F2
public void Register(string name, Action<long> f)
{
	this.m_functions.Add(name.GetStableHashCode(), new RoutedMethod(f));
}

// Token: 0x06000C75 RID: 3189 RVA: 0x0000A20B File Offset: 0x0000840B
public void Register<T>(string name, Action<long, T> f)
{
	this.m_functions.Add(name.GetStableHashCode(), new RoutedMethod<T>(f));
}

// Token: 0x06000C76 RID: 3190 RVA: 0x0000A224 File Offset: 0x00008424
public void Register<T, U>(string name, Action<long, T, U> f)
{
	this.m_functions.Add(name.GetStableHashCode(), new RoutedMethod<T, U>(f));
}

// Token: 0x06000C77 RID: 3191 RVA: 0x0000A23D File Offset: 0x0000843D
public void Register<T, U, V>(string name, Action<long, T, U, V> f)
{
	this.m_functions.Add(name.GetStableHashCode(), new RoutedMethod<T, U, V>(f));
}

// Token: 0x06000C78 RID: 3192 RVA: 0x0000A256 File Offset: 0x00008456
public void Register<T, U, V, B>(string name, RoutedMethod<T, U, V, B>.Method f)
{
	this.m_functions.Add(name.GetStableHashCode(), new RoutedMethod<T, U, V, B>(f));
}

// Token: 0x06000C79 RID: 3193 RVA: 0x0005DCAC File Offset: 0x0005BEAC
public void Unregister(string name)
{
	int stableHashCode = name.GetStableHashCode();
	this.m_functions.Remove(stableHashCode);
}

// Token: 0x06000C7A RID: 3194 RVA: 0x0005DCD0 File Offset: 0x0005BED0
public void HandleRoutedRPC(ZRoutedRpc.RoutedRPCData rpcData)
{
	RoutedMethodBase routedMethodBase;
	if (this.m_functions.TryGetValue(rpcData.m_methodHash, out routedMethodBase))
	{
		routedMethodBase.Invoke(rpcData.m_senderPeerID, rpcData.m_parameters);
		return;
	}
	ZLog.LogWarning("Failed to find rpc method " + rpcData.m_methodHash.ToString());
}

// Token: 0x06000C7B RID: 3195 RVA: 0x0000A26F File Offset: 0x0000846F
public void InvokeRPC(long targetID, string method, params object[] parameters)
{
	ZRoutedRpc.instance.InvokeRoutedRPC(targetID, this.m_NetSync.m_uid, method, parameters);
}

// Token: 0x06000C7C RID: 3196 RVA: 0x0000A289 File Offset: 0x00008489
public void InvokeRPC(string method, params object[] parameters)
{
	ZRoutedRpc.instance.InvokeRoutedRPC(this.m_NetSync.m_owner, this.m_NetSync.m_uid, method, parameters);
}

// Token: 0x06000C7D RID: 3197 RVA: 0x0005DD20 File Offset: 0x0005BF20
public static object[] Deserialize(long callerID, ParameterInfo[] paramInfo, ZPackage pkg)
{
	List<object> list = new List<object>();
	list.Add(callerID);
	ZRpc.Deserialize(paramInfo, pkg, ref list);
	return list.ToArray();
}

// Token: 0x06000C7E RID: 3198 RVA: 0x0000A2AD File Offset: 0x000084AD
public static void StartGhostInit()
{
	ZNetView.m_ghostInit = true;
}

// Token: 0x06000C7F RID: 3199 RVA: 0x0000A2B5 File Offset: 0x000084B5
public static void FinishGhostInit()
{
	ZNetView.m_ghostInit = false;
}

// Token: 0x04000D4B RID: 3403
public bool m_persistent;

// Token: 0x04000D4C RID: 3404
public bool m_distant;

// Token: 0x04000D4D RID: 3405
public NetSync.ObjectType m_type;

// Token: 0x04000D4E RID: 3406
public bool m_syncInitialScale;

// Token: 0x04000D4F RID: 3407
private NetSync m_NetSync;

// Token: 0x04000D50 RID: 3408
private Rigidbody m_body;

// Token: 0x04000D51 RID: 3409
private Dictionary<int, RoutedMethodBase> m_functions = new Dictionary<int, RoutedMethodBase>();

// Token: 0x04000D52 RID: 3410
private bool m_ghost;

// Token: 0x04000D53 RID: 3411
public static bool m_useInitNetSync;

// Token: 0x04000D54 RID: 3412
public static NetSync m_initNetSync;

// Token: 0x04000D55 RID: 3413
public static bool m_forceDisableInit;

// Token: 0x04000D56 RID: 3414
private static bool m_ghostInit;