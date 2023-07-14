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

##########
# Project
##########
cmake_minimum_required (VERSION 3.14)
project(Valhalla)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(VHSettings)
include(get_steamworks_sdk)
include(get_cpm)

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
endif()



option(TRACY_ENABLE "" OFF)

############
# Libraries
############
find_package(Threads REQUIRED)
if (NOT DEFINED VH_USE_ZLIB OR VH_USE_ZLIB)
    find_package(ZLIB REQUIRED)
endif()
find_package(range-v3 CONFIG REQUIRED)
if (NOT DEFINED VH_USE_MODS OR VH_USE_MODS)
    message("Mods: enabled")
    find_package(lua CONFIG REQUIRED)
    find_package(sol2 CONFIG REQUIRED)
endif()
find_package(yaml-cpp CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
if (NOT DEFINED VH_DISCORD_INTEGRATION OR VH_DISCORD_INTEGRATION)
    message("Discord: enabled")
    find_package(dpp CONFIG REQUIRED)
endif()
find_package(quill CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)

CPMAddPackage(
    NAME unordered_dense
    GITHUB_REPOSITORY martinus/unordered_dense
    VERSION 4.0.0
)

# remove annoying min/max getting in the way of other min/max functions
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

# ignore annoying msvc warnings
if(WIN32)
    add_definitions("/wd4244") # float loss
    add_definitions("/wd4267") # int loss
    add_definitions("/wd4996") # c functions it seems unsafe
endif()



##########
# Linking
##########
add_executable(${PROJECT_NAME}
    "src/Main.cpp"

    
    "src/DiscordManager.cpp"
    "src/DungeonGenerator.cpp"
    "src/DungeonManager.cpp"
    "src/RandomEventManager.cpp"
    "src/GeoManager.cpp"
    "src/Heightmap.cpp"
    "src/HeightmapBuilder.cpp"
    "src/HeightmapManager.cpp"
    
    "src/ModManager.cpp"
    "src/NetAcceptorSteamDedicated.cpp"
    "src/NetAcceptorTCP.cpp"
    "src/NetManager.cpp"    
    "src/NetSocketSteam.cpp"
    "src/Peer.cpp"
    "src/PrefabManager.cpp"
    "src/Quaternion.cpp"
    "src/NetSocketPlayback.cpp"
    "src/DungeonRoom.cpp"
    "src/RouteManager.cpp"
    
    "src/ValhallaServer.cpp"
    "src/Vector.cpp"
    "src/VUtils.cpp"
    "src/VUtilsMath.cpp"
    "src/VUtilsMathf.cpp"
    "src/VUtilsPhysics.cpp" 
    "src/VUtilsRandom.cpp"
    "src/VUtilsResource.cpp"
    "src/VUtilsString.cpp"
    "src/WorldManager.cpp"
    "src/ZDO.cpp"
    "src/ZDOManager.cpp"
    "src/ZDOID.cpp"
    "src/ZoneManager.cpp"
    "src/Tests.cpp"
 "include/RandomSpawn.h" "src/NetSocketTCP.cpp"  "src/NetSocketProxy.cpp")

find_path(GTL_INCLUDE_DIRS "gtl/adv_utils.hpp")

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${STEAMWORKS_SDK_SOURCE_DIR}
    PUBLIC ankerl::unordered_dense
    PRIVATE ${GTL_INCLUDE_DIRS}
)

add_compile_options($<$<CONFIG:Debug>:-fsanitize-address>)
add_link_options($<$<CONFIG:Debug>:-fsanitize-address>)

set(CMAKE_CXX_STANDARD 23)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

if (NOT DEFINED VH_USE_MODS OR VH_USE_MODS)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC lua sol2
    )
endif()

if (NOT DEFINED VH_DISCORD_INTEGRATION OR VH_DISCORD_INTEGRATION)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE dpp::dpp
    )
endif()

if (NOT DEFINED VH_USE_ZLIB OR VH_USE_ZLIB)
    target_link_libraries(${PROJECT_NAME}
        ZLIB::ZLIB
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC Threads::Threads
    ${STEAMWORKS_SDK_BINARY_DIR}
    yaml-cpp
    PRIVATE Tracy::TracyClient
    PRIVATE $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
    PRIVATE quill::quill
    PRIVATE unordered_dense
    PRIVATE range-v3::meta range-v3::concepts range-v3::range-v3
)



##########
# Options
##########
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/run/
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj "/diagnostics:caret")
endif()
if(MSVC AND CMAKE_BUILD_TYPE MATCHES Release)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
endif()



#add_compile_definitions(TRACY_ENABLE=0) 

#set_target_properties(${PROJECT_NAME} Tracy::TracyClient PROPERTIES 
#    TRACY_ENABLE OFF
#)



# steam stuff
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${STEAMWORKS_SDK_SHARED_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)



# working directory and resources
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if (DEFINED VH_USE_MODS)
    add_compile_definitions(VH_USE_MODS=$<BOOL:${VH_USE_MODS}>) 
endif()

if (DEFINED VH_DISCORD_INTEGRATION)
    add_compile_definitions(VH_DISCORD_INTEGRATION=$<BOOL:${VH_DISCORD_INTEGRATION}>) 
endif()

if (DEFINED VH_USE_ZLIB)
    add_compile_definitions(VH_USE_ZLIB=$<BOOL:${VH_USE_ZLIB}>) 
endif()

if (DEFINED VH_USE_PREFABS)
    add_compile_definitions(VH_USE_PREFABS=$<BOOL:${VH_USE_PREFABS}>) 
endif()

if (DEFINED VH_ZONE_GENERATION)
    add_compile_definitions(VH_ZONE_GENERATION=$<BOOL:${VH_ZONE_GENERATION}>) 
endif()
