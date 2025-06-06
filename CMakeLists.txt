cmake_minimum_required(VERSION 3.10)
project(netcode VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

# Find Raylib (installed via Homebrew)
set(CMAKE_PREFIX_PATH "/opt/homebrew/lib/cmake/raylib" ${CMAKE_PREFIX_PATH})
find_package(raylib 5.5 REQUIRED)

# Main library
add_library(netcode_lib
        src/netcode/client/client.cpp
        src/netcode/server/server.cpp
        src/netcode/utils/logger.cpp
        src/netcode/utils/visualization_logger.cpp
        src/netcode/visualization/game_window.cpp
        src/netcode/visualization/game_scene.cpp
        src/netcode/visualization/player.cpp
        src/netcode/visualization/network_utility.cpp
        src/netcode/visualization/control_panel.cpp
        src/netcode/visualization/concrete_settings.cpp
    # Add new netcode files
        src/netcode/prediction/snapshot.cpp
        src/netcode/prediction/prediction.cpp
        src/netcode/prediction/reconciliation.cpp
        src/netcode/prediction/interpolation.cpp
)

target_include_directories(netcode_lib 
    PUBLIC include
    PUBLIC /opt/homebrew/include
)
target_link_libraries(netcode_lib raylib)

# Main executable
#add_executable(netcode src/main.cpp)
#target_link_libraries(netcode netcode_lib)

# GUI Test executable
add_executable(gui_full tests/visualization/gui_full.cpp
        include/netcode/math/my_vec3.hpp)
target_link_libraries(gui_full netcode_lib)

# GUI Full executable
#add_executable(gui_full tests/visualization/gui_full.cpp)
#target_link_libraries(gui_full netcode_lib)

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs/doxygen) # Or your preferred path
    set(DOXYGEN_INPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(DOXYGEN_RECURSIVE YES)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Doxyfile")
        add_custom_target(docs_doxygen
                COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM
        )
    else()
        message(WARNING "Doxyfile not found in project root. Cannot create doxygen target.")
    endif()

else()
    message(WARNING "Doxygen not found. Documentation target will not be available.")
endif()

# Google Test
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG       v1.17.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable CTest
enable_testing()

# Basic tests
add_executable(netcode_tests
        tests/test_client.cpp
        tests/test_server.cpp
)

target_link_libraries(netcode_tests PRIVATE netcode_lib gtest_main)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(netcode_tests)