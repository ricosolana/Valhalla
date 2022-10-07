# Valhalla 

## Server
This is the non-steam server implementation. It's a TCP server implementation that mimics how Valheim ZSocket2 sends/receives. Joining the server normally through the Valheim client does require a small patch to be made before joining (see [Patching](https://github.com/PeriodicSeizures/Valhalla/tree/server#patching)).

You'll have to manually build the server (I created a new Cmake project within MSVC), or install the release version to try it out.

## Progress
10/8/2022 + TODO
    
    I plan on adding ZDO reading for the server and sending ZDOs. I dont know yet what ZDO controls player visibilty to others on join. Also, some kind of world generation. Valheim terrain generation is client side, as in the client is given the seed, and it generates the terrain. The exception is manually modified terrain, like with a hoe/pickaxe/cultivator.
    
    Also the client ZRpc occasionally experiences a timeout. I'm not sure why yet, because data is still sent fine between the client and server until the timeout disconenct.
    

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

## Patching
This patch works on Valheim 0.211.8 (Crossplay update). The steps below assume you have never used Dnspy. Make sure to backup `assembly_valheim` before continuing.

1. Download and install [Dnspy](https://github.com/dnSpy/dnSpy/releases/tag/v6.1.8). Open it after installing.

2. Within Dnspy, open `assembly_valheim.dll` using **one** of the following methods:
 - Drag-and-drop the `assembly_valheim.dll` into the left hand side of Dnspy `Assembly Explorer`.
 - In the top-left click on `File`->`Open` then locate `assembly_valheim.dll`. It should be wherever you installed the game under `.\valheim_Data\Managed`.
    
2. Enter the shortcut `ctrl`+`shift`+`K` (search assemblies). Search for `fejdstartup`. Double click the first result (.cctor). 

3. Scroll down to line 857. Right-click then click on `Edit Method C#...`.

4. The following line should be on line 46 in the code editor window:
```c#
ZNet.SetServerHost(serverJoin.GetIPString(), (int)serverJoin.m_port, OnlineBackendType.Steamworks);
```

5. Change `OnlineBackendType.Steamworks` to `OnlineBackendType.CustomSocket`.

6. Click on compile in the bottom right. The change should now be reflected in the class view.

7. In the top-left click on `File`->`Save Module`. Click `OK` in the window.

The assembly should now be patched. Open Valheim, and try joining the Valhalla server as you would any other dedicated server.