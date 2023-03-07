# Valhalla 

## Server
This is an implementation of the Valheim Dedicated Server in C++.

Current state of the server:
 - Mostly accurate heightmap
 - World loading/saving (legacy support)
 - World generation
  - Features / Foliage / Dungeons / Creatures / Bosses
 - LUA modding (no documentation yet, I'm lazy)
 - Enhanced customizability

The server is in a currently in a functional state and somewhat usable. I would give it a 70% right now in terms of stability when compared with the Valheim dedicated server. Feel free to play around with it.

## Usage
Command line arguments (all optional):
 - `-root [path]`: Sets the working directory
 - `-colors [0/false 1/true]`: Enable or disable colors (doesnt seem properly formatted in release mode)
 - `-backup-logs [0/false 1/true]`: Whether to backup and compress old logs (will be saved to /logs)
 
All other settings are set in the `server.yml`. If it seems empty, try removing it and another will generate with defaults.

Properly shutdown the server by using ctrl+c.

## Progress
### 3/7/2023 + TODO

![All dungeons](/docs/pics/dungeons.jpg)

The dungeon generator has been mostly fixed and 90% perfect. There are still issues which I have tried to fix by bruting the solution. An example of this are room end-caps, which for some reason keep failing to place 100% of the time. Also, netviews in rooms are quite accurate, but some are not placed correctly. This makes so sense to me because I used the same approach for all netviews when dumping them from the game. Either I'm missing something, or everything is done right and there is something going on behind the scenes. I suspect it has something to do with the parent/child transforms being off axis or something. For instance, the fenring-claw and/or fur pedestal is missing, with the claw floating in some of the deeper rooms. Crystals float in a small lengthy room. Aside from this, everything else is perfect.

I plan to fully implement dungeon regeneration and fixing edge cases in failing end-caps (this has annoyed me the most). I had the roughest time in implementing room overlap detection, and took the over-complicated route. I ended up on a method that was way simpler than I made it out to be, and I think I'll stick with it.

In short, I created a grid system with the rooms that do not rotate, and they function correctly given the rotations are cardinal/90 degree increments. The rooms are only transformed when the ZDO dungeon data is saved relative to the start room.

Anyways, they look quite close to the original:

![All dungeons](/docs/pics/dungeon-forest.jpg)

![All dungeons](/docs/pics/dungeon-swamp.jpg)

![All dungeons](/docs/pics/dungeon-mountain.jpg)

![All dungeons](/docs/pics/dungeon-mistlands.jpg)

### 2/28/2023 + TODO

![Dungeons broken](/docs/pics/dungeons-experimental.jpg)

I'm currently working on dungeon generator, and as you can see there are some issues. I am planning to get the base-dungeon generation working, then possibly expand into dungeon reset intervals and maybe even deeper dungeons (I dont really like the zone-restricted dungeons at the moment, and am thinking on making generated-dungeons go beyond their zone for the future).

Most of the process involved when creating a new module such as what I did with ZoneManager and ZDOManager consists of porting the c# to c++, then trying to get it to compile, then making sure the library functions being used and everything works the same way (like std::stable_sort vs std::sort and the equivalent IEnumerator orderby in c#). Then I have to somehow get the data from Valheim into a format readable by Valhalla. There are a lot of other Components in every gameobject that I really have no clue how I will implement, and I don't really want to go there. This is not supposed to be turning into a Component/Unity based server with parent-child objects. Valheim makes heavy use of these concepts, and I am basically trying to split all that apart and make it functional in an immediately understood sense (that makes no sense). 

I hope to finish this without too much hassle or indirection caused by the way Valheim is internally shaped.

### 2/18/2023 + TODO

World saving is fixed (I was wrongly writing a utf8 char instead of byte for ZDO members, hence turning map member-data into garbage). 

I would like to address networking next, more specifically, client-side processing and latency issues caused by it for other clients. The zdo-owner manages physics, packets, and processing for that ZDO. Clients in Valheim take control of every object within 4 zones in radius around them. Clients who are not the owner, who for instance, attack something, that data has to travel to the server, to the owner-client, then back to the server, then to the initial client. That is pretty lengthy. There is no need for this when the owner is 55m away in a house afk, and other players are doing whatever, dealing with 125ms latency and throttling due to limited bandwidth. 

I will test out some applications to doing this and see how well it works. I am using Clumsy latency simulator https://github.com/jagt/clumsy for replicating realistic situations with poor latency. It so far works well, and will give a good demonstration when I test high latencies and ways to accomodate against it. I could also assign the least laggy player with being the owner, only if the current owner has very poor latency. I am not fully sure which approach is best, but this is a start.

### 2/15/2023 + TODO

Development might slow down for the next few weeks, but I have some things in mind regarding world saving. World loading seems to work flawlessly, but world saving appears to be problematic. Several areas regarding ZDOs might involve more than ZDO serialization, and the affected parts of this are:
 - Chests (Container): Completely emptied
 - Terrain (TerrainComp): Deleted / Missing?
 - Rocks (MineRock): Completely reset to original state
 - Wards (PrivateArea): Seem to be disabled, and possibly missing owner (but permitted are not reset)
 - Beds (Bed): Claimee cleared, and appears non-interactive upon clicking
 - Signs (Sign): Completely cleared
 - Fermenter (Fermenter): Completely reset
 - One-time spawners (SpawnPrefab?): Completely reset (IE Greydwarf houses will respawn mobs around them on reload when player approaches

These are some things I have noticed to be broken, along with their descriptions. This is just an observation, and no in-depth experimenting has been carried out. There could be more, or I might have misunderstood some occurrences. For some reason, a Windows Update seemed to have broken Dnspy debugging with Valheim. I have not tried in over a week, so it might be different now, but I'm not too optimistic... Debugging has really helped me solve issues like this in the past, so I hope it will work again soon.

I've also worked on the Lua API a bit, and would like to try out documentation one day, just to start explaining things in the API. I will do this only once most of the functions are set-in-stone and the design seems stable and solid enough (mostly). The whole point of this project/server is easy customizability, something the Valheim Dedicated server does not have. I feel like mods break the game, and are a hassle to deal with... Quality of life mods make sense, but client-side anticheats do not...

### 2/11/2023 + TODO

![Vegetation rotations](/docs/pics/experimental-rotations.jpg)

I am playing around with vegetation rotations using some implementations of Unity.Quaternion methods Ive found online. I'm surprised anyone has done this.

Mistland 'cliffs' are finally cliffs, and not jagged spires pointing straight up. I also have a hunch as to why mist is placed everywhere (and trees are not placed as frequently), and it has to do with world generation and preworld generation. Since the mistlands update, a major feature was added uniquely for the mistlands, and its a vegetation color map. I dont really understand how it works, but it basically determines what vegetation is allowed where. I think my implementation of this is inaccurate or completley incorrect. I'll have to do more testing.

I have also implemented rather intricate lua modding functionality, where I've created two mods making use of the api for example. Valheim portals linking doesnt really fit in, so I created a mod for it. I also created a ZDO networking compression mod based on the Comfy Valheim server implementation. It needs more testing though.

What I plan on doing next is focusing on location generation, and primarily dungeons next, which seems daunting. Also fixing the mist blanketing the mistlands.

### 2/7/2023 + TODO

Ive implemented several generation fixes, such as heightmap normals, primitive char-utf8 stream read/write support, and completely overhauled the netpackage system into two seperate portions (reader + writer).

Some of the features in Valheim are kinda miscellaneously handled on the side, such as portal linking, bed sleep/awake, among a variad of other things that dont really have a particular fitting in the server. Because of this and in general, I will be reintegrating Lua modding functionality over the next few days. 

A bit ago I removed many subsystems to make the server work (because of weird compile errors I just couldnt fix). One of these was the mod manager, and the last commit featuring it is here https://github.com/PeriodicSeizures/Valhalla/tree/57c6d19a853f3154061bdaf316672f4bedc25f12/src. I plan to approach things differently this time around. Instead of creating a Lua vm for every script, I want them to be able to communicate with each other, so they might have the same Lua instance. I have not gone too deep into what sol provides, but I've read there are sol::environments that provide a virtual space for use within a single script? I am not sure.

Also another Valheim update was released today.

### 2/5/2023 + TODO

Ive made many changes, primarily to NetPackage to make it more specifically read or write oriented in certain scenarios. 

I've identified a relatively major bug related to ZDOs. Since UTF-8 isnt completely handled, UTF-8 encoded counts up to 127 are supported. It turns out that dungeon locations set many ZDO members, more than this limit. I found this out while trying to load a large older world. It seems that I will have to fully enter the world of character encoding...

~~The server seems mostly playable. I am currently making structural changes that will make things more readable and hopefully safe. Generation seems nice, with the exception of mistlands (mist everywhere) and mountains partly.~~

### 2/3/2023 + TODO
 - Vegetation naturally generated at semi-correct height and fine dispersement

Vegetation now generates throughout the world, and correctly too according to biome. A few things to note: vegetation placement currently does not match an equal Valheim-generated world vegetation, but comes somewhat close with results appearing similar to that of a Valheim world (I do not have a major solution to this, but I will work-around this caveat by implementing some simple radius checks for vegetation, because they are frequently crammed together), also, vegetation heights are sometimes slightly above ground, but StaticPhysics in the client will fix this.

![Vegetation generation](/docs/pics/natural-generation.jpg)

You can see in the left of the image how the small trees are in the open clearing, where this never happens in Valheim as far as I know. 

### 2/1/2023 + TODO
 - Pregenerated worlds work (will seem to work perfectly assuming area is loaded)

ZDO's are spawned in correctly and matching the client placed objects. There are still some issues, such as ghost players (something to do with ZDO's not being released). Everything seems functional, because the client is really the one doing the heavy lifting. With these results, I am sure theres something incorrect with the serialize algorithm I used to dump prefabs (since ZoneLocations are skewed weirdly). Anyways, heres the pregenerated world results:

![Pregenerated example with ghost player](/docs/pics/pregenerated.jpg)

### 1/30/2023 + TODO
 - Fixed major oversight on ZoneLocation pkg (was causing incorrect ZoneLocation spawning)
 - ZoneLocation spawning fully works (still working on LocationInstance generation)

ZoneLocations are finally placed correctly in the world. There might be some inconsistencies with height, but it seems fairly consistent so far. I will have to test the other ZoneLocations, but I must first fix the generation algorithm.

Anyways, here it the correct placement (finally):

![Spawn](/docs/pics/zonelocations-instantiation.jpg)

### 1/28/2023 + TODO
 - World seed loading works (.fwl only)
 - Initial spawn location placement ("StartTemple") is accurate (I think?)
 - ZDOs appear to work!

I am very close to getting ZoneLocation generation to correctly work. So far, the starting spawn location and a hundful of other locations are correctly placed in the world (only positional, nothing appears though).

As you can see, ZDO's (sort of) work! I may have intentionally or unintentionally cloned my character:

![cloned player in world](/docs/pics/zdo-demonstrate.jpg)

### 1/27/2023 + TODO
 - WorldGenerator (renamed GeoManager) seems yields results similar to Valheim
 - Generation is more stable now, but still inaccurate compared to the Unity implementation. 
 
![Comparative generation console results](/docs/pics/generation-console.jpg)

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

![Ocean spawn image](/docs/pics/ocean_spawn.jpg)

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
