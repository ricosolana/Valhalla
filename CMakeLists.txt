# CLion setup:
# - use msvc compiler 2022
# - specify the vcpkg toolchain file location
# - specify arm64 architecture

# Winsock multiple includes errors:
# - Locate Windows.h and remove the define section containing WIN32_LEAN_AND_MEAN
# - Add an '#include <Winsock2.h>' replacing the above section
# - Some stack overflow 'enthusiasts' dislike this method, however its the only one which worked for me, so yea..
#   I will be removing asio anyways because I consider RCON to be semi-insecure and too extra/finicky, so the forbidden technique above isnt needed!

# setting command line program arguments (https://unrealistic.dev/posts/setting-debug-parameters-with-cmake-tools-for-visual-studio)
#   right click the cmakelists.txt -> 'add debug configuration''
#   add a new key 'args' followed by args

# luasocket install tutorial
#   just use vcpkg to build, then:
#   copy the folders from luasocket/bin/ to lua root dir
#   copy the folders from luasocket/lua/ to lua root dir

##########
# Project
##########
cmake_minimum_required (VERSION 3.14)
project(Valhalla)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(get_steamapi)
include(get_cpm)

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
endif()


#add_compile_definitions(WIN32_LEAN_AND_MEAN)
#add_compile_definitions(VC_EXTRALEAN)

############
# Libraries
############
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
#find_package(robin_hood CONFIG REQUIRED)
find_package(ZLIB REQUIRED) # able to remove; nothing *has* to be compressed, could include as a side-package?
find_package(lua CONFIG REQUIRED)
#find_package(sol2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED) # might be able to remove
#find_package(tomlplusplus CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)    
    
find_package(zstd CONFIG REQUIRED) # also could remove; zstd used primarily only for lua

find_package(dpp CONFIG REQUIRED)

#add_compile_definitions(SOL_PRINT_ERRORS)

#remove_definitions(SOL_PRINT_ERRORS)

#target_compile_definitions(asio INTERFACE WIN32_LEAN_AND_MEAN)

# remove annoying min/max getting in the way of other min/max functions
add_compile_definitions(NOMINMAX)

# ignore annoying msvc warnings
if(WIN32)
    add_definitions("/wd4244") # float loss
    add_definitions("/wd4267") # int loss
    add_definitions("/wd4996") # c functions it seems unsafe
endif()

#IF(NOT(CMAKE_BUILD_TYPE MATCHES DEBUG))
#ENDIF()

CPMAddPackage(
    NAME easyloggingpp
    GITHUB_REPOSITORY amrayn/easyloggingpp
    VERSION 9.97.0
    OPTIONS "build_static_lib ON"
)
target_include_directories(easyloggingpp PUBLIC ${easyloggingpp_SOURCE_DIR}/src)
target_compile_definitions(easyloggingpp PUBLIC 
    ELPP_THREAD_SAFE 
    ELPP_FEATURE_PERFORMANCE_TRACKING
    ELPP_DISABLE_DEBUG_LOGS
    ELPP_NO_DEFAULT_LOG_FILE
    ELPP_NO_LOG_TO_FILE
    ELPP_FRESH_LOG_FILE
)
if (LINUX)
    target_compile_definitions(easyloggingpp PUBLIC 
        ELPP_FEATURE_CRASH_LOG 
        ELPP_STL_LOGGING
    )
endif ()

# optick profiler
#   seems abandoned since last year
#   and client is buggy and crashes frequently
#CPMAddPackage("gh:bombomby/optick@1.4.0.0#1.4.0.0")

#target_compile_definitions(${PROJECT_NAME} INTERFACE WIN32_LEAN_AND_MEAN)
#target_compile_definitions(WIN32 INTERFACE WIN32_LEAN_AND_MEAN)

#add_definitions(WIN32_LEAN_AND_MEAN)

##########
# Linking
##########
add_executable(${PROJECT_NAME}
    "src/Main.cpp"

    "src/DataReader.cpp"
    
    "src/DungeonGenerator.cpp"
    "src/DungeonManager.cpp"
    "src/RandomEventManager.cpp"
    "src/GeoManager.cpp"
    "src/Heightmap.cpp"
    "src/HeightmapBuilder.cpp"
    "src/HeightmapManager.cpp"
    
    "src/ModManager.cpp"
    "src/NetAcceptorSteamDedicated.cpp"
    "src/NetManager.cpp"    
    "src/NetSocketSteam.cpp"
    "src/Peer.cpp"
    "src/PrefabManager.cpp"
    "src/Quaternion.cpp"
    "src/NetSocketPlayback.cpp"
    "src/DungeonRoom.cpp"
    "src/RouteManager.cpp"
    "src/Task.cpp"
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
    "src/NetAcceptorSteamP2P.cpp"
    
    "src/DiscordManager.cpp" 
    "src/DataWriter.cpp"
)

# change this to wherever your sol is
set(SOL2_ROOT_DIR "C:/Users/rico/Documents/Visual Studio 2022/Libraries/sol2")

# change to wherever your ankerl is
set(ANKERL_UNORDERED_DENSE_ROOT_DIR "C:/Users/rico/Documents/Visual Studio 2022/Libraries/unordered_dense")

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${STEAMAPI_SOURCE_DIR}
    ${SOL2_ROOT_DIR}/include
    ${ANKERL_UNORDERED_DENSE_ROOT_DIR}/include
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_link_libraries(${PROJECT_NAME}
    PUBLIC Threads::Threads
    ${STEAMAPI_BINARY_DIR}
    PUBLIC OpenSSL::SSL OpenSSL::Crypto
    #robin_hood::robin_hood
    ZLIB::ZLIB
    PUBLIC lua
    #sol2
    yaml-cpp
    #PRIVATE tomlplusplus::tomlplusplus
    easyloggingpp
    PRIVATE Tracy::TracyClient
    #OptickCore
    PRIVATE $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
    PRIVATE dpp::dpp
)

#target_compile_definitions(${PROJECT_NAME} INTERFACE WIN32_LEAN_AND_MEAN)


##########
# Options
##########
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/run/
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
    #VS_DEBUGGER_COMMAND_ARGUMENTS "-v"
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj "/diagnostics:caret")
endif()
if(MSVC AND CMAKE_BUILD_TYPE MATCHES Release)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
endif()



#set_target_properties(${PROJECT_NAME} OptickCore PROPERTIES
    #RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
#)


set_target_properties(${PROJECT_NAME} Tracy::TracyClient PROPERTIES 
    TRACY_ENABLE ON
)
#add_definitions(TRACY_ENABLE)



# steam stuff
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${STEAMAPI_SHARED_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

# working directory and resources
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
