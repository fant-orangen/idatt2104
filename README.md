# Netcode Library

## Introduction

Netcode Library is a lightweight C++ library for implementing client-server network communication in games and real-time applications. This project was developed as part of a voluntary project in a network programming course, focusing on creating netcode for multiplayer games with prediction algorithms. The implementation emphasizes educational value and practical understanding of network programming concepts.

## Implemented Functionality

- Client-server architecture
- Game state synchronization
- Client-side prediction
- Entity interpolation
- Lag compensation
- Reliable UDP protocol implementation
- Packet loss handling
- Low-latency communication
- Thread-safe implementation
- Cross-platform compatibility (Windows, macOS, Linux)

## Future Work / Current Limitations and Weaknesses

### Current Limitations
- Full peer-to-peer support not implemented
- Automatic connection and reconnection handling
- Network data compression
- Advanced anti-cheat mechanisms

### Known Weaknesses
- Network bandwidth can become a bottleneck with many players
- Prediction algorithms may cause visible jumps during large deviations
- High-latency player handling needs improvement
- Performance optimization under heavy load

## External Dependencies

- **C++20 Standard Library**: Core data structures and algorithms
- **CMake (3.10+)**: Cross-platform build system
- **SFML (2.5.1)**: Simple and Fast Multimedia Library, used for graphical demonstration of the prediction algorithm
- **Catch2 (3.0.1)**: Testing framework for unit tests
- **spdlog (1.9.2)**: High-performance logging library for debugging

## Installation Instructions

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.10 or higher
- Git

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/netcode-library.git
cd netcode-library

# Setup build system
mkdir build && cd build
cmake ..

# Compilation
make -j$(nproc)  # On Unix-like systems
# or
cmake --build .  # Platform independent
```

### Installation (Optional)
```bash
# Install the library system-wide (requires admin privileges)
sudo make install
```

## Usage Instructions

### Running the Demo
```bash
# From the build directory
./demo/netcode_demo
```

### Server Example
```cpp
#include <netcode/server.h>

int main() {
    // Create a server on port 8080
    netcode::Server server(8080);
    
    // Register callback for new clients
    server.onClientConnect([](netcode::ClientID client) {
        std::cout << "Client " << client << " connected.\n";
    });
    
    // Start the server (blocking call)
    server.start();
    
    return 0;
}
```

### Client Example
```cpp
#include <netcode/client.h>

int main() {
    // Create a client instance
    netcode::Client client;
    
    // Connect to the server
    if (client.connect("localhost", 8080)) {
        std::cout << "Connected to server.\n";
        
        // Main loop
        while (client.isConnected()) {
            client.update();
            // Game logic
        }
    } else {
        std::cerr << "Could not connect to server.\n";
    }
    
    return 0;
}
```

## Testing

### Running Unit Tests
```bash
# From the build directory
./tests/netcode_tests
```

### Running Performance Tests
```bash
./tests/netcode_performance_tests
```

## API Documentation
Detailed API documentation is available on [GitHub Pages](https://yourusername.github.io/netcode-library/) or locally in the `docs/` folder after running:

```bash
# Generate documentation (requires Doxygen)
cd build
make docs
```

## Project Structure
```
netcode-library/
├── include/         # Public API headers
├── src/            # Implementation files
├── tests/          # Unit and performance tests
├── demo/           # Demo application
├── docs/           # Documentation
└── examples/       # Usage examples
```

## Performance Considerations
- Optimized for low-latency communication
- Efficient state synchronization algorithms
- Minimal memory footprint
- Thread-safe operations with lock-free implementations where possible
- Configurable buffer sizes for different use cases

## External Resources and References

The following resources were used as references during development:
- Gabriel Gambetta's ["Fast-Paced Multiplayer"](https://www.gabrielgambetta.com/client-server-game-architecture.html): Client-side prediction concepts
- Glenn Fiedler's ["Networking for Game Programmers"](https://gafferongames.com/): Netcode concepts and implementation details
- Valve Developer Community's ["Source Multiplayer Networking"](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking): Lag compensation and entity interpolation

## Contributing
Contributions are welcome! Please read our [Contributing Guidelines](CONTRIBUTING.md) for details on our code of conduct and the process for submitting pull requests.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Authors
- Josefine Arntsen
- Magnus Eik
- Håvard Versto Daleng

## Acknowledgments
Special thanks to NTNU's Network Programming course (IDATT2104) for providing the opportunity to develop this project.