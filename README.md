# Valhalla 

## Server
This is an implementation of the Valheim Dedicated Server in C++.

The server is in a currently non-production ready state, however feel free to play around with it.

The goal is to soon have a server with somewhat functional location generation, the ability to see other players in game, and support for loading already existing worlds in the Valheim format. 

A lot of the inner-workings of Valheim are completely client-side, leaving things such as ZDO dispersement, prefabs part of world generation, and smart networking to the server. These are the things Valhalla must implement.

## Client
No longer being developed. Graphics programming is not my thing :(

## Progress
### 2/1/2023 + TODO
 - Pregenerated worlds work (will seem to work perfectly assuming area is loaded)

ZDO's are spawned in correctly and matching the client placed objects. There are still some issues, such as ghost players (something to do with ZDO's not being released). Everything seems functional, because the client is really the one doing the heavy lifting. With these results, I am sure theres something incorrect with the serialize algorithm I used to dump prefabs (since ZoneLocations are skewed weirdly). Anyways, heres the pregenerated world results:

![Pregenerated example with ghost player](/pics/pregenerated.jpg)

### 1/30/2023 + TODO
 - Fixed major oversight on ZoneLocation pkg (was causing incorrect ZoneLocation spawning)
 - ZoneLocation spawning fully works (still working on LocationInstance generation)

ZoneLocations are finally placed correctly in the world. There might be some inconsistencies with height, but it seems fairly consistent so far. I will have to test the other ZoneLocations, but I must first fix the generation algorithm.

Anyways, here it the correct placement (finally):

![Spawn](/pics/zonelocations-instantiation.jpg)

### 1/28/2023 + TODO
 - World seed loading works (.fwl only)
 - Initial spawn location placement ("StartTemple") is accurate (I think?)
 - ZDOs appear to work!

I am very close to getting ZoneLocation generation to correctly work. So far, the starting spawn location and a hundful of other locations are correctly placed in the world (only positional, nothing appears though).

As you can see, ZDO's (sort of) work! I may have intentionally or unintentionally cloned my character:

![cloned player in world](/pics/zdo-demonstrate.jpg)

### 1/27/2023 + TODO
 - WorldGenerator (renamed GeoManager) seems yields results similar to Valheim
 - Generation is more stable now, but still inaccurate compared to the Unity implementation. 
 
![Comparative generation console results](/pics/generation-console.jpg)

There was a patch released for Valheim today (0.213.3), and I have yet to see whether anything with world generation was changed (I doubt it). Most of the fixes seem to be client-side networking (Playfab too) and some new buildings/hats being added (I'll have to update prefabs list).

### 1/25/2023 + TODO
Latest progress:
 - World loading fully works (a world.db file can be read and interpreted; world file emitting is untesting).
 - World generation is fully implemented but untested (some undeniable caveats exist due to UnityEngine-specific features).
 - Prefabs and structure generation will be a complete hit-or-miss (the *.pkg files in ./data).
 - Lua modding has been disabled for the time being. 
 - I was previously using asio for this project (for little experimental additions, such as RCON cli), but was having some Winsock errors (thanks Windows), so I decided to remove anything remotely out of scope to keep the project minimal with hopes of compiling. I have tested nothing extensively up to this point because I have been simply trying this whole month to get it to compile
 - RPC's are now lambda only!
 - I dont fully understand LocationProxy.
 
My efforts in the coming weeks will be primarily to test ZDO dispersement along with ZoneLocation prefabs and ZoneVegetation. 

I was looking into some Valheim mods mainly intended for network performance, and some stood out, with features such as:
 - ZDO compression
 - ZDO min/max send rates
 - Using SendZDOsToPeers in ZDOMan (instead of the ...Peers2 one)
 - Multithreaded steam message sending?

I can easily implement the above with a few tweaks, except for the compression. There is no behaviour in the vanilla Valheim client to support receiving compressed packages, although the devs could have easily implemented this, as several mod authors have already done. They might/not do this in the future, so we will see eventually.

Anyways thats the latest for now.
 
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
