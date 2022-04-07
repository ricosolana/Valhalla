# Valhalla
Open source Valheim client

## Disclaimer
I am in no way affiliated, endorsed, or associated with Valheim nor its creators.

## Overview
Valheim uses the Steamworks API for both ticket authentication and networking. I could have used the Steam gamenetworkingsockets for C++ to be better compatible with the Valheim server and overall general implementation, but that's too complicated to implement at the moment.

I did find a way around this issue, by digging into the C# source, and using one of the older socket implementations that Valheim used at one point.
Walkthrough of how to do this below:

## Dnspy
I used Dnspy to do all of this, with Valheim v0.202.19. 

What you will need to do is open the `assembly_valheim.dll` located at `./valheim_Data/Managed/assembly_valheim.dll`.

I highly recommend making a backup of the dll before continuing.

For my usages, I have removed any steam layering and lobby stuff to simplify things. It's up to you if you want to do the same. Most of it is just for the server finder and for the smart udp networking that it uses.

In ZNet.cs, in the Unity Awake() method:
```c#
// ZNet.cs
if (ZNet.m_openServer) {
  ZSteamSocket zsteamSocket = new ZSteamSocket();
  zsteamSocket.StartHost();
  this.m_hostSocket = zsteamSocket;
  bool password = ZNet.m_serverPassword != "";
  string versionString = global::Version.GetVersionString();
  ZSteamMatchmaking.instance.RegisterServer(ZNet.m_ServerName, password, versionString, ZNet.m_publicServer, ZNet.m_world.m_seedName);
}
```
Change it to this:
```c#
// ZNet.cs
if (ZNet.m_openServer) {
  ZSocket2 socket = new ZSocket2();
  socket.StartHost(2456);
  this.m_hostSocket = socket;
}
```
What this does is basically uses the socket implementation that wraps a C# TcpClient, and starts it as a server.

The next thing is to change the connect(ip) method for the client:
```c#
public void Connect(SteamNetworkingIPAddr host) {
  ZNetPeer peer = new ZNetPeer(new ZSteamSocket(host), true);
  this.OnNewConnection(peer);
  ZNet.m_connectionStatus = ZNet.ConnectionStatus.Connecting;
  this.m_connectingDialog.gameObject.SetActive(true);
}
```
Change it to this:
```c#
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
This basically uses the existing SteamNetworkingIPAddr structure to get the ip from it and use with ZSocket2.

I though it would make sense to debug the code to make everything works as expected, but good luck doing that, becauseit seems that Unity keeps track of which debug logs originally existed for performance purposes or something idk. `ZLog.Log("hi"); // this never prints`

I really have no clue on how to do dll injections for modding/adding the funcitonality into the base game to support this, so the best I can do is provide the walkthrough and above code.

See My Valheim Manifesto for more findings and stuff
https://docs.google.com/document/d/1Op_0dvQnDZkcbUCZ3MQMSAsxho7J8jbJeEkfW_8lMRY/edit?usp=sharing
