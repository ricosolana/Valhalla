# Project
cmake_minimum_required (VERSION 3.14)
project (MyProject)

# Libraries
find_package(Threads REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(GameNetworkingSockets CONFIG REQUIRED)

# Linking
add_executable(${PROJECT_NAME}
    "src/Main.cpp"
    "src/trivial_signaling_client.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
    
target_link_libraries(${PROJECT_NAME}
    PRIVATE Threads::Threads
    PRIVATE GameNetworkingSockets::static GameNetworkingSockets::GameNetworkingSockets
    PRIVATE protobuf::libprotobuf
)
