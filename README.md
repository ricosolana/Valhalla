Development will continue by next week (by 1/10/2023 hopefully).

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
1/3/2023 + TODO
Just a few notes and planning for how to continue this project. A look at the serverside plugin for Valheim shows some features that are crucial to general security and integrity of server tasks related to object spawning and physics (For instance, ZNetScene seemingly has zero player permission checks for RPC_SpawnObject).

As for my previous TODO, the Heightmap will be challenging. It is also pretty crucial to the game as a whole, so I must figure something out. This is all really in the air at the moment; I am thinking on using an array for the heightmap. Unity probably does something similar anyways.

11/30/2022 + TODO
 - Currently implementing Heightmap and related classes
    - Heightmap makes use of many special Unity features (Raycasting / Terrain physics) mainly for collision and height polling. Im figuring it should be easy to just query a point from the Heightmap float[]. 
    - Many parts are also for rendering the heightmap for the client, in terms of using hoe/cultivator to paint terrain. 
    - It turns out that the Mistlands update new vegetation reads the Heightmap, grabbing pixles from it to determine vegetation placement during (world?) generation. Why. Why should the server store textures? I understand that some kind of texture mask is required for things like farmed/unfarmed land, but this seems extensive... Clients use textures, not servers...
    - I am really striving for these simple objectives atm:
        - Be able to join as a client
        - Join in at the spawn location (adhering with the Valheim worldgen seed)
        - Placement of prefabs where possible.
            - This might involve having to get the exact ZoneLocation prefab objects and each of the asubobject positions/rotations within a ZoneLocation. Then somehow spawning them (sending client packets and handling them server side, idk).

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
