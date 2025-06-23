# Compile settings for platforms (06/15/2025):
#   Linux / VSCod(ium)e:
#       Extensions
#           Ctrl+P
#           ext install ms-vscode.cpptools
#           ext install llvm-vs-code-extensions.vscode-clangd
#           ext install ms-vscode.cmake-tools
#           ext install xaver.clang-format
#       sudo apt install clang-format
#           Format with Ctrl+Shift+I
#       Select Kits (Compiler) 
#       Variants (Release / Debug)



#OUTDATED (but might still work):
# If using CLion:
# - use msvc compiler 2022
# - specify the vcpkg toolchain file location
# - specify arm64 architecture

# If getting Winsock multiple includes errors:
# - This setup is not recommended, but sometimes Windows is just too rage-inducing
# - Locate Windows.h and remove the define section containing WIN32_LEAN_AND_MEAN
# - Add an '#include <Winsock2.h>' replacing the above section
# - Some stack overflow 'enthusiasts' dislike this method, however its the only one which worked for me, so yea..
#   I will be removing asio anyways because I consider RCON to be semi-insecure and too extra/finicky, so the forbidden technique above isnt needed!

# To set command line program arguments: 
# - https://unrealistic.dev/posts/setting-debug-parameters-with-cmake-tools-for-visual-studio
# - right click the cmakelists.txt -> 'add debug configuration''
# - add a new key 'args' followed by args

# To use the luasocket library with the Lua mods (useful for debugging with Zerobrane)
# - Use vcpkg to easily build the sources, then:
# - Copy the folders from luasocket/bin/ to lua root dir
# - Copy the folders from luasocket/lua/ to lua root dir



#################
#
#   Projects
#
#################
cmake_minimum_required(VERSION 3.20)
project(avledet)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CXX_EXTENSIONS OFF)

# TODO change this file impl or how defines are 'defined'
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(VHSettings)

# ignore annoying msvc warnings
if(WIN32)
   # remove annoying min/max getting in the way of other min/max functions
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    set(VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/run/)
    add_definitions("/wd5045") # spectre
    add_definitions("/wd4244") # float loss
    add_definitions("/wd4267") # int loss
    add_definitions("/wd4996") # unsafe cstd functions
    add_definitions("/wd4702") # unreachable code

    target_compile_options(avledet PRIVATE /bigobj "/diagnostics:caret")

    #if(CMAKE_BUILD_TYPE MATCHES Release)
    #    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
    #endif()
else()
    #TODO reenable after tests and unit-migration
    #set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wconversion -Werror -Wno-unknown-pragmas -Wno-unused-variable -Wno-deprecated-declarations") # -Wno-unused-but-set-variable")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")



#################
#
#   Steamworks
#
#################

# TODO
#   Allow USER to specify sdk path via args
if(WIN32)
    message(FATAL_ERROR "NYI")
    #set(STEAMWORKS_SDK_LOCATION "C:/Users/$ENV{USERNAME}/steamsdk")
    #set(STEAMWORKS_SDK_SOURCE_DIR "${STEAMWORKS_SDK_LOCATION}/sdk/public/steam")
    #set(STEAMWORKS_SDK_SHARED_DIR "${STEAMWORKS_SDK_LOCATION}/sdk/redistributable_bin/win64/steam_api64.lib")
else()
    set(STEAMWORKS_SDK_LOCATION "$ENV{HOME}/.local/bin/steamsdk")
    set(STEAMWORKS_SDK_SOURCE_DIR "${STEAMWORKS_SDK_LOCATION}/sdk/public/steam")
    set(STEAMWORKS_SDK_SHARED_DIR "${STEAMWORKS_SDK_LOCATION}/sdk/redistributable_bin/linux64/libsteam_api.so")
endif()

if(NOT EXISTS "${STEAMWORKS_SDK_SHARED_DIR}")
    message (FATAL_ERROR "Steamworks not found")
else()

endif()

message("Steamworks: ${STEAMWORKS_SDK_LOCATION}")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt
     DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

file(COPY ${STEAMWORKS_SDK_SHARED_DIR}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)



