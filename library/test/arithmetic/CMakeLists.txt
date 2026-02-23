set(AVLEDET_ARITHMETIC_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(avledet_arithmetic_test)

# Public include headers
target_include_directories(avledet_arithmetic_test
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  #PRIVATE 
    #${CMAKE_CURRENT_SOURCE_DIR}/include/avledet/crypto_test
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(avledet_arithmetic_test
  LINK_PUBLIC
  LINK_PRIVATE
    avledet_library  
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME avledet_arithmetic COMMAND avledet_arithmetic_test)

#configure_file("key.pem" "${CMAKE_BINARY_DIR}/library/api/test/key.pem" COPYONLY)
