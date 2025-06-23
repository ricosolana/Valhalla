# Avledet

Valheim server written in modern C++. 

Includes Discord integration, Lua scripting, and potential Valheim modding support between clients (WIP).

## Dependencies
Requires the following dependencies with versions (./vcpkg list):

 - abseil:x64-linux                                  20250127.1#1        Abseil is an open-source collection of C++ libra...
 - asio:x64-linux                                    1.32.0              Asio is a cross-platform C++ library for network...
 - dpp:x64-linux                                     10.1.2              D++ Extremely Lightweight C++ Discord Library.
 - gtest:x64-linux                                   1.17.0#1            Google Testing and Mocking Framework
 - gtl:x64-linux                                     1.2.0               Greg's Template Library of useful classes.
 - lua:x64-linux                                     5.4.8               A powerful, fast, lightweight, embeddable script...
 - nlohmann-json:x64-linux                           3.12.0              JSON for Modern C++
 - openssl:x64-linux                                 3.5.0#1             OpenSSL is an open source project that provides ...
 - opus:x64-linux                                    1.5.2               Totally open, royalty-free, highly versatile aud...
 - pthreads:x64-linux                                3.0.0#14            Meta-package that provides PThreads4W on Windows...
 - quill:x64-linux                                   10.0.0              Asynchronous Low Latency C++ Logging Library
 - range-v3:x64-linux                                0.12.0#4            Range library for C++14/17/20, basis for C++20's...
 - sol2:x64-linux                                    3.5.0               Sol3 (sol2 v3.0) - a C++ <-> Lua API wrapper wit...
 - tracy:x64-linux                                   0.11.1#2            A real time, nanosecond resolution, remote telem...
 - tracy[crash-handler]:x64-linux                                        Enable crash handler
 - unordered-dense:x64-linux                         4.5.0               A fast & densely stored hashmap and hashset base...
 - vcpkg-cmake-config:x64-linux                      2024-05-23          
 - vcpkg-cmake-get-vars:x64-linux                    2024-09-22          
 - vcpkg-cmake:x64-linux                             2024-04-23          
 - yaml-cpp:x64-linux                                0.8.0#3             yaml-cpp is a YAML parser and emitter in C++ mat...
 - zlib:x64-linux                                    1.3.1               A compression library
 - zstd:x64-linux                                    1.5.7               Zstandard - Fast real-time compression algorithm

Using anything later, or earlier, will (99% Guaranteed, without your money back) result in compiler errors from hell.

The above is a collective list of my vcpkg. Some dependencies are unused, but good to have as future changes are implemented.

## Building

- Steamworks SDK 
  - Download from https://partner.steamgames.com/downloads/list
  - Extract to your home directory. `sdk` must be placed inside `steamsdk`
    - Windows: `C:/Users/~/steamsdk/sdk`
    - Linux: `/home/~/.local/bin/steamsdk/sdk`
- Install avledet
  - `git clone https://github.com/ricosolana/avledet`
- Install vcpkg
    - `git clone https://github.com/microsoft/vcpkg`
    - Windows: `cd vcpkg && bootstrap-vcpkg.bat -disableMetrics && vcpkg integrate install`
    - Linux: `cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics`
- Install packages 
  - `./vcpkg install abseil magic-enum asio gtest quill unordered-dense yaml-cpp`
  - ~~Linux via apt
    - sudo apt install libabsl-dev -y
    - sudo apt install libmagicenum-dev
    - sudo apt install libasio-dev -y
      - Always failed to find asio, but setting the toolchain worked.
      - https://stackoverflow.com/questions/42034606/compiling-standalone-asio-with-makefile-on-linux
        ~~

- Open avledet using your favorite editor (MSVC, VSC, ...)
  - Launch CMake configuration
  - VSC:
    - On first launch, must select kit first 'CMAKE Tab -> Launch -> GCC / (Your compiler)'
      - If packages missing, set the toolchain `Ctrl + Shift + P` -> `CMake: Open CMake Tools Extention Settings` -> `Configure Args`,
        add entry: `-DCMAKE_TOOLCHAIN_FILE=/home/~/vcpkg/scripts/buildsystems/vcpkg.cmake` (change as needed).
      - Make sure to clean or manually delete `build` directory
    - To debug plainly, use the cmake debug/launch
    - To debug with command line arguments, use the VSC `RUN AND DEBUG` side tab. Create and configure launch.json beforehand.

- Useful Utilities
  - clangd (Super Intellisense Extension for vscode)
    - Installation:
      - `Ctrl + P` , then paste `ext install llvm-vs-code-extensions.vscode-clangd`
    - Must disable C++ Intellisense for clangd to take over:
      - `Ctrl + P`, edit settings.json (User settings), paste following:
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
