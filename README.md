# Netcode Library

## Introduction

Netcode Library is a comprehensive C++ library for implementing client-server network communication in games and real-time applications. This project was developed as part of a voluntary project in a network programming course (IDATT2104), focusing on creating netcode for multiplayer games with advanced prediction algorithms and visual demonstration. The implementation emphasizes educational value and practical understanding of network programming concepts with real-time 3D visualization.

## Implemented Functionality

### Core Networking
- Client-server architecture with UDP sockets
- Real-time network communication
- Thread-safe implementation
- Packet handling and processing
- Network delay simulation
- Cross-platform compatibility (macOS, Linux, Windows)

### Game Networking Features
- **Client-side prediction**: Immediate response to user input
- **Server reconciliation**: Authoritative server state correction
- **Entity interpolation**: Smooth visual transitions for remote entities
- **Snapshot management**: State history for rollback and prediction
- **Lag compensation**: Handling of network delays and packet loss
- **Multi-client support**: Multiple clients connecting to a single server

### 3D Visualization and Demo
- Real-time 3D visualization using Raylib
- Multiple view modes (client, server, and interpolated views)
- Interactive control panel for adjusting network settings
- Visual demonstration of prediction algorithms
- Camera controls for exploring the 3D scene
- Network message logging and display

### Configurable Settings
- Adjustable client-to-server and server-to-client delays
- Toggle prediction and interpolation on/off
- Real-time settings modification during runtime
- Test mode vs. real networking mode

## Current Architecture

The project consists of several key components:

### Network Components
- **Client**: Handles client-side networking, prediction, and server communication
- **Server**: Manages multiple clients, authoritative game state, and broadcasting updates
- **NetworkedEntity**: Interface for objects that can be synchronized across the network
- **Packet System**: Structured packet handling for reliable communication

### Prediction Systems
- **Snapshot Manager**: Manages historical state snapshots for rollback
- **Prediction System**: Handles client-side prediction of movements
- **Reconciliation System**: Corrects client state based on authoritative server updates
- **Interpolation System**: Smooths visual transitions for remote entities

### Visualization
- **GameWindow**: Main window management and event handling
- **GameScene**: 3D scene rendering with camera controls
- **Player**: 3D player entity with physics and visual representation
- **NetworkUtility**: Bridges networking and visualization components
- **ControlPanel**: GUI controls for adjusting network settings in real-time

## External Dependencies

- **C++20 Standard Library**: Core data structures and algorithms
- **CMake (3.10+)**: Cross-platform build system
- **Raylib (5.5)**: 3D graphics library for visualization and input handling
  - Cross-platform support (Windows, Linux, macOS, and more)
  - Hardware accelerated with OpenGL
  - No external dependencies (self-contained)
- **GoogleTest (1.17.0)**: Testing framework for unit tests (automatically fetched)
- **POSIX Sockets**: Network communication (cross-platform socket implementation)

## Installation Instructions

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.10 or higher
- Raylib 5.5 (install via package manager)
- Git

### Windows Installation

#### Option 1: Using Pre-built Binaries (Easiest)
```bash
# Download raylib from GitHub releases
# Visit: https://github.com/raysan5/raylib/releases
# Download raylib-5.5_win64_msvc16.zip (or appropriate version)
# Extract to a folder like C:\raylib

# Clone and build the project
git clone https://github.com/fant-orangen/idatt2104.git
mkdir build && cd build
cmake .. -DRAYLIB_ROOT=C:\raylib
cmake --build . --config Release
```

#### Option 2: Using vcpkg Package Manager
```bash
# Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install raylib
.\vcpkg install raylib

# Clone and build the project
git clone https://github.com/fant-orangen/idatt2104.git
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

#### Option 3: Using Visual Studio
```bash
# Install raylib via vcpkg as shown above, then:
# 1. Open Visual Studio 2019/2022
# 2. Create a new C++ project or open existing CMakeLists.txt
# 3. Ensure vcpkg integration: .\vcpkg integrate install
# 4. Build the project using Visual Studio

# Or use the CMake Visual Studio generator:
cmake .. -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
# Then open the generated .sln file in Visual Studio
```

#### Option 4: Build raylib from Source
```bash
# Clone raylib
git clone https://github.com/raysan5/raylib.git
cd raylib

# Build raylib
mkdir build && cd build
cmake .. -DBUILD_SHARED_LIBS=ON
cmake --build . --config Release

# Install raylib (optional, requires admin privileges)
cmake --install . --config Release

# Then build the netcode project
git clone https://github.com/fant-orangen/idatt2104.git
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### macOS Installation (using Homebrew)
```bash
# Install dependencies
brew install cmake raylib

# Clone the repository
git clone https://github.com/fant-orangen/idatt2104.git

# Setup build system
mkdir build && cd build
cmake ..

# Compilation
make -j$(sysctl -n hw.ncpu)
```

### Linux Installation
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install cmake build-essential libraylib-dev

# Or for Fedora/CentOS
sudo dnf install cmake gcc-c++ raylib-devel

