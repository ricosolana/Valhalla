# Valhalla 

## Server
Now compatible with the Steam Valheim client.

## Progress
10/24/2022 + TODO
 - Using Steam API now, clients can join without requiring any Patching. Lua bindings are minorly in progress.

 - Focus must be put into ZDO's and RoutedRPC stuff and game states. The login procedure was comparitively easy. Now it will get difficult.

 - I have to recreate the functionality of some Unity components and classes. Two important features of Unity heavily used by Valheim are Random and Mathf.PerlinNoise, both of which are in native code. PerlinNoise might be possible by mapping Unity input and outputs to a huge texture, but random will not be trivial (I could use a map with several thousands of Random values, but just no..). The algorithms will have to be reverse engineered. For now, I will ignore the problem until I begin work on dungeons and randomly generated locations.

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
```
