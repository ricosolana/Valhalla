set(AVLEDET_UTIL_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(avledet_util_test)

# Public include headers
target_include_directories(avledet_util_test
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  #PRIVATE 
    #${CMAKE_CURRENT_SOURCE_DIR}/include/avledet/crypto_test
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(avledet_util_test
  LINK_PUBLIC
  LINK_PRIVATE
    avledet_library  
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME avledet_util COMMAND avledet_util_test)

#configure_file("key.pem" "${CMAKE_BINARY_DIR}/library/api/test/key.pem" COPYONLY)
