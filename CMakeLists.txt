cmake_minimum_required(VERSION 3.10)
project(netcode VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

# Main library
add_library(netcode_lib
    src/client/client.cpp
    src/server/server.cpp
    src/common/network.cpp
    src/common/game_state.cpp
)

target_include_directories(netcode_lib PUBLIC include)

# Main executable
add_executable(netcode src/main.cpp)
target_link_libraries(netcode netcode_lib)

# Tests
enable_testing()
add_subdirectory(tests)

# Examples
add_subdirectory(examples)
