# Valhalla server

This is the primary server branch with planned support for Lua modding.

TODO:
	In Valheim C#, "static" instances are instantiated via gameobjects
	which normally have a lifetime in Unity.

	I plan to streamline this usage into namespaces?

	A system which is constantly utilized for nothing but a container
	for other subsystems should not need a lifetime if its active for like all
	the time.

	tldr,
	ZNet, ZRoutedRPC, ZoneSystem, ZDOMan, ...
	should not be treated as objects but instead as namespaces
	encapsulations for functionality rather than having a lifetime

All RPC's and registration are here: https://github.com/Valheim-Modding/Wiki/wiki/RPC-Method-registrations

Valheim networking system:
	Remote invocation (ZRpc)
		Valheim makes use of this with Remote Procedure Calling (RPC). Its a simple system calling functions located on one machine from another machine. Before any invocations can be made, the local machine registers the function to be invoked by the remote. The remote is now able to call that function. 

	Routed remote invocation (ZRoutedRpc)
		This is the same as the above, but clients can communicate with other clients and local entities (by entities, I'm referring to Valheim game objects). This utilizes a unique identification system called ZDOID.
		
	Routed member synchronization (ZDOMan/ZDO's)
		This is what ZDO's are used for. In Valheim, ZDOMan is the ZDO container and it manages synchronization timings and when to send data to keep the clients up to date. Each ZDO however contains members ranging from int-float-string-Quaternion... The members are contains in maps that contain the id of the item (as a hash), and the object.

	All togethor (ZNetView)
		ZNetView is just an encapsulation mechanism for an object's routing ability. It contains its singular ZDO and includes methods that forward directly to ZRoutedRPC for convenience (passes its owner-id and zdo-id automatically).

	ZoneSystem
		Looking at the source and scenes, I think this is for loading prefab building templates. This includes specially placed tar pit arrangements, Fuling villages, and boss spawner temples.

	Network player (ZNetPeer)
		A simple structure containing the underlying abstracted socket and RPC mostly.

	Net time protocol (ZNtp)
		Pings an NTP server to get the time.

	What's with the 'Z' prefix?
		I think its to keep all the networking stuff togethor as its important and tightly meshed.