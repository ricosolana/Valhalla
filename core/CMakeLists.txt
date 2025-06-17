set(AVLEDET_SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(avledet_server)

target_include_directories(avledet_server
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        # nothing here \ /
)

add_subdirectory("src")

target_link_libraries(avledet_server
    LINK_PRIVATE
        avledet_library
)
