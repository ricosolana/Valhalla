##########
# Project
##########
cmake_minimum_required (VERSION 3.14)
project (Valhalla)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(get_steamapi)
include(get_cpm)

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
endif()



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
#find_package(c4core CONFIG REQUIRED)
#find_package(ryml CONFIG REQUIRED)
#find_package(easyloggingpp CONFIG REQUIRED) # somehow doesnt work
#add_compile_definitions(SOL_NO_PRINT_ERRORS)
remove_definitions(SOL_PRINT_ERRORS)

add_compile_definitions(NOMINMAX)

#set_target_properties(ryml::ryml PROPERTIES RYML_STANDALONE ON)
#set_target_properties(${PROJECT_NAME} OptickCore PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
#)



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
    "src/ValhallaServer.cpp" 
    "src/HashUtils.cpp"
    "src/Main.cpp"
     
     
     
    "src/Player.cpp"
    "src/PlayerProfile.cpp"
    "src/Quaternion.cpp"
    "src/ResourceManager.cpp"
    "src/ModManager.cpp" 
    
    
    "src/Stream.cpp"
    "src/Task.cpp"
    
    "src/Utils.cpp"
    "src/NetSync.cpp"
    
    "src/NetSyncManager.cpp"
    
    "src/NetManager.cpp"
    "src/NetPeer.cpp" 
    "src/NetPackage.cpp"    
    "src/NetRouteManager.cpp"
    "src/NetRpc.cpp" 
    
    
    "src/ZoneSystem.cpp" 
    "src/Vector.cpp" 
    #"src/WorldGenerator.cpp" # temporarily broken
    "src/MathUtils.cpp"
    "src/NetObject.cpp" 

    # steam
     
    "src/NetSteamSocket.cpp" 
    "src/NetAcceptorSteam.cpp"
    "src/objects/Beehive.cpp"
 "include/NetScene.h" "src/NetScene.cpp" "include/NetSceneHashes.h")

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
    #PUBLIC c4core::c4core
    #PRIVATE ryml::ryml
    easyloggingpp
    OptickCore
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
