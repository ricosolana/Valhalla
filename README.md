I severely overestimated the difficulty of this project, and might be able to have the server within a functioning state quite shortly. I just have to fix Git branches and stuff for the final product, and tidy-up any loose ends.

The only thing I might have difficulty doing is getting the Location instance prefabs from the client so the server can correctly send LocationProxy and stuff. Additionally, the HeightMap might be fully implemented by now. I really done know why the zone generator used raycasts to determine height placement with a heightmap...

ZNetScene and ZNetView are pretty much client only
    an exception is during zone generation where server sends proxy locations to clients
    as ghost objects (only exist primarily as zdos)
    
ZDOMan m_width seems to be much larger than the world size
    according to a redditor, Valheim is 20000m in diameter. This equates to 20000m / 64 = 313 zones in diameter
    m_width however, is 512. This value has probably gone ignored by the devs prior to them figuring out the perfect world size. This could be additionally proven by the ZDO-zone dictionary ('object by outside sector')

# Valhalla 

## Server
This is an implementation of the Valheim Dedicated Server in C++.

The server is currently very barebones and under heavy development. It is only capable of upholding a client connection (the server game state up to RPC_PeerInfo is fully implemented and keepalive pings are functional). Also Lua modding works (I might change it later as I learn new stuff and better techniques). This means that:
  - No gameobjects are sent out to clients
  - Any connected client cannot see other clients
  - This server is essentially a proof of concept device (this might change in the future if the server reaches a point of functionality equal to the Dedicated Valheim Server). Play around with it if you are interested in the inner workings of Valheim.

## Client
No longer being developed. Graphics programming is not my thing :(

## Progress
### 1/5/2023 + TODO
Some alternate thoughts on clients-side simulations:
 - Everything is Unity based, so recreating physics engine will be extremely challenging and time-consuming. 
 - The load of physics simulations could be performed by the client with the least latency as a slightly easier workaround— this only solves one problem, but this still puts the client in control of physics, which I am trying to avoid all together (must never trust the client!).

I suppose an immediate fix to this problem will be to go ahead and trust the client anyways. Proceed with client-side processing so I would not have to deal with physics myself.

Requiring clients to install a mod which adds features such as horses or new furniture models is necessary (It makes sense). Requiring the client to install a mod for anticheat is redundant. They will find a way around this. 

The game revolves around a system similar to P2P, where the server is mainly a middle man for transferring data back and forth, and saving the world occasionally. Ultimately clients have control. Give malicious clients control and they will wreck havoc. So why? It is mainly for the server to not suffer.

Anyways, at the end of this, I am writing a mod to log built-in data (Item statuses, GameObjects, Prefabs, ZoneLocations, Locations, Vegetation...) so I wouldn't have to manually patch the Valheim executable each time I require information across new game updates/releases.

ItemDrop is similar to Bukkit Item entity. ItemDrop.ItemData contains volatile ingame data (current durability, current stack size and current quality). ItemDrop.ItemData.SharedItem seems to be the structure containing constant data about the item, such as item name, max durability, max stack size, unit weight, teleportable...).

EffectList seems to be a list of particles, animations, or grouped things for playing out effects to the client (client-side only).

StatusEffect is spaghetti. Spirit and fire damage cannot overlap. There seems to be a status effect for the player spawning (SE_Spawn). 

There are many interesting/odd/redundant/unused systems, such as SteamManager, all sockets except for SteamSocket2, ZNat, ZConnector(s), NDA funcitonality in FejdManager, EggHatch (only EggGrow used?), there are over 600 censored words for specifically XBOX (I wonder why..), player dream text while sleeping relies on the ZoneSystem global keys.

Gibber seems to be responsible for creating the particle fragments when a structure is broken, which I believe can be better defined by this https://link.springer.com/referenceworkentry/10.1007/3-540-31060-6_145. Gibber are basically particles or fragments. This confused me because Gibber reads/writes 2 ZNetView variables, which I think is kinda weird for something that really should be created only client-side as decorative. Well I seem to be wrong because RPC_CreateFragments exists. Purely for visual effects.

Plains used to be called Heathlands!

Just a hunch but I feel that combat related FPS drops related to many players using AOE weapons near buildings is caused by RPC_CreateFragments. Many particles per building, being hit by multiple players, just maybe will cause stutters. Testing is required.

### 1/3/2023 + TODO
Just a few notes and planning for how to continue this project. A look at the serverside plugin for Valheim shows some features that are crucial to general security and integrity of server tasks related to object spawning and physics (For instance, ZNetScene seemingly has zero player permission checks for RPC_SpawnObject).

As for my previous TODO, the Heightmap will be challenging. It is also pretty crucial to the game as a whole, so I must figure something out. This is all really in the air at the moment; I am thinking on using an array for the heightmap. Unity probably does something similar anyways.

### 11/30/2022 + TODO
 - Currently implementing Heightmap and related classes
    - Heightmap makes use of many special Unity features (Raycasting / Terrain physics) mainly for collision and height polling. Im figuring it should be easy to just query a point from the Heightmap float[]. 
    - Many parts are also for rendering the heightmap for the client, in terms of using hoe/cultivator to paint terrain. 
    - It turns out that the Mistlands update new vegetation reads the Heightmap, grabbing pixles from it to determine vegetation placement during (world?) generation. Why. Why should the server store textures? I understand that some kind of texture mask is required for things like farmed/unfarmed land, but this seems extensive... Clients use textures, not servers...
    - I am really striving for these simple objectives atm:
        - Be able to join as a client
        - Join in at the spawn location (adhering with the Valheim worldgen seed)
        - Placement of prefabs where possible.
            - This might involve having to get the exact ZoneLocation prefab objects and each of the asubobject positions/rotations within a ZoneLocation. Then somehow spawning them (sending client packets and handling them server side, idk).

### 11/14/2022 + TODO
 - Still planning to implement:
    - ZDO/ZDOMan system
    - ZNetView system for wrapping in-game ZDO object instances
    - Representation of Unity GameObjects
        - Everything eventually sent across network, just how
        - ZNetScene capable of instantiating objects locally and remotely
        - Most instantiated objects already contain a ZNetView with ZDO instance, so networking is automatic
            - ZNetView instance is added to ZNetScene which takes care of everything
            - ZNetScene added instances have few usages
        - ZDO prefab hash field
        - ZDO string field referring to prefabs by name

### 10/24/2022 + TODO
 - Clients can now directly join without any patching. 
 
 - Limited Lua support. Lua Rpc/Routed/Sync callbacks will be prioritized next.

 - Must prioritize working on ZDO dispersement, ZNetView, and gameobjects.

 - UnityEngine Random and Mathf.PerlinNoise are required, both of which are in native code. Theres no likely solution besides reverse engineering the original algorithms or being provided equivalent if not native algorithms from Unity themselves.
 
 - All gameobjects in Valheim (especially character-like) are prefabs with predefined members and attributes (such as persistent...). No single component has a specialized class attached, its a one-works-for-all way of functionality (All entities have Humanoid class, which really simplifies the design, but many settings happen to be redundant with this design). 
 
 - So far (limiting to the Chair class), the Chair and other gameobjects when interacted client-side dont make same use of interact/hover/useitem on the server-side. The client at the lowest level sets a ZDO, which gets sent. The server sort of processes and relays this off to other clients.
 
 - The client has too much control over deciding who should be the target of ZNetView calls. This can be easily solved by gating each client->client call by checking if the call is meant to be sent, and is well formed. This is one of the goals of this project.

### 10/8/2022 + TODO
 - I plan on adding ZDO reading for the server and sending ZDOs. I dont know yet what ZDO controls player visibilty to others on join. Also, some kind of world generation. Valheim terrain generation is client side, as in the client is given the seed, and it generates the terrain. The exception is manually modified terrain, like with a hoe/pickaxe/cultivator.
 
 - Also the client ZRpc occasionally experiences a timeout. I'm not sure why yet, because data is still sent fine between the client and server until the timeout disconenct.
    

### 10/7/2022 - The client can join and finally see the world, albeit is an ocean.

![Ocean spawn image](/pics/ocean_spawn.jpg)

## Building
Install vcpkg:
```bash
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg.exe integrate install
```
Install the required libraries:
```bash
.\vcpkg\vcpkg.exe install openssl --triplet=x64-windows
.\vcpkg\vcpkg.exe install robin-hood-hashing --triplet=x64-windows
.\vcpkg\vcpkg.exe install zlib --triplet=x64-windows
.\vcpkg\vcpkg.exe install sol2 --triplet=x64-windows
.\vcpkg\vcpkg.exe install yaml-cpp --triplet=x64-windows
```
