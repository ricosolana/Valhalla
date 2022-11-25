# CLion setup:
# - use msvc compiler 2022
# - specify the vcpkg toolchain file location
# - specify arm64 architecture

# Winsock multiple includes errors:
# - Locate Windows.h and remove the define section containing WIN32_LEAN_AND_MEAN
# - Add an '#include <Winsock2.h>' replacing the above section

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
find_package(robin_hood CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
#find_package(libssh CONFIG REQUIRED) # server is too complicated
find_package(asio CONFIG REQUIRED)
#find_package(c4core CONFIG REQUIRED)
#find_package(ryml CONFIG REQUIRED)
#find_package(easyloggingpp CONFIG REQUIRED) # somehow doesnt work
#add_compile_definitions(SOL_NO_PRINT_ERRORS)
remove_definitions(SOL_PRINT_ERRORS)

#target_compile_definitions(asio INTERFACE WIN32_LEAN_AND_MEAN)
#target_compile_definitions(ssh INTERFACE WIN32_LEAN_AND_MEAN)

# remove annoying min/max getting in the way of other min/max functions
add_compile_definitions(NOMINMAX)

#set_target_properties(ryml::ryml PROPERTIES RYML_STANDALONE ON)
#set_target_properties(${PROJECT_NAME} OptickCore PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
#)

# ignore annoying msvc warnings
if(WIN32)
    add_definitions("/wd4244") # float loss
    add_definitions("/wd4267") # int loss
    add_definitions("/wd4996") # c functions it seems unsafe
endif()

#IF(NOT(CMAKE_BUILD_TYPE MATCHES DEBUG))
    add_compile_definitions(ELPP_DISABLE_DEBUG_LOGS)
#ENDIF()
CPMAddPackage(
    NAME easyloggingpp
    GITHUB_REPOSITORY amrayn/easyloggingpp
    VERSION 9.97.0
    OPTIONS "build_static_lib ON"
)
target_include_directories(easyloggingpp PUBLIC ${easyloggingpp_SOURCE_DIR}/src)
target_compile_definitions(easyloggingpp PUBLIC ELPP_THREAD_SAFE ELPP_FEATURE_PERFORMANCE_TRACKING)
if (LINUX)
    target_compile_definitions(easyloggingpp PUBLIC ELPP_FEATURE_CRASH_LOG ELPP_STL_LOGGING)
endif ()



# optick profiler
CPMAddPackage("gh:bombomby/optick@1.4.0.0#1.4.0.0")



##########
# Linking
##########
add_executable(${PROJECT_NAME}
    "src/Main.cpp"
    "src/ValhallaServer.cpp"
    "src/HashUtils.cpp"

     
     
    "src/Player.cpp"
    "src/PlayerProfile.cpp"
    "src/Quaternion.cpp"
    "src/VUtilsResource.cpp"
    "src/ModManager.cpp" 
    
    
    "src/Stream.cpp"
    "src/Task.cpp"
    
    "src/VUtils.cpp"
    "src/NetSync.cpp"
    
    "src/NetSyncManager.cpp"
    
    "src/NetManager.cpp"
    "src/NetPeer.cpp" 
    "src/NetPackage.cpp"    
    "src/NetRouteManager.cpp"
    "src/NetRpc.cpp" 
    
    
    "src/ZoneSystem.cpp" 
    "src/Vector.cpp" 
    "src/WorldGenerator.cpp" # temporarily broken
    "src/VUtilsMath.cpp"
    "src/NetObject.cpp" 

    "src/NetSteamSocket.cpp" 
    "src/NetAcceptorSteam.cpp"
    "src/objects/Beehive.cpp"
    #"src/NetScene.cpp" # temporarily broken
    "src/ChatManager.cpp"
    "src/RCONSocket.cpp"
    "src/RCONAcceptor.cpp"
    "src/VUtilsString.cpp"
    "src/GameObject.cpp"
    "src/GameObjectManager.cpp"
    "src/VUtilsRandom.cpp"
    "src/Game.cpp"
 
 "src/WorldManager.cpp")

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${STEAMAPI_SOURCE_DIR}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
    
target_link_libraries(${PROJECT_NAME}
    PUBLIC Threads::Threads
    ${STEAMAPI_BINARY_DIR}
    PUBLIC OpenSSL::SSL OpenSSL::Crypto
    robin_hood::robin_hood # not really needed, but this is c++ :>)
    ZLIB::ZLIB
    PUBLIC lua
    sol2
    yaml-cpp
    #ssh
    #PUBLIC c4core::c4core
    #PRIVATE ryml::ryml
    easyloggingpp
    OptickCore
    asio asio::asio
)

target_compile_definitions(${PROJECT_NAME} INTERFACE WIN32_LEAN_AND_MEAN)


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



set_target_properties(${PROJECT_NAME} OptickCore PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)


# steam stuff
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${STEAMAPI_SHARED_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

# working directory and resources
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
