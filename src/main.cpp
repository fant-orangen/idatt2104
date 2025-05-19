#include "netcode/client.hpp"
#include "netcode/server.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

std::atomic<bool> server_running(true);

// Server thread function
void server_function() {
    Server server(12345);

    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return;
    }

    char buffer[1024];
    struct sockaddr_in client_addr;

    std::cout << "Server waiting for messages..." << std::endl;

    while (server_running) {
        int bytes_received = server.receive_data(buffer, sizeof(buffer), client_addr);

        if (bytes_received > 0) {
            // Null-terminate the received data
            buffer[bytes_received] = '\0';

            // Get client IP and port for display
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);

            std::cout << "Server received: " << buffer << " from "
                      << client_ip << ":" << client_port << std::endl;

            // Echo back the message
            std::string response = "Echo: ";
            response += buffer;

            server.send_data(response.c_str(), response.length(), client_addr);
        }

        // Brief sleep to prevent CPU hogging
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    server.stop();
}

int main() {
    // Start server in a separate thread
    std::thread server_thread(server_function);

    // Brief pause to ensure server is started
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Create and connect client
    Client client("127.0.0.1", 12345);
    if (!client.connect()) {
        std::cerr << "Failed to connect client" << std::endl;
        server_running = false;
        server_thread.join();
        return 1;
    }

    // Send a few test messages
    const char* messages[] = {
        "Hello, Server!",
        "This is a UDP test",
        "Testing basic socket communication"
    };

    for (const char* msg : messages) {
        std::cout << "Client sending: " << msg << std::endl;

        if (!client.send_data(msg, strlen(msg))) {
            std::cerr << "Failed to send message" << std::endl;
            continue;
        }

        // Wait for server response
        char buffer[1024];
        int bytes_received = client.receive_data(buffer, sizeof(buffer));

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Client received: " << buffer << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Cleanup
    client.disconnect();
    server_running = false;
    server_thread.join();

    std::cout << "UDP socket test completed successfully" << std::endl;
    return 0;
}