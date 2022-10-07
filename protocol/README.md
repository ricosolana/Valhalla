# Handshake

The steps below for the handshake are sequential RPC calls performed serially (client -> server -> client -> server -> client -> ...).

The client initiates the handshake by invoking `ServerHandshake`. 

The server invokes `ClientHandshake`, containing a password flag and the salt used in determining the server password. 
The client prompts the user for the password (if required) or proceeds with invoking `PeerInfo`. This involves calling RPC `PeerInfo` with the information about the client (version, player name, auth ticket, etc...). 

The server runs some prerequisite login checks. If the player passes all of them, the server then registers several RPCs, ZDOMan/ZRoutedRPC initialized, Zonesystem sends out map icons and global keys, and then sends PeerInfo containing very basic world data (world name, seed, version, time). Login fails if any of the following is true:
	- incompatible versions
	- banned
	- not whitelisted
	- invalid auth ticket
	- too many players
	- incorrect password
	- uid already connected

The client saves the world data, initializes WorldGenerator, and registers some RPC methods.

The handshake is now complete.

# Spawn point
The client stores their login point. Not the server. The only time the server has control is when a player joins for the first time.

There are one of three login circumstances a player will take to determine their login location:
	- The player has logged out previously on a world with this uid.
	- The player has a custom spawn point (bed).
	- The player will use the server "StartTemple" login point.

I have not experimented with any fresh login being flown in by Hugin.


# Minimap
Although the client/server share like 99% of the code, there are some distinct but minute differences between a dedicated server and the client. The differences are difficult to detect because the client/server specific code is so intertwined.

The server does not care about the Minimap. There is ZPackage code to de/compress, but its only used for the minimap (kinda redundant but idk). The Minimap class contains pindata/pixels/biomecolors/textures for one specific player, and also contains a static instance for easy access. All other classes in Valheim which use single static instances like this use it as a Manager class to handle an aspect of the game. I will claim with a high degree of certainty that the Minimap class is used only by the client. Additionally, there are no ZDOs or RPC registrations that give any sort of a hint that they communicate Minimap data. The **exception** to this are LocationIcons, which is pretty mandatory for the server to send.

# Movement
Clients get invoked `Step` for any (player?) movement. I have only tested this with a single client connected to a modified dedicated server.



# RPCs
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
