# Project
cmake_minimum_required (VERSION 3.14)
project (MyProject)

# Libraries
find_package(protobuf CONFIG REQUIRED)
find_package(GameNetworkingSockets CONFIG REQUIRED)

# Linking
add_executable(${PROJECT_NAME}
    "src/Main.cpp"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
    
target_link_libraries(${PROJECT_NAME}
    PRIVATE GameNetworkingSockets::static GameNetworkingSockets::GameNetworkingSockets
    PRIVATE protobuf::libprotobuf
)
