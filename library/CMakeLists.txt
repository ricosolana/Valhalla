set(AVLEDET_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# add shared
add_library(avledet_library STATIC)

target_include_directories(avledet_library
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${STEAMWORKS_SDK_SOURCE_DIR}
        ${GTL_INCLUDE_DIRS}
        ankerl::unordered_dense
    PRIVATE
        # ... ?
)

add_subdirectory("src")

if (NOT DEFINED AVL_ENABLE_SCRIPTING OR AVL_ENABLE_SCRIPTING)
    target_link_libraries(avledet_library
        PUBLIC 
            lua #sol2
    )
endif()

if (NOT DEFINED VH_DISCORD_INTEGRATION OR VH_DISCORD_INTEGRATION)
    target_link_libraries(avledet_library
        PUBLIC 
            OpenSSL::SSL 
            OpenSSL::Crypto
            dpp::dpp
    )
endif()

target_link_libraries(avledet_library
    PUBLIC 
        Threads::Threads
        ${STEAMWORKS_SDK_SHARED_DIR}
        ZLIB::ZLIB
        yaml-cpp::yaml-cpp
        magic_enum::magic_enum
        Tracy::TracyClient
        $<IF:$<TARGET_EXISTS:zstd::libzstd_shared>,zstd::libzstd_shared,zstd::libzstd_static>
        quill::quill
        unordered_dense::unordered_dense
        # TODO, are we really using v3? c++23 ranges are released...
        range-v3::meta range-v3::concepts range-v3::range-v3
)

#TODO add testing unit!
add_subdirectory("test")
