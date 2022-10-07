# Valhalla
Open source Valheim server and client written in C++.

## Disclaimer
I am in no way affiliated, endorsed, or associated with Valheim nor its creators.

The purpose of this project is just to explore and be creative in a non-commercial manner. Any usages of code below are in fair use and in full compliance with Coffee Stain Publishing EULA https://irongatestudio.se/CoffeeStainPublishingAB-GameContentUsagePolicy-18-05-2021.pdf

Do this at your own risk. I am not liable for anything you decide to do with this.

## Overview
I have separated the different implementations of Valhalla into branches. The main branch is mainly informative based and my personal findings. The client is located on the `client` branch. The server is located in the `server` branch. The client is no longer being developed due to my technical limitations with graphical programming. The `steamapi-example` branch is a minimal example of steam api. I made a feeble attempt in using the steam api to host the server (like Valheim does), but I kept running into issues with the server requiring the Steam client to be already running.

Valheim uses the Steamworks API for authentication, networking, and lobbies. Valve's GameNetworkingSockets will not work because Steam sockets require the Steam backend. This is frustrating because I want legitimate clients to be authorized normally and join this server. I hope to use Steam authorization in the future, its just too complicated right now. So, how can clients connect?

A temporary fix is introduced by modifying a few lines in both the Valheim server and client. For some reason, the devs left some of their older networking sockets that make use of C# TCP sockets. We will be using those.

## Patching
We will be modifying the Valheim C# assembly using Dnspy. This will allow for us to easily connect, but there will be no encryption or user authentication.

Make a backup of the `assembly_valheim.dll` before continuing. 

Open the `assembly_valheim.dll` and navigate to `ZNet.cs`.

### Client-side

<details><summary>211.7</summary>

Change the FejdStartup JoinServer() method similar to:
```c#
// FejdStartup
ZPlayFabMatchmaking.FindHostByIp(serverJoin.GetIPPortString(), delegate(PlayFabMatchmakingServerData result)
{
	ZNet.SetServerHost(result.remotePlayerId);
	ZLog.Log("Determined backend of dedicated server to be PlayFab");
}, delegate
{
	ZNet.SetServerHost(serverJoin.GetIPString(), (int)serverJoin.m_port, OnlineBackendType.Steamworks);
	ZLog.Log("Determined backend of dedicated server to be Steamworks");
}, true);
```
to this
```c#
// FejdStartup
ZPlayFabMatchmaking.FindHostByIp(serverJoin.GetIPPortString(), delegate(PlayFabMatchmakingServerData result)
{
	ZNet.SetServerHost(result.remotePlayerId);
	ZLog.Log("Determined backend of dedicated server to be PlayFab");
}, delegate
{
	ZNet.SetServerHost(serverJoin.GetIPString(), (int)serverJoin.m_port, OnlineBackendType.CustomSocket);
	ZLog.Log("Determined backend of dedicated server to be a CustomSocket (insecure)");
}, true);
```

</details>

<details><summary>210.6</summary>
  
Change the ZNet::connect(ip) method similar to:
```c#
// ZNet
public void Connect(SteamNetworkingIPAddr host) {
  ZNetPeer peer = new ZNetPeer(new ZSteamSocket(host), true);
  this.OnNewConnection(peer);
  ZNet.m_connectionStatus = ZNet.ConnectionStatus.Connecting;
  this.m_connectingDialog.gameObject.SetActive(true);
}
```
to this:
```c#
// ZNet
public void Connect(SteamNetworkingIPAddr host) {
  string ip;
  host.ToString(out ip, false);
  
  TcpClient tcpClient = ZSocket2.CreateSocket();
  IPEndPoint ep = new IPEndPoint(IPAddress.Parse(ip), (int)host.m_port);
  tcpClient.Client.Connect(ep);
  ZNetPeer peer = new ZNetPeer(new ZSocket2(tcpClient, null), true);
  this.OnNewConnection(peer);
  ZNet.m_connectionStatus = ZNet.ConnectionStatus.Connecting;
  this.m_connectingDialog.gameObject.SetActive(true);
}
```
  
</details>

Make sure to include `System.Net.Sockets` and `System.Net`. Enable the ZRpc method name debug (set ZNet.m_DEBUG to true).

### Server-side
211.7: Crossplay got added (9/27/2022)
You can connect now connect to any TCP server. The catch is that the devs didn't include it for some reason. It would have been one small check to see if a Steam server is registered, and if it isnt, try connecting to the server as a custom socket.

<details><summary>210.6</summary>
  
Change the ZNet MonoBehaviour::Awake() method similar to:
```c#
// ZNet
if (ZNet.m_openServer) {
  ZSteamSocket zsteamSocket = new ZSteamSocket();
  zsteamSocket.StartHost();
  this.m_hostSocket = zsteamSocket;
  bool password = ZNet.m_serverPassword != "";
  string versionString = global::Version.GetVersionString();
  ZSteamMatchmaking.instance.RegisterServer(ZNet.m_ServerName, password, versionString, ZNet.m_publicServer, ZNet.m_world.m_seedName);
}
```
to this:
```c#
// ZNet
if (ZNet.m_openServer) {
  ZSocket2 socket = new ZSocket2();
  socket.StartHost(2456);
  this.m_hostSocket = socket;
}
```
  
</details>

Lastly remove the ticket read/write from both the server/client.

Log messages to Player.log file using `ZLog.Log("Your message here")`

Just read the ticket as a dummy and remove the verification `pkg.ReadByteArray();`.

You should now be able to connect with IP to any server (Steam must still be open although it's relatively unused).

See My Valheim Manifesto for more findings and stuff
https://docs.google.com/document/d/1Op_0dvQnDZkcbUCZ3MQMSAsxho7J8jbJeEkfW_8lMRY/edit?usp=sharing
