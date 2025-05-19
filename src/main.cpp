#include "netcode/client.hpp"
#include "netcode/server.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    // Create and start server
    Server server(12345);
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    // Create and connect client
    Client client("127.0.0.1", 12345);
    if (!client.connect()) {
        std::cerr << "Failed to connect client" << std::endl;
        return 1;
    }

    // Keep running for a few seconds to simulate activity
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Cleanup
    client.disconnect();
    server.stop();

    std::cout << "Test completed successfully" << std::endl;
    return 0;
} 