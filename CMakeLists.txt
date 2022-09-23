# Valhalla Global Project Space

cmake_minimum_required (VERSION 3.14)
project (Valhalla)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(get_cpm)
#add_subdirectory(external/openssl-cmake)

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
endif()

############
# LIBRARIES
############


#find_package(CryptoPP REQUIRED)
#CPMAddPackage(
#    NAME cryptopp
#    GITHUB_REPOSITORY noloader/cryptopp-cmake
#    #VERSION 3.11.3
#    GIT_TAG CRYPTOPP_8_6_0
#    DOWNLOAD_ONLY TRUE
#)
#add_library(cryptopp INTERFACE)
#target_include_directories(cryptopp INTERFACE ${cryptopp_SOURCE_DIR})

#set(OPENSSL_ROOT_DIR "C:/Users/Rico/Documents/VisualStudio2019/Libraries/OpenSSL-Win64")


set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
set(OPENSSL_INCLUDE_DIR "C:/Program Files/OpenSSL-Win64/include")
set(OPENSSL_LIBRARIES "C:/Program Files/OpenSSL-Win64/lib")
find_package(openssl)
include_directories(${OPENSSL_INCLUDE_DIRS})





#set(GameNetworkingSockets_DIR "C:/Users/Rico/Documents/Visual Studio 2022/Libraries/GameNetworkingSockets")
#set( _GameNetworkingSockets_HEADER_SEARCH_DIR "C:/Users/Rico/Documents/Visual Studio 2022/Libraries/GameNetworkingSockets/include/GameNetworkingSockets" )
#set( _GameNetworkingSockets_LIB_SEARCH_DIR "C:/Users/Rico/Documents/Visual Studio 2022/Libraries/GameNetworkingSockets/lib" )
#find_package(GameNetworkingSockets CONFIG REQUIRED)
#target_include_directories(Valhalla PRIVATE ${GameNetworkingSockets_INCLUDE_DIR})



#find_package(GameNetworkingSockets)



#CPMAddPackage(
#    NAME GameNetworkingSockets
#    GITHUB_REPOSITORY ValveSoftware/GameNetworkingSockets
#    #VERSION 3.11.3
#    GIT_TAG v1.4.1
#    DOWNLOAD_ONLY TRUE
#    OPTIONS
#    "USE_STEAMWEBRTC ON"
#    "Protobuf_USE_STATIC_LIBS ON"
#)
##add_library(lua STATIC ${Lua_sources})
##add_library(gamenetworkingsockets STATIC ${gamenetworkingsockets_sources})
#add_library(gamenetworkingsockets PUBLIC)
#target_include_directories(GameNetworkingSockets SYSTEM INTERFACE ${gamenetworkingsockets_SOURCE_DIR}/include)
#target_include_directories(gamenetworkingsockets PUBLIC ${gamenetworkingsockets_SOURCE_DIR}/include)
#target_include_directories(gamenetworkingsockets PUBLIC ${gamenetworkingsockets_BINARY_DIR})
#target_compile_definitions(gamenetworkingsockets PUBLIC STEAMNETWORKINGSOCKETS_STATIC_LINK)
#target_include_directories(gamenetworkingsockets SYSTEM INTERFACE ${gamenetworkingsockets_SOURCE_DIR}/include)
#target_include_directories(gamenetworkingsockets SYSTEM INTERFACE ${gamenetworkingsockets_BINARY_DIR}/include)

#target_include_directories(gamenetworkingsockets PRIVATE ${gamenetworkingsockets_INCLUDE_DIR})






#option(BUILD_OPENSSL ON)
#option(SYSTEM_OPENSSL ON)
#target_include_directories(ssl BEFORE INTERFACE $<BUILD_INTERFACE:${OPENSSL_PREFIX}/usr/local/include> )
#target_include_directories(crypto BEFORE INTERFACE $<BUILD_INTERFACE:${OPENSSL_PREFIX}/usr/local/include> )


# ASIO
find_package(Threads REQUIRED)

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



CPMAddPackage(
    NAME robinhood
    GITHUB_REPOSITORY martinus/robin-hood-hashing
    #VERSION 3.11.3
    GIT_TAG 3.11.5
    DOWNLOAD_ONLY TRUE
)
add_library(robinhood INTERFACE)
target_include_directories(robinhood SYSTEM INTERFACE ${robinhood_SOURCE_DIR}/src/include)



#CPMAddPackage("gh:bombomby/optick@1.3.1.0#1.3.1.0")



CPMAddPackage(
    NAME json
    GITHUB_REPOSITORY nlohmann/json
    VERSION 3.9.1
    OPTIONS
    "JSON_BuildTests OFF"
)



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



CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG ea2f937a01ce39795ab02b6c6e30173b4f1ed46c
    DOWNLOAD_ONLY TRUE
)
if(stb_ADDED)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
endif()



CPMAddPackage(
    NAME zlib
    GITHUB_REPOSITORY madler/zlib
    VERSION 1.2.11
)
target_include_directories(zlibstatic PUBLIC ${zlib_SOURCE_DIR})
target_include_directories(zlibstatic PUBLIC ${zlib_BINARY_DIR})



CPMAddPackage(
    NAME Lua
    GITHUB_REPOSITORY lua/lua
    VERSION 5.4.3
    DOWNLOAD_ONLY YES
)
if(Lua_ADDED)
    file(GLOB Lua_sources ${Lua_SOURCE_DIR}/*.c)
    list(REMOVE_ITEM Lua_sources "${Lua_SOURCE_DIR}/lua.c" "${Lua_SOURCE_DIR}/luac.c" "${Lua_SOURCE_DIR}/onelua.c")
    add_library(lua STATIC ${Lua_sources})
    target_include_directories(lua SYSTEM PUBLIC $<BUILD_INTERFACE:${Lua_SOURCE_DIR}>)
    install(TARGETS lua
		EXPORT lua-targets
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
    install(EXPORT lua-targets DESTINATION ${LIB_INSTALL_DIR}/lua_ac/cmake)
    export(EXPORT lua-targets)
endif()



CPMAddPackage(
    NAME sol2
    GITHUB_REPOSITORY ThePhD/sol2
    VERSION 3.2.2
    DOWNLOAD_ONLY YES
)
if(sol2_ADDED)
    add_library(sol2 INTERFACE IMPORTED)
    target_include_directories(sol2 SYSTEM INTERFACE ${sol2_SOURCE_DIR}/include)
    target_link_libraries(sol2 INTERFACE lua)
endif()



#
# Linking
#

add_executable(Valhalla
    "src/BinaryReader.cpp"
    "src/BinaryWriter.cpp"
    "src/Game.cpp" 
    "src/HashUtils.cpp"
    "src/Main.cpp"
     
     
     
    "src/Player.cpp"
    "src/PlayerProfile.cpp"
    "src/Quaternion.cpp"
    "src/ResourceManager.cpp"
    "src/ScriptManager.cpp" 
    
    
    "src/Stream.cpp"
    "src/Task.cpp"
    
    "src/Utils.cpp"
    "src/ZDO.cpp"
    "src/ZDOID.cpp"
    "src/ZDOMan.cpp"
    "src/ZDOPeer.cpp"
    "src/ZNet.cpp"
    "src/ZNetPeer.cpp" 
    "src/ZPackage.cpp"    
    "src/ZRoutedRpc.cpp"
    "src/ZRpc.cpp" 
    "src/ZSocket2.cpp"
)



target_include_directories(Valhalla PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(Valhalla PRIVATE cxx_std_20)

target_link_libraries(Valhalla
    #cryptopp-static
    #gamenetworkingsockets
    #GameNetworkingSockets::shared
    #GameNetworkingSockets::GameNetworkingSockets_s
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
    asio
    robinhood
    stb
    easyloggingpp
    nlohmann_json::nlohmann_json
    #OptickCore
    zlibstatic
    sol2
)


#include_directories(${CMAKE_SOURCE_DIR}/src)

#set_target_properties(Valhalla OptickCore PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
#)

set_target_properties(Valhalla PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/run/
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)



#set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Valhalla)

if (MSVC)
    target_compile_options(Valhalla PRIVATE /bigobj "/diagnostics:caret")
endif()
if(MSVC AND CMAKE_BUILD_TYPE MATCHES Release)
    set_target_properties(Valhalla PROPERTIES WIN32_EXECUTABLE ON)
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