#################
#
#  Dependencies
#
#################
### abseil:x64-linux                                  20250127.1#1        Abseil is an open-source collection of C++ libra...
### asio:x64-linux                                    1.32.0              Asio is a cross-platform C++ library for network...
### dpp:x64-linux                                     10.1.2              D++ Extremely Lightweight C++ Discord Library.
### gtest:x64-linux                                   1.17.0#1            Google Testing and Mocking Framework
### gtl:x64-linux                                     1.2.0               Greg's Template Library of useful classes.
### lua:x64-linux                                     5.4.8               A powerful, fast, lightweight, embeddable script...
### nlohmann-json:x64-linux                           3.12.0              JSON for Modern C++
### openssl:x64-linux                                 3.5.0#1             OpenSSL is an open source project that provides ...
### opus:x64-linux                                    1.5.2               Totally open, royalty-free, highly versatile aud...
### pthreads:x64-linux                                3.0.0#14            Meta-package that provides PThreads4W on Windows...
### quill:x64-linux                                   10.0.0              Asynchronous Low Latency C++ Logging Library
### range-v3:x64-linux                                0.12.0#4            Range library for C++14/17/20, basis for C++20's...
### sol2:x64-linux                                    3.5.0               Sol3 (sol2 v3.0) - a C++ <-> Lua API wrapper wit...
### tracy:x64-linux                                   0.11.1#2            A real time, nanosecond resolution, remote telem...
### tracy[crash-handler]:x64-linux                                        Enable crash handler
### unordered-dense:x64-linux                         4.5.0               A fast & densely stored hashmap and hashset base...
### vcpkg-cmake-config:x64-linux                      2024-05-23          
### vcpkg-cmake-get-vars:x64-linux                    2025-05-29          
### vcpkg-cmake:x64-linux                             2024-04-23          
### yaml-cpp:x64-linux                                0.8.0#3             yaml-cpp is a YAML parser and emitter in C++ mat...
### zlib:x64-linux                                    1.3.1               A compression library
### zstd:x64-linux                                    1.5.7               Zstandard - Fast real-time compression algorithm

include(CTest)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED) # vcpkg install gtest
find_package(ZLIB REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED) # https://github.com/martinus/unordered_dense?tab=readme-ov-file#21-installing-using-cmake

if (NOT DEFINED AVL_ENABLE_SCRIPTING OR AVL_ENABLE_SCRIPTING)
    message("Lua: enabled")
    find_package(lua CONFIG REQUIRED) # could replace with LuaJIT
    find_package(sol2 CONFIG REQUIRED)
else()
    message("Lua: disabled")
endif()

if (NOT DEFINED AVL_DISCORD_INTEGRATION OR AVL_DISCORD_INTEGRATION)
    message("Discord: enabled")
    find_package(OpenSSL REQUIRED)
    find_package(dpp CONFIG REQUIRED)
else()
    message("Discord: disabled")
endif()

find_path(GTL_INCLUDE_DIRS "gtl/adv_utils.hpp")



#################
#
#   Options
#
#################

# working dir
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(DEFINED AVL_ENABLE_SCRIPTING)
    add_compile_definitions(AVL_ENABLE_SCRIPTING=$<BOOL:${AVL_ENABLE_SCRIPTING}>) 
endif()

if(DEFINED AVL_DISCORD_INTEGRATION)
    add_compile_definitions(AVL_DISCORD_INTEGRATION=$<BOOL:${AVL_DISCORD_INTEGRATION}>) 
endif()

if(DEFINED AVL_USE_PREFABS)
    add_compile_definitions(AVL_USE_PREFABS=$<BOOL:${AVL_USE_PREFABS}>) 
endif()

if(DEFINED AVL_ZONE_GENERATION)
    add_compile_definitions(AVL_ZONE_GENERATION=$<BOOL:${AVL_ZONE_GENERATION}>) 
endif()

add_compile_definitions(SOL_ALL_SAFETIES_ON=1)
add_compile_definitions(SOL_PRINT_ERRORS=0) #because stdout duplicate errors are anooooying



#################
#
#    Projects
#
#################

add_subdirectory("library")

add_subdirectory("core")
