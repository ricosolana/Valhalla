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
project(MinimalPerlin)

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release)
endif()

# remove annoying min/max getting in the way of other min/max functions
add_compile_definitions(NOMINMAX)



##########
# Linking
##########
add_executable(${PROJECT_NAME} "src/Main.cpp")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)



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



# working directory and resources
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
