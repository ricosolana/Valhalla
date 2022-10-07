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