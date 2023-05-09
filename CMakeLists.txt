##########
# Project
##########
cmake_minimum_required (VERSION 3.14)
project(SocketTest)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(get_steamapi)

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
endif()

############
# Libraries
############
find_package(Threads REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(dpp CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)

add_compile_definitions(
    NOMINMAX
    #WIN32_LEAN_AND_MEAN
)

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
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${STEAMAPI_SOURCE_DIR}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_link_libraries(${PROJECT_NAME}
    PUBLIC Threads::Threads
    ${STEAMAPI_BINARY_DIR}
    PRIVATE Tracy::TracyClient
    PRIVATE dpp::dpp
    PRIVATE quill::quill
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



set_target_properties(${PROJECT_NAME} Tracy::TracyClient PROPERTIES 
    TRACY_ENABLE ON
)



# steam stuff
file(COPY ${STEAMAPI_SHARED_DIR}
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
