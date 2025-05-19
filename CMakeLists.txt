cmake_minimum_required(VERSION 3.10)
project(netcode VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 3 COMPONENTS Graphics REQUIRED)

# Main library
add_library(netcode_lib
    src/client/client.cpp
    src/server/server.cpp
    src/visualization/game_window.cpp
)

target_include_directories(netcode_lib PUBLIC include)
target_link_libraries(netcode_lib SFML::Graphics)

# Main executable
add_executable(netcode src/main.cpp)
target_link_libraries(netcode netcode_lib)

# Visualization demo
add_executable(viz_demo src/visualization/visualization_demo.cpp)
target_link_libraries(viz_demo netcode_lib)
