cmake_minimum_required(VERSION 3.10)
project(netcode VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

# Find Raylib (installed via Homebrew)
set(CMAKE_PREFIX_PATH "/opt/homebrew/lib/cmake/raylib" ${CMAKE_PREFIX_PATH})
find_package(raylib 5.5 REQUIRED)

# Main library
add_library(netcode_lib
    src/client/client.cpp
    src/server/server.cpp
    src/visualization/game_window.cpp
    src/visualization/game_scene.cpp
    src/visualization/player.cpp
    src/visualization/entity_controller.cpp
    src/visualization/network_utility.cpp
    src/visualization/control_panel.cpp
)

target_include_directories(netcode_lib 
    PUBLIC include
    PUBLIC /opt/homebrew/include  # Add Homebrew include path
)
target_link_libraries(netcode_lib raylib)

# Main executable
add_executable(netcode src/main.cpp)
target_link_libraries(netcode netcode_lib)

# GUI Test executable
add_executable(gui_test tests/visualization/gui_test.cpp)
target_link_libraries(gui_test netcode_lib)
