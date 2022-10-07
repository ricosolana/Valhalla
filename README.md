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

	ZDO allocation stockpile (ZDOPool)
		I think this is used to hold unused and used ZDO's, mainly to reuse dead ZDO's because of performance concerns. After all, this game uses ZDO's like oxygen, so constantly throwing ZDO's to the garbage collector is not wise. This is the current structure Valheim utilizes regarding ZDO's:
			
			ZDOPeer
				contains a map with references to ZDO's

			ZDO
				contain maps with 

	What's with the 'Z' prefix?
		I think its to keep all the networking stuff togethor as its important and tightly meshed.

	Dispersement of ZDO's

		ZDOs are queued for player dispatchment any time they undergo a change in their state. This involves anytime a member is set, or when the ZDO location.
		
		When ZDOs are sent out, they are not all sent at once. They are sent in batches. These batches are sent out to a different player every .05 seconds, until all the changed ZDOs have been sent to each player. This is most likely to keep the server relatively performant, knowing how expensive ZDO operations are. 

		Here is a depiction of how ZDOs and which ZDOs are queried for sending.

			ZDOMan Update loop:
				server: release zdos
				SendZDOs to next peer (incrementally every .05s):
					CreateSyncList ()
						gets sectors: current, neighboring, distant. assigns them to the passed parameter lists

						ZDOs with an newer revision continue onward for further filters

						priority/sort:
							this is done solely because of performance concerns (regarding network congestion). This is proven because the amount of ZDOs is limited per socket depending on the current send queue size (in bytes). Limit is ~8000?

							ZDOs are sorted based on player distance, and the seconds since last update.

							if 10 or less ZDOs reach this point, add some of the distant ZDOs (far from the player) to the mix

						ZDOs are now sent until the socket is *nearly* congested

						You can tell that the devs really tried to improve network performance, because some code is left over as a "reminder" I guess (SendZDOToPeers, ...). It would be better if they compressed the SendZDOs call like literally every other game. Some mods take care of this, but still why.
				
				SendDestroyed ZDOs to all peers 


	Through some debugging, it appears that every ZDO on the server has its owner set to the server uid. In thinking about what data goes in/out between server/client, I should be able to create a system to send out ZDOs. I'm just not going to be able to rewrite Valheims system completely because its insanely long. Each of these heavy-lifting classes are at least 1k lines. The largest class I've found so far is Player (~5k).

	It seem that when RandEventSystem is invoke remotely by the server -> client for the first time, the player client is able to load the world (visually). This is based off one test however and requires further validation. This oculd also correlate with the player global join message being sent, but its unknown whether this is an effective causation.

		In Game::FindSpawnPoint, this is what dictates the player
		spawning into the world and seeing terrain/trees (after the loading screen 8s wait). The player will spawn in only after 8s have passed and ZNetScene is ready.