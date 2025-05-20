#include "netcode/client.hpp"
#include "netcode/server.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/utils/network_logger.hpp"
#include "netcode/utils/visualization_logger.hpp"
#include "netcode/visualization/game_window.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

std::atomic<bool> server_running(true);

// Server thread function
void server_function() {
    LOG_INFO("Starting serverthread", "ServerThread");

    Server server(12345);

    if (!server.start()) {
        LOG_ERROR("Failed to start server", "ServerThread");
        return;
    }

    char buffer[1024];
    struct sockaddr_in client_addr;

    LOG_INFO("Server waiting for messages...", "ServerThread");

    while (server_running) {
        int bytes_received = server.receive_data(buffer, sizeof(buffer), client_addr);

        if (bytes_received > 0) {
            // Null-terminate the received data
            buffer[bytes_received] = '\0';

            // Get client IP and port for display
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);

            LOG_INFO("Server received: " + std::string(buffer) + " from " +
                      std::string(client_ip) + ":" + std::to_string(client_port), "ServerThread");

            // Echo back the message
            std::string response = "Echo: ";
            response += buffer;

            if (!server.send_data(response.c_str(), response.length(), client_addr)) {
                LOG_ERROR("Failed to send response", "ServerThread");
            }
        }

        // Brief sleep to prevent CPU hogging
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    server.stop();
}

// Small demo showing visualization with logger
void run_visualization() {
    auto window = std::make_unique<netcode::visualization::GameWindow>("Netcode Visualization", 800, 600);

    // Initialize visualization logger with the window
    netcode::utils::VisualizationLogger::initialize(window.get());

    // Run the game
    window->run();

    // Shutdown the logger
    netcode::utils::VisualizationLogger::shutdown();
}

int main(int argc, char** argv) {
    // Initialize the logging system
    netcode::utils::Logger::get_instance().set_level(netcode::utils::LogLevel::DEBUG);
    netcode::utils::Logger::get_instance().set_log_file("netcode.log");
    LOG_INFO("Netcode application starting", "Main");

    // Check command line arguments
    bool run_visualization_demo = false;
    if (argc > 1 && std::string(argv[1]) == "--visual") {
        run_visualization_demo = true;
    }

    if (run_visualization_demo) {
        // Run the visualization demo
        run_visualization();
    } else {
        // Standard network test
        // Start server in a separate thread
        std::thread server_thread(server_function);

        // Brief pause to ensure server is started
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Create and connect client
        Client client("127.0.0.1", 12345);
        if (!client.connect()) {
            LOG_ERROR("Could not connect client", "Main");
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

            if (!client.send_data(msg, strlen(msg))) {
                LOG_ERROR("Could not send message", "Main");
                continue;
            }

            // Wait for server response
            char buffer[1024];
            int bytes_received = client.receive_data(buffer, sizeof(buffer));

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                LOG_INFO("Received: " + std::string(buffer), "Main");
            } else if (bytes_received == 0) {
                LOG_WARNING("Timeout while waiting for response", "Main");
            } else {
                LOG_ERROR("Error receiving data", "Main");
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Cleanup
        client.disconnect();

        server_running = false;
        server_thread.join();

        LOG_INFO("UDP socket test completed", "Main");
    }

    return 0;
}
