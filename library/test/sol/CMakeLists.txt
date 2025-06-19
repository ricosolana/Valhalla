set(AVLEDET_SOL_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(avledet_sol_test)

# Public include headers
target_include_directories(avledet_sol_test
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  #PRIVATE 
    #${CMAKE_CURRENT_SOURCE_DIR}/include/avledet/crypto_test
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(avledet_sol_test
  LINK_PUBLIC
  LINK_PRIVATE
    #avledet_library  
    lua
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME avledet_sol COMMAND avledet_sol_test)

#configure_file("key.pem" "${CMAKE_BINARY_DIR}/library/api/test/key.pem" COPYONLY)
