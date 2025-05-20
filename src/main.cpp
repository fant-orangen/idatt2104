/**
* @file main.cpp
 * @brief Main application file for a simple client-server echo demonstration.
 *
 * This file contains the main entry point and the server's primary logic.
 * It sets up a server in a separate thread and a client in the main thread.
 * The client sends a few messages to the server, and the server echoes them back.
 * Demonstrates basic socket programming, threading, and custom packet serialization.
 */

#include "netcode/client.hpp"
#include "netcode/server.hpp"
#include "netcode/serialization.hpp" // For Buffer, PacketHeader, MessageType
#include "netcode/utils/logger.hpp"
#include "netcode/utils/network_logger.hpp"
#include "netcode/utils/visualization_logger.hpp"
#include "netcode/visualization/game_window.hpp"
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <vector>
#include <arpa/inet.h>

/**
 * @brief Atomic boolean to control the server thread's execution.
 *
 * When set to false, the server thread will attempt to shut down gracefully.
 */
std::atomic<bool> server_should_run(true);

/**
 * @brief Function executed by the server thread.
 *
 * Initializes and starts the server. Then, it enters a loop to receive packets,
 * process them (currently only ECHO_REQUEST), and send responses.
 * The loop continues as long as server_should_run is true.
 * Handles basic error checking for server start, packet reception, and sending.
 */
void server_function() {
    LOG_INFO("Starting serverthread", "ServerThread");

    Server server(12345);

    if (!server.start()) {
        server_should_run = false; // Signal main thread
        LOG_ERROR("Failed to start server", "ServerThread");
        return;
    }

    netcode::Buffer receive_buffer;
    struct sockaddr_in client_address_info;

    LOG_INFO("Server waiting for messages...", "ServerThread");

    while (server_should_run.load()) {
        // Attempt to receive a packet with a timeout (implicit in Server::receive_packet if non-blocking)

        int bytes_received = server.receive_packet(receive_buffer, 1024, client_address_info);

        if (bytes_received > 0) {
            try {
                // Reset read offset before reading from buffer if it's reused
                receive_buffer.read_offset_ = 0;
                netcode::PacketHeader header = receive_buffer.read_header();

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_address_info.sin_addr), client_ip, INET_ADDRSTRLEN);
                int client_port = ntohs(client_address_info.sin_port);

                LOG_INFO("Server received: " + std::string(receive_buffer.get_data() + receive_buffer.read_offset_) +
                         " from " + std::string(client_ip) + ":" + std::to_string(client_port), "ServerThread");

                if (header.type == netcode::MessageType::ECHO_REQUEST) {
                    std::string payload_str = receive_buffer.read_string();
                    LOG_DEBUG("EchoRequest payload: \"" + payload_str + "\"", "Server");

                    // Prepare and send echo response
                    netcode::Buffer response_buffer;
                    netcode::PacketHeader response_header;
                    response_header.type = netcode::MessageType::ECHO_RESPONSE;
                    response_header.sequenceNumber = header.sequenceNumber; // Echo back the same sequence

                    response_buffer.write_header(response_header);
                    response_buffer.write_string("Server Echo: " + payload_str);

                    if (!server.send_packet(response_buffer, client_address_info)) {
                        LOG_ERROR("Could not send response", "ServerThread");
                    } else {
                        LOG_DEBUG("Sent EchoResponse", "Server");
                    }
                } else {
                    LOG_WARNING("Received unhandled packet type: " + std::to_string(static_cast<int>(header.type)), "Server");
                }
            } catch (const std::runtime_error& e) {
                LOG_ERROR("Error processing packet: " + std::string(e.what()), "Server");
            }
        } else if (bytes_received < 0) {
            // An actual error occurred, not just a timeout
            LOG_ERROR("Receive error in loop. Check server logs.", "Server");
        }
        // If bytes_received == 0 (timeout), the loop continues, checking server_should_run.
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

/**
 * @brief Main entry point of the application.
 *
 * This function initializes and starts a server in a separate thread.
 * It then creates a client that attempts to connect to this server.
 * The client sends a series of ECHO_REQUEST messages and waits for ECHO_RESPONSE.
 * Finally, it signals the server to stop and waits for the server thread to join.
 *
 * @return 0 on successful completion, 1 on error (e.g., server failed to start, client failed to connect).
 */
int main() {

    netcode::utils::Logger::get_instance().set_level(netcode::utils::LogLevel::DEBUG);
    netcode::utils::Logger::get_instance().set_log_file("netcode.log");
    LOG_INFO("Netcode application starting", "Main");

    std::thread server_thread_obj(server_function);

    // Allow server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
     if (!server_should_run.load()) { // Check if the server thread signaled a failure
        LOG_ERROR("Server did not start correctly. Exiting.", "Main");
        if(server_thread_obj.joinable()) server_thread_obj.join();
        return 1;
    }


    Client client("127.0.0.1", 12345);
    if (!client.connect_to_server()) {
        LOG_ERROR("Client failed to connect/configure.", "Main");
        server_should_run = false;
        if (server_thread_obj.joinable()) server_thread_obj.join();
        return 1;
    }

    std::string messages_to_send[] = {
        "First packet!",
        "Another one, with sequence.",
        "The last test message."
    };
    uint32_t current_sequence_number = 0;

    for (const auto& msg_content : messages_to_send) {
        netcode::Buffer send_buffer;
        netcode::PacketHeader request_header;
        request_header.type = netcode::MessageType::ECHO_REQUEST;
        request_header.sequenceNumber = current_sequence_number++;

        send_buffer.write_header(request_header);
        send_buffer.write_string(msg_content);

        LOG_INFO("Sending EchoRequest. Seq: " + std::to_string(request_header.sequenceNumber) +
                 ", Payload: \"" + msg_content + "\"", "Client");

        if (!client.send_packet(send_buffer)) {
            LOG_ERROR("Failed to send packet.", "Client");
            continue;
        }

        netcode::Buffer RcvBuffer;
        int bytes = client.receive_packet(RcvBuffer, 1024);

        if (bytes > 0) {
            try {
                // Reset read offset before reading from buffer
                RcvBuffer.read_offset_ = 0;
                netcode::PacketHeader response_hdr = RcvBuffer.read_header();
                LOG_DEBUG("Received packet. Type: " + std::to_string(static_cast<int>(response_hdr.type)) +
                         ", Seq: " + std::to_string(response_hdr.sequenceNumber), "Client");

                if (response_hdr.type == netcode::MessageType::ECHO_RESPONSE) {
                    std::string response_payload_str = RcvBuffer.read_string();
                    LOG_INFO("EchoResponse payload: \"" + response_payload_str + "\"", "Client");
                } else {
                    LOG_WARNING("Received unexpected packet type.", "Client");
                }
            } catch (const std::runtime_error& e) {
                LOG_ERROR("Error processing received packet: " + std::string(e.what()), "Client");
            }
        } else if (bytes == 0) {
            LOG_WARNING("Receive timeout waiting for echo response.", "Client");
        } else {
            LOG_ERROR("Receive failed.", "Client");
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Slow down for readability
    }

    client.disconnect_from_server();
    server_should_run = false; // Signal server thread to stop
    if (server_thread_obj.joinable()) {
        server_thread_obj.join();
    }

    LOG_INFO("UDP socket test completed", "Main");
    return 0;
};