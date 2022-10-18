##########
# Project
##########
cmake_minimum_required (VERSION 3.14)
project (Valhalla)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
# for some fuckin reason you have to login to steam not to download sdk
# who cares
#include(get_steamapi)
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
#find_package(easyloggingpp CONFIG REQUIRED) # somehow doesnt work
#add_compile_definitions(SOL_NO_PRINT_ERRORS)
remove_definitions(SOL_PRINT_ERRORS)


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
#CPMAddPackage("gh:bombomby/optick@1.3.1.0#1.3.1.0")
CPMAddPackage("gh:bombomby/optick@1.4.0.0#1.4.0.0")



# ASIO

CPMAddPackage("gh:chriskohlhoff/asio#asio-1-21-0@1.21.0")

# ASIO doesn't use CMake, we have to configure it manually. Extra notes for using on Windows:
#
# 1) If _WIN32_WINNT is not set, ASIO assumes _WIN32_WINNT=0x0501, i.e. Windows XP target, which is
# definitely not the platform which most users target.
#
# 2) WIN32_LEAN_AND_MEAN is defined to make Winsock2 work.
add_library(asio INTERFACE)

target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)

target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)

target_link_libraries(asio INTERFACE Threads::Threads)

if(WIN32)
    # macro see @ https://stackoverflow.com/a/40217291/1746503
    macro(get_win32_winnt version)
        if(CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
            string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
            # Check for Windows 10, b/c we'll need to convert to hex 'A'.
            if("${verMajor}" MATCHES "10")
                set(verMajor "A")
                string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
            endif("${verMajor}" MATCHES "10")
            # Remove all remaining '.' characters.
            string(REPLACE "." "" ver ${ver})
            # Prepend each digit with a zero.
            string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif()
    endmacro()

    if(NOT DEFINED _WIN32_WINNT)
      get_win32_winnt(ver)
      set(_WIN32_WINNT ${ver})
    endif()

    message(STATUS "Set _WIN32_WINNT=${_WIN32_WINNT}")

    target_compile_definitions(asio INTERFACE _WIN32_WINNT=${_WIN32_WINNT} WIN32_LEAN_AND_MEAN)
endif()



##########
# Linking
##########
add_executable(${PROJECT_NAME}
    #"src/SteamManager.cpp"

    
    
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
    "src/NetRpcManager.cpp"
    "src/NetRpc.cpp" 
    "src/NetZSocket2.cpp"
    "src/ZAcceptor.cpp"
     "src/ZoneSystem.cpp" 
     "src/Vector.cpp" 
     #"src/WorldGenerator.cpp" # temporarily broken
     "src/MathUtils.cpp"
 "include/NetObject.h" "include/ICallable.h" "src/NetObject.cpp" "include/NetHashes.h" "include/NetID.h")

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    #PUBLIC ${STEAMAPI_SOURCE_DIR}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
    
target_link_libraries(${PROJECT_NAME}
    PUBLIC Threads::Threads
    #${STEAMAPI_BINARY_DIR}
    PUBLIC OpenSSL::SSL OpenSSL::Crypto
    robin_hood::robin_hood # not really needed, but this is c++ :>)
    ZLIB::ZLIB
    PUBLIC lua
    sol2
    easyloggingpp
    asio
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



#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt
#     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#file(COPY ${STEAMAPI_SHARED_DIR}
#    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
#)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
