# Avledet

Valheim server written in modern C++. 

Includes Discord integration, Lua scripting, and potential Valheim modding support between clients (WIP).

## TODO

- fully rename project to avledet
- remove discord integration
- remove tracy profiler
- fix worldgen
- other stuff thats cool! like better lua...
- prune tmp lua test case scripts

## Documentation

- I realize the documentation is very lacking and outdated
- I started revising it but will need more time

## Building

### Dependencies
The below is my `./vcpkg list`:

 | name                           | version    | description                                         |
 |--------------------------------|------------|-----------------------------------------------------|
 | abseil:x64-linux               | 20250814.1 | Abseil is an open-source collection of C++ libra... |
 | asio:x64-linux                 | 1.32.0     | Asio is a cross-platform C++ library for network... |
 | gtest:x64-linux                | 1.17.0#2   | Google Testing and Mocking Framework                |
 | gtl:x64-linux                  | 1.2.0      | Greg's Template Library of useful classes.          |
 | intrusive-shared-ptr:x64-linux | 1.9        | Intrusive reference counting smart pointer, high... |
 | lua:x64-linux                  | 5.5.0#1    | A powerful, fast, lightweight, embeddable script... |
 | magic-enum:x64-linux           | 0.9.7#1    | Header-only C++17 library provides static reflec... |
 | nlohmann-json:x64-linux        | 3.12.0#2   | JSON for Modern C++                                 |
 | openssl:x64-linux              | 3.6.1#2    | OpenSSL is an open source project that provides ... |
 | opus:x64-linux                 | 1.5.2#1    | Totally open, royalty-free, highly versatile aud... |
 | pthreads:x64-linux             | 3.0.0#14   | Meta-package that provides PThreads4W on Windows... |
 | quill:x64-linux                | 11.0.2     | Asynchronous Low Latency C++ Logging Library        |
 | range-v3:x64-linux             | 0.12.0#4   | Range library for C++14/17/20, basis for C++20's... |
 | sol2:x64-linux                 | 3.5.0#1    | Sol3 (sol2 v3.0) - a C++ <-> Lua API wrapper wit... |
 | tracy:x64-linux                | 0.13.1     | A real time, nanosecond resolution, remote telem... |
 | tracy[crash-handler]:x64-linux |            | Enable crash handler                                |
 | unordered-dense:x64-linux      | 4.8.1      | A fast & densely stored hashmap and hashset base... |
 | vcpkg-cmake-config:x64-linux   | 2024-05-23 |                                                     |
 | vcpkg-cmake-get-vars:x64-linux | 2025-05-29 |                                                     |
 | vcpkg-cmake:x64-linux          | 2024-04-23 |                                                     |
 | yaml-cpp:x64-linux             | 0.9.0      | yaml-cpp is a YAML parser and emitter in C++ mat... |
 | zlib:x64-linux                 | 1.3.1      | A compression library                               |
 | zstd:x64-linux                 | 1.5.7      | Zstandard - Fast real-time compression algorithm    |

### Installation

- Install your favorite **IDE** (VSC / MSVC)
  - **VSCodium**
    - Extensions
      - `CMake Tools` - `ctrl + p` -> `ext install ms-vscode.cmake-tools`
      - `C/C++` - unfortunately youll have to install manually
      - `clangd` - `ctrl + p` -> `ext install llvm-vs-code-extensions.vscode-clangd`
      - `Clang-Format` (optional; automatic C++ code formatting)
      - `EmmyLua` (optional; Lua formatting + highlighting)
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
