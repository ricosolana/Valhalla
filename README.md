# Avledet

Valheim server written in modern C++. 

Includes Discord integration, Lua scripting, and potential Valheim modding support between clients (WIP).

## Documentation

- I realize the documentation is very lacking and outdated
- I started revising it but will need more time

## Building

### Linux

- IDE (VSCodium)
  - Extensions
    - `CMake Tools` - `ctrl + p` -> `ext install ms-vscode.cmake-tools`
    - `C/C++`
    - `clangd` - `ctrl + p` -> `ext install llvm-vs-code-extensions.vscode-clangd`
    - `Clang-Format` (optional)
    - `EmmyLua` (optional)
- Libraries
  - Steamworks SDK
    - Sign in & Download @ https://partner.steamgames.com/downloads/list
    - Extract somewhere (ie, `~/.local/bin/steamsdk/sdk`)
  - vcpkg
    - `git clone https://github.com/microsoft/vcpkg && cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics && realpath scripts/buildsystems/vcpkg.cmake`
    - Take note of the vcpkg.cmake path
  - Lua
    - Yes, I wrote an entire section specially for lua, because this was 1000% more complicated than it ever needed to be.
    - Classic Valhalla used a combination of static and mostly dynamic linking. This *worked*. When I switched to Linux, things switched
      to mostly static libraries, probably because of vcpkg changes. I stopped the Lua script development for a while after that because of migration-hell between
      Avledet <=> Valhalla, and overall lack of interest and other projects I had going on. Well, the lua library resolution I had no longer worked.
      I spent an entire day talking to chatgpp, the useless chatbot that provides you with hallucinated garbage. I ended up somehow fixing things by requiring that Lua be dynamically linked to the project. The "easiest" way I found to get a dynamic copy of Lua5.5 is via a vcpkg triplet. This actually generates a shared library
      that can be linked by Avledet. There is probably a better way, but for now, I am done.
    - `./vcpkg install lua:x64-linux-dynamic`
- Back to IDE 
  - Clone from VCS `ricosolana/avledet`
  - Prepare Kit
    - `Ctrl + Shift + P` => `scan for kits` => `enter`
    - `Ctrl + Shift + P` => `select a kit` => `enter`
      - Select `GCC 12.x...` or `GCC 13.x...` (the latest one)
  - CMake Arguments
    - Switch to `User` or `Workspace` (your preference)
    - `Ctrl + Shift + P` => `CMake: Open CMake Tools Extension Settings` => `Configure Args`
      - It's time to configure path arguments. These vary based on where you installed things,
        so edit them accordingly. The below is a realistic sample:
        - `-DCMAKE_TOOLCHAIN_FILE=/your/user/here/vcpkg/scripts/buildsystems/vcpkg.cmake`
        - `-DLUA_INCLUDE_DIR=/your/user/here/vcpkg/installed/x64-linux-dynamic/include`
        - `-DLUA_LIBRARY=/your/user/here/vcpkg/installed/x64-linux-dynamic/lib/liblua.so`
        - `-DSTEAMWORKS_SDK=/your/user/here/.local/bin/steamsdk`


    
- CMake
  - CMAKE_TOOLCHAIN_FILE
  - LUA_INCLUDE_DIR
  - LUA_LIBRARY


... 


  - **MSVC**
    - Visual Studio Installer
      - Install C++ Desktop Environment
      - Install CMake Tools
- Install **Steamworks SDK**
  - Download from https://partner.steamgames.com/downloads/list (sign in)
  - Extract to your home directory, Making sure `sdk` is placed inside `steamsdk`
    - Windows: `C:/Users/~/steamsdk/sdk`
    - Linux: `/home/~/.local/bin/steamsdk/sdk`
  - I'm still working on correctly setting env / CMake args / workspace args for path variables
  such as with Steam above...
- Install **vcpkg** `git clone https://github.com/microsoft/vcpkg`
    - Windows: `cd vcpkg && ./bootstrap-vcpkg -disableMetrics && ./vcpkg integrate install`
    - Linux: `cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics`
- Install packages `./vcpkg install abseil asio dpp gtest gtl intrusive-shared-ptr lua nlohmann-json quill range-v3 sol2 tracy unordered-dense magic-enum yaml-cpp zlib zstd`
  - (Optional) Linux via **apt** (if the above aren't recognized by CMake)
    - `sudo apt install libabsl-dev -y`
    - `sudo apt install libmagicenum-dev`
    - `sudo apt install libasio-dev -y`
      - Always failed to find asio, but setting the toolchain worked.
      - https://stackoverflow.com/questions/42034606/compiling-standalone-asio-with-makefile-on-linux
- Open in editor (MSVC, VSC, ...)
  - Install **avledet** or clone from Version Control `ricosolana/avledet`
    - `git clone https://github.com/ricosolana/avledet` or manually...
  - Linux / Windows + VSC(odium):
    - On first launch, must select kit first 'CMAKE Tab -> Launch -> GCC / (Your compiler)'
      - If packages missing, set the toolchain `Ctrl + Shift + P` -> `CMake: Open CMake Tools Extention Settings` -> `Configure Args`,
        add entry: `-DCMAKE_TOOLCHAIN_FILE=/home/~/vcpkg/scripts/buildsystems/vcpkg.cmake` (change as needed).
      - ~~Make sure to clean or manually delete `build` directory~~ *why again?*
    - Launch CMake Configure Task (on both VSC and MSVC it launches automatically on opening CMake Project)
    - To debug plainly, use the cmake debug/launch
    - To debug with command line arguments, use the VSC `RUN AND DEBUG` side tab. Create and configure launch.json beforehand.

- VSCode extensions
  - clangd
    - Installation:
      - `Ctrl + P` , then paste `ext install llvm-vs-code-extensions.vscode-clangd`
    - Must disable C++ Intellisense for clangd to take over:
      - `Ctrl + Shift + P` -> `Preferences: Open User Settings (JSON)`, paste in:
        ```
        "C_Cpp.intelliSenseEngine": "disabled",
            
        //"clangd.path": "/path/to/your/clangd",
        "clangd.arguments": ["-log=verbose", 
                            "-pretty", 
                            "--background-index", 
                            //"--query-driver=/bin/arm-buildroot-linux-gnueabihf-g++", //for cross compile usage
                            //"--compile-commands-dir=/path/to/your/compile_commands_dir/"
                            ]
        ```
    - See https://stackoverflow.com/a/59820115 for more
  - clangd language server `ctrl + p` -> `clangd language server`
  - clang-format
    - cpp file formatter
  - emmy-lua
    - lua file formatter + ...
