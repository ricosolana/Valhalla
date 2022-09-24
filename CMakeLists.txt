# Project
cmake_minimum_required (VERSION 3.14)
project (MyProject)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(get_steamapi)

# Libraries
find_package(Threads REQUIRED)

# Linking
add_executable(${PROJECT_NAME}
    "src/Main.cpp"    
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${STEAMAPI_SOURCE_DIR}
    #PUBLIC "C:/Users/Rico/Documents/Visual Studio 2022/Libraries/steam-sdk/public/steam"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
    
target_link_libraries(${PROJECT_NAME}
    PRIVATE Threads::Threads
    PUBLIC ${STEAMAPI_BINARY_DIR}
    #"C:/Users/Rico/Documents/Visual Studio 2022/Libraries/steam-sdk/redistributable_bin/win64/steam_api64.lib"
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${STEAMAPI_SHARED_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
