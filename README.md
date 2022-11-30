~~This project has been suspended until further notice. This is the more or less my own official server implementation after several previous (failed/abandoned) attempts to build a multiplayer server in C++ from the ground up. I have fulfilled several of my goals with this project: do stuff with Steam, Lua script functionality, powerful networking structure. This is definitly one of the biggest projects I've began on my own. I am calling it quits after realizing how complicated things will get. I have learnt a lot, and will return with another similar project in the future. I am providing a final release version cause why not. Happy Halloween!~~

I am working on this project again. Must closely repair world generation to match the latest Valheim Mistlands update, and other stuff I am likely to miss. Some things I wanna refactor but not sure how exactly; I'll experiment with some designs. 

World should be better integrated into WorldManager/Generator, ZoneLocation with ZoneSystem will be renamed ZoneFeature or ZoneArtifact (ZoneLocation is so terribly named). 

ZNet and Game have A LOT of misc. stuff stored in them that screws up their intended purpose. Global important things should be stored within ValhallaServer or their respective manager namespace. 

Also some namespaces should be better as classes, or? IE NetRouteManager has some methods not intended for direct use externally.

Working on World Generation! Next thing to test out will be getting ZoneFeatures to be generated in correct locations.

I realize Lua scripting is also limited, and every feature I plan to implement has to be implemented in Lua too if functionality is to be bridged between native and Lua. IE custom modded worldgen or custom biomes idk how to implement. Well, biomes are stored simply as a enum bitmask, so not much expandable there without massive overhauling.

So refactoring to keep things consistent and aligned in order to keep motivated

# Valhalla 

## Server
Now compatible with the Steam Valheim client.

## Progress
11/30/2022 + TODO
 - Currently implementing Heightmap and related classes
    - Heightmap makes use of many special Unity features (Raycasting / Terrain physics) mainly for collision and height polling. Im figuring it should be easy to just query a point from the Heightmap float[]. 
    - Many parts are also for rendering the heightmap for the client, in terms of using hoe/cultivator to paint terrain. 
    - It turns out that the Mistlands update new vegetation reads the Heightmap, grabbing pixles from it to determine vegetation placement during (world?) generation. Why. Why should the server store textures? I understand that some kind of texture mask is required for things like farmed/unfarmed land, but this seems extensive... Clients use textures, not servers...

11/14/2022 + TODO
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

10/24/2022 + TODO
 - Clients can now directly join without any patching. 
 
 - Limited Lua support. Lua Rpc/Routed/Sync callbacks will be prioritized next.

 - Must prioritize working on ZDO dispersement, ZNetView, and gameobjects.

 - UnityEngine Random and Mathf.PerlinNoise are required, both of which are in native code. Theres no likely solution besides reverse engineering the original algorithms or being provided equivalent if not native algorithms from Unity themselves.
 
 - All gameobjects in Valheim (especially character-like) are prefabs with predefined members and attributes (such as persistent...). No single component has a specialized class attached, its a one-works-for-all way of functionality (All entities have Humanoid class, which really simplifies the design, but many settings happen to be redundant with this design). 
 
 - So far (limiting to the Chair class), the Chair and other gameobjects when interacted client-side dont make same use of interact/hover/useitem on the server-side. The client at the lowest level sets a ZDO, which gets sent. The server sort of processes and relays this off to other clients.
 
 - The client has too much control over deciding who should be the target of ZNetView calls. This can be easily solved by gating each client->client call by checking if the call is meant to be sent, and is well formed. This is one of the goals of this project.

10/8/2022 + TODO
 - I plan on adding ZDO reading for the server and sending ZDOs. I dont know yet what ZDO controls player visibilty to others on join. Also, some kind of world generation. Valheim terrain generation is client side, as in the client is given the seed, and it generates the terrain. The exception is manually modified terrain, like with a hoe/pickaxe/cultivator.
 
 - Also the client ZRpc occasionally experiences a timeout. I'm not sure why yet, because data is still sent fine between the client and server until the timeout disconenct.
    

10/7/2022 - The client can join and finally see the world, albeit is an ocean.

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
