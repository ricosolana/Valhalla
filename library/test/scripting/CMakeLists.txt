set(AVLEDET_SCRIPTING_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(avledet_scripting_test)

# Public include headers
target_include_directories(avledet_scripting_test
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  #PRIVATE 
    #${CMAKE_CURRENT_SOURCE_DIR}/include/avledet/crypto_test
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(avledet_scripting_test
  LINK_PUBLIC
  LINK_PRIVATE
    avledet_library  
    lua
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME avledet_scripting COMMAND avledet_scripting_test)

# hmmm, change this?
configure_file("server.yml" "${CMAKE_BINARY_DIR}/test/scripting/server.yml" COPYONLY)
