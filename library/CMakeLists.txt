set(AVLEDET_LIBRARY_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# add shared
add_library(avledet_library STATIC)

if(WIN32)
    target_compile_options(avledet_library PRIVATE /bigobj "/diagnostics:caret")

    # ignore annoying msvc warnings
    add_definitions("/wd4244") # float loss
    add_definitions("/wd4267") # int loss
    add_definitions("/wd4996") # c functions it seems unsafe
else()
    target_compile_options(avledet_library
        PRIVATE
            -Wall 
            -Wextra 
            #-Wconversion # many compile errors due to migration
            -Werror 
            -Wno-unknown-pragmas 
            -Wno-unused-variable 
            -Wno-deprecated-declarations #hmm change detected?
            -Wno-unused-value
            -Wno-unused-parameter
    )
endif()

target_include_directories(avledet_library
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# SYSTEM - ignore external warnings / errors
target_include_directories(avledet_library
    SYSTEM
        PUBLIC
            ${STEAMWORKS_SDK_SOURCE_DIR}
            ${GTL_INCLUDE_DIRS}
            ${LUA_INCLUDE_DIR}
            ankerl::unordered_dense
        PRIVATE
            # ... ?
)

add_subdirectory("src")

if (NOT DEFINED AVL_ENABLE_SCRIPTING OR AVL_ENABLE_SCRIPTING)
    target_link_libraries(avledet_library
        PUBLIC 
            #lua sol2
            ##lua::lua
            ###${LUA_LIBRARIES}
            ###sol2::sol2
            #lua sol2::sol2
            ###Lua::Lua sol2::sol2

            ${LUA_LIBRARY} sol2::sol2
    )
endif()

if (NOT DEFINED AVL_DISCORD_INTEGRATION OR AVL_DISCORD_INTEGRATION)
    target_link_libraries(avledet_library
        PUBLIC 
            OpenSSL::SSL 
            OpenSSL::Crypto
            dpp::dpp
    )
endif()

if(WIN32)
    target_link_libraries(avledet_library
        LINK_PRIVATE
            # TODO SYSTEM?
            iphlpapi # windows api, for my socket function get_ping()
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
        isptr::isptr
)

message("Lua libraries: ${LUA_LIBRARIES}")

add_custom_command(TARGET avledet_library POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        #$<TARGET_FILE:Lua::Lua>
        ###$<TARGET_FILE:Lua>
        #${LUA_LIBRARIES}  # points to liblua.so
        ##${LUA_LIB_PATH} ## Err: unrecognized command... weird
        #####$<TARGET_FILE:unofficial::lua::lua>
        #$<TARGET_FILE_DIR:avledet_library>
        ${LUA_LIBRARY}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)



#TODO add testing unit!
add_subdirectory("test")
