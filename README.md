# Valhalla 

## Server
This is the non-steam server implementation. It's a TCP server implementation that mimics how Valheim ZSocket2 sends/receives. Joining the server normally through the Valheim client does require a small patch to be made before joining (see [Patching](https://github.com/PeriodicSeizures/Valhalla/tree/server#patching)).

You'll have to manually build the server (I created a new Cmake project within MSVC).

## Progress
10/7/2022 - The client can join and finally see the world, albeit is an ocean.


## Building
For the server, assuming you have no dependencies or required stuff installed, open a shell and install vcpkg, then bootstrap it:
```bash
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
```
Now the libraries to install:
```bash
.\vcpkg\vcpkg.exe integrate install
.\vcpkg\vcpkg.exe install openssl --triplet=x64-windows
.\vcpkg\vcpkg.exe install robin-hood-hashing --triplet=x64-windows
.\vcpkg\vcpkg.exe install zlib --triplet=x64-windows
.\vcpkg\vcpkg.exe install sol2 --triplet=x64-windows
```

## Patching
This patch works on 0.211.8. The steps are different for different versions, especially versions before 0.211.7 (the Crossplay update). The steps below assume you have never used Dnspy.

Install [Dnspy](https://github.com/dnSpy/dnSpy/releases/tag/v6.1.8). It might prompt you to install. Open Dnspy if it doesn't automatically open. Once it's opened, you can now open the valheim c# assembly. 

Make sure to backup the assembly. To open the .dll with Dnspy:
 - Drag-and-drop the `assembly_valheim.dll` into the left hand side of Dnspy `Assembly Explorer`.
 - Or click `File`->`Open` then find the `assembly_valheim.dll`. It should be located wherever you installed the game under `.\valheim_Data\Managed`.
    
Press hotkey `ctrl`+`shift`+`K` (search assemblies). Search for `fejdstartup`. The bottom search list will show a few hundred or so matches. Double click the first one (.cctor). Scroll down to line 857. 

Right click, `Edit Method C#...`.

You will see this line in the editor screen (should now be on relative line 46):
```c#
ZNet.SetServerHost(serverJoin.GetIPString(), (int)serverJoin.m_port, OnlineBackendType.Steamworks);
```

Change `OnlineBackendType.Steamworks` to `OnlineBackendType.CustomSocket`.

Click on compile in the bottom right.

The editor window should automatically close, and the change should now be reflected. Click on `File`->`Save Module`. Click ok in the window that opens.

Open Valheim, and try joining the Valhalla server as you would any other dedicated server.