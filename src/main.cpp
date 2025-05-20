/**
 * @file main.cpp
 * @brief Main application file for demonstrating basic UDP client-server communication.
 *
 * This file sets up a UDP server in a separate thread and then runs a UDP client
 * in the main thread to send messages to the server and receive echo responses.
 * It serves as a basic test and example of using the Client and Server classes
 * from the netcode library.
 */

#include "netcode/client.hpp"
#include "netcode/server.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

/**
 * @brief Atomic boolean flag to control the server's running state.
 *
 * When set to `false`, the server thread will gracefully shut down.
 */
std::atomic<bool> server_running(true);

/**
 * @brief Function executed by the server thread.
 *
 * Initializes and starts a UDP server on port 12345. It then enters a loop,
 * waiting to receive data from clients. Upon receiving data, it prints the
 * message along with the client's IP address and port, and then sends an
 * echo response back to the client. The loop continues as long as
 * `server_running` is true.
 */
void server_function() {
    Server server(12345);

    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return;
    }

    char buffer[1024];
    struct sockaddr_in client_addr; // Struct to hold client address information

    std::cout << "Server waiting for messages..." << std::endl;

    while (server_running) {
        // Attempt to receive data, client_addr will be populated by receive_data
        int bytes_received = server.receive_data(buffer, sizeof(buffer), client_addr);

        if (bytes_received > 0) {
            // Null-terminate the received data to treat it as a C-string
            buffer[bytes_received] = '\0';

            // Get client IP and port for display
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);

            std::cout << "Server received: " << buffer << " from "
                      << client_ip << ":" << client_port << std::endl;

            // Prepare and send an echo response
            std::string response = "Echo: ";
            response += buffer;

            server.send_data(response.c_str(), response.length(), client_addr);
        }

        // Brief sleep to prevent the loop from consuming too much CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    server.stop(); // Clean up server resources
}

/**
 * @brief Main entry point of the application.
 *
 * Starts the server in a separate thread, then creates a UDP client
 * that connects to the server. The client sends a series of predefined messages
 * to the server and prints the echoed responses. Finally, it disconnects the
 * client, signals the server to stop, and waits for the server thread to join
 * before exiting.
 *
 * @return 0 if the program completes successfully, 1 if the client fails to connect.
 */
int main() {
    // Start server_function in a new thread
    std::thread server_thread(server_function);

    // Brief pause to give the server thread time to start and initialize
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Create and connect the client
    Client client("127.0.0.1", 12345); // Connect to localhost on port 12345
    if (!client.connect()) {
        std::cerr << "Failed to connect client" << std::endl;
        server_running = false; // Signal server thread to stop
        server_thread.join();   // Wait for server thread to finish
        return 1; // Indicate an error
    }

    // Array of messages to send to the server
    const char* messages[] = {
        "Hello, Server!",
        "This is a UDP test",
        "Testing basic socket communication"
    };

    // Loop through the messages, send each one, and wait for an echo
    for (const char* msg : messages) {
        std::cout << "Client sending: " << msg << std::endl;

        if (!client.send_data(msg, strlen(msg))) {
            std::cerr << "Failed to send message" << std::endl;
            continue; // Skip to the next message if sending fails
        }

        // Wait for server response
        char buffer[1024];
        int bytes_received = client.receive_data(buffer, sizeof(buffer));

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Null-terminate the response
            std::cout << "Client received: " << buffer << std::endl;
        } else if (bytes_received == 0) {
            std::cout << "Client: No response from server (timeout)" << std::endl;
        } else {
            // An error occurred during receive, already printed by client.receive_data
        }

        // Pause briefly between messages
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Cleanup
    client.disconnect();        // Disconnect the client
    server_running = false;     // Signal the server thread to stop
    server_thread.join();       // Wait for the server thread to complete its execution

    std::cout << "UDP socket test completed successfully" << std::endl;
    return 0; // Indicate successful execution
}