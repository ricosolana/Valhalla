set(AVLEDET_STREAM_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(avledet_stream_test)

# Public include headers
target_include_directories(avledet_stream_test
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  #PRIVATE 
    #${CMAKE_CURRENT_SOURCE_DIR}/include/avledet/stream_test
)

# Source subdirectory
add_subdirectory("src")

target_link_libraries(avledet_stream_test
  LINK_PUBLIC
  LINK_PRIVATE
    avledet_library  
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME avledet_stream COMMAND avledet_stream_test)

#configure_file("key.pem" "${CMAKE_BINARY_DIR}/library/api/test/key.pem" COPYONLY)