# Clone and build
git clone https://github.com/fant-orangen/idatt2104.git
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Alternative Build Methods
If you encounter issues with package managers, you can also build raylib from source:

```bash
# Build raylib from source (if package manager version doesn't work)
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED
sudo make install PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED
```

### Troubleshooting
- **Windows**: 
  - If CMake can't find raylib, try setting `RAYLIB_ROOT` environment variable
  - For Visual Studio, ensure vcpkg integration is installed: `.\vcpkg integrate install`
  - Download pre-built binaries from [raylib releases](https://github.com/raysan5/raylib/releases) if package managers fail
- **macOS**: If CMake can't find raylib, ensure Homebrew installed it correctly: `brew list raylib`
- **Linux**: If `libraylib-dev` is not found, try `raylib-dev` or install from source
- **All platforms**: Ensure you have C++20 support (GCC 10+, Clang 10+, or MSVC 2019+)

## Usage Instructions

### Running the Visual Demo
```bash
# From the build directory
./gui_full          # Full networking demo with 3D visualization
```

### Development Workflow
```bash
# Auto-rebuild and run on file changes (requires fswatch)
./watch_and_run.sh
```

### Running Tests
```bash
# From the build directory
./netcode_tests     # Run all unit tests
```

### Basic Usage Example

#### Server Setup
```cpp
#include "netcode/server/server.hpp"

int main() {
    // Create a server on port 7000
    netcode::Server server(7000);
    
    // Start the server
    server.start();
    
    return 0;
}
```

#### Client Setup
```cpp
#include "netcode/client/client.hpp"

int main() {
    // Create a client with ID 1 on port 7001
    netcode::Client client(1, 7001, "127.0.0.1", 7000);
    
    // Start the client
    client.start();
    
    // Send movement input
    netcode::math::MyVec3 movement{1.0f, 0.0f, 0.0f};
    client.sendMovementRequest(movement, false);
    
    return 0;
}
```

#### Visualization Setup
```cpp
#include "netcode/visualization/game_window.hpp"
#include "netcode/visualization/network_utility.hpp"

int main() {
    // Create window in standard networking mode
    netcode::visualization::GameWindow window(
        "Netcode Demo", 800, 600, 
        NetworkUtility::Mode::STANDARD
    );
    window.run();
    return 0;
}
```

## Features Demonstration

The visual demo showcases:

1. **Three synchronized views**: Client 1, Server (authoritative), and Client 2
2. **Real-time networking**: Actual UDP packet transmission between processes
3. **Prediction visualization**: See how client-side prediction works
4. **Interpolation effects**: Smooth movement of remote entities
5. **Network delay simulation**: Adjustable delays to simulate various network conditions
6. **Interactive controls**: Real-time adjustment of network parameters

### Controls
- **WASD**: Move player
- **Space**: Jump
- **Mouse**: Camera control (right-click and drag)
- **Control Panel**: Adjust network delays and toggle prediction/interpolation

## Project Structure
```
├── include/netcode/           # Public API headers
│   ├── client/               # Client networking
│   ├── server/               # Server networking  
│   ├── prediction/           # Prediction algorithms
│   ├── visualization/        # 3D visualization
│   ├── packets/             # Network packet definitions
│   ├── utils/               # Utility classes
│   └── math/                # Mathematical utilities
├── src/netcode/             # Implementation files
├── tests/                   # Unit tests and visual tests
├── assets/                  # 3D models and textures
└── build/                   # Build output directory
```

## Testing

### Unit Tests
```bash
./netcode_tests              # Comprehensive test suite
```

### Visual Tests
```bash
./gui_full                   # Full networking demonstration
```

## Performance Considerations
- Optimized for low-latency communication (sub-50ms typical)
- Efficient UDP-based protocol
- Thread-safe implementation with minimal blocking
- Memory-efficient snapshot management
- Configurable buffer sizes and network parameters
- Real-time 3D rendering at 60+ FPS

## Educational Value

This project serves as a comprehensive educational resource for:
- Understanding client-server networking fundamentals
- Learning client-side prediction and lag compensation techniques
- Exploring UDP socket programming in C++
- 3D game programming with physics simulation
- Network protocol design and implementation
- Real-time systems programming

## Future Work / Current Limitations

### Potential Enhancements
- Advanced anti-cheat mechanisms
- Network data compression
- Automatic reconnection handling
- Support for more complex game states
- WebRTC integration for browser compatibility
- Mobile platform support

### Known Limitations
- Basic physics simulation (suitable for demonstration)
- Limited to demonstration-scale player counts
- No built-in authentication system
- Requires manual network configuration

#### Bugs
- Upon turning interpolation on, off and then on again, there is a small, additional delay

## Contributing
Contributions are welcome! Please ensure your code follows the existing style and includes appropriate tests.

## Authors
- Josefine Arntsen
- Håvard Versto Daleng
- Magnus Eik  

## Acknowledgments
- NTNU's Network Programming course (IDATT2104) for providing the opportunity to develop this project
- Raylib community for the excellent 3D graphics library
- Gabriel Gambetta's "Fast-Paced Multiplayer" articles for netcode concepts