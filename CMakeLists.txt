# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

project(Valhalla)

if (ESP_PLATFORM)
	include($ENV{IDF_PATH}/tools/cmake/project.cmake)
else()
	message("Hopefully this is MSVC!")
	add_subdirectory("main")
endif()
