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
#include "netcode/serialization.hpp"
#include "netcode/packet_types.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/utils/network_logger.hpp"
#include <thread>
#include <chrono>
#include <string>
#include <atomic>
#include <vector>
#include <arpa/inet.h>
#include <iostream>

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
    LOG_INFO("Starting server thread...", "ServerThread");
    Server server(12345);

    if (!server.start()) {
        server_should_run = false;
        LOG_ERROR("Failed to start server.", "ServerThread");
        return;
    }

    netcode::Buffer receive_buffer; // Reusable buffer for receiving
    netcode::PacketHeader received_header;
    struct sockaddr_in client_address_info;

    LOG_INFO("Server waiting for messages...", "ServerThread");

    while (server_should_run.load()) {
        // receive_buffer is cleared within server.receive_packet or before this call if reused
        int bytes_received = server.receive_packet(receive_buffer, 1024, client_address_info);

        if (bytes_received > 0) {
            // DO NOT set receive_buffer.read_offset = 0; here. It's private.
            // The buffer's internal offset is managed by its methods (clear, read_*, write_*)
            if (netcode::try_deserialize(receive_buffer, received_header)) {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client_address_info.sin_addr), client_ip, INET_ADDRSTRLEN);
                int client_port = ntohs(client_address_info.sin_port);

                LOG_INFO("Server received packet. Type: " + std::to_string(static_cast<int>(received_header.type)) +
                         ", Seq: " + std::to_string(received_header.sequenceNumber) +
                         " from " + std::string(client_ip) + ":" + std::to_string(client_port), "ServerThread");

                switch (received_header.type) {
                    case netcode::MessageType::ECHO_REQUEST: {
                        try {
                            std::string payload_str = receive_buffer.read_string();
                            LOG_DEBUG("EchoRequest payload: \"" + payload_str + "\"", "Server");

                            netcode::Buffer response_buffer;
                            netcode::PacketHeader response_header;
                            response_header.type = netcode::MessageType::ECHO_RESPONSE;
                            response_header.sequenceNumber = received_header.sequenceNumber;

                            netcode::serialize(response_buffer, response_header);
                            response_buffer.write_string("Server Echo: " + payload_str);

                            if (!server.send_packet(response_buffer, client_address_info)) {
                                LOG_ERROR("Could not send EchoResponse.", "ServerThread");
                            } else {
                                LOG_DEBUG("Sent EchoResponse.", "Server");
                            }
                        } catch (const std::runtime_error& e) {
                            LOG_ERROR("Error processing ECHO_REQUEST payload: " + std::string(e.what()), "Server");
                        }
                        break;
                    }
                    default:
                        LOG_WARNING("Received unhandled packet type: " + std::to_string(static_cast<int>(received_header.type)), "Server");
                        break;
                }
            } else {
                LOG_WARNING("Failed to deserialize PacketHeader from received data. Bytes: " + std::to_string(bytes_received) +
                            " Remaining in buffer: " + std::to_string(receive_buffer.get_remaining()), "Server");

            }
            // Buffer is cleared by receive_packet before next use, or should be cleared here if receive_packet doesn't do it.
            // Based on current server.cpp, receive_packet calls buffer.clear().
        } else if (bytes_received < 0) {
            LOG_ERROR("Receive error in server loop.", "ServerThread");
        }
    }
    server.stop();
    LOG_INFO("Server thread stopped.", "ServerThread");
}

int main() {
    netcode::utils::Logger::get_instance().set_level(netcode::utils::LogLevel::DEBUG);
    netcode::utils::Logger::get_instance().set_log_file("netcode_app.log");
    LOG_INFO("Netcode application starting...", "Main");

    std::thread server_thread_obj(server_function);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
     if (!server_should_run.load()) {
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
        "Testing new serialization!",
        "Packet sequence test.",
        "Final message in this sequence."
    };
    uint32_t current_sequence_number = 0;

    netcode::Buffer send_buffer;
    netcode::Buffer RcvClientBuffer; // Renamed from RcvBuffer for clarity
    netcode::PacketHeader request_header;
    netcode::PacketHeader response_header;

    for (const auto& msg_content : messages_to_send) {
        send_buffer.clear();

        request_header.type = netcode::MessageType::ECHO_REQUEST;
        request_header.sequenceNumber = current_sequence_number++;

        netcode::serialize(send_buffer, request_header);
        send_buffer.write_string(msg_content);

        LOG_INFO("Client sending EchoRequest. Seq: " + std::to_string(request_header.sequenceNumber) +
                 ", Payload: \"" + msg_content + "\"", "Client");

        if (!client.send_packet(send_buffer)) {
            LOG_ERROR("Client failed to send packet.", "Client");
            continue;
        }

        // RcvClientBuffer is cleared within client.receive_packet or before this call.
        int bytes = client.receive_packet(RcvClientBuffer, 1024);

        if (bytes > 0) {
            // DO NOT set RcvClientBuffer.read_offset = 0; here.
            if (netcode::try_deserialize(RcvClientBuffer, response_header)) {
                LOG_DEBUG("Client received packet. Type: " + std::to_string(static_cast<int>(response_header.type)) +
                         ", Seq: " + std::to_string(response_header.sequenceNumber), "Client");

                if (response_header.type == netcode::MessageType::ECHO_RESPONSE) {
                    try {
                        std::string response_payload_str = RcvClientBuffer.read_string();
                        LOG_INFO("Client received EchoResponse payload: \"" + response_payload_str + "\"", "Client");
                    } catch (const std::runtime_error& e) {
                        LOG_ERROR("Error processing ECHO_RESPONSE payload: " + std::string(e.what()), "Client");
                    }
                } else if (response_header.type == netcode::MessageType::SERVER_ANNOUNCEMENT) {
                     netcode::ServerAnnouncementData announcement_payload; // Define it here
                     if (netcode::try_deserialize(RcvClientBuffer, announcement_payload)) {
                        LOG_INFO("Client received ServerAnnouncement: " + announcement_payload.message_text, "Client");
                    } else {
                        LOG_WARNING("Failed to deserialize ServerAnnouncement payload.", "Client");
                    }
                }
                else {
                    LOG_WARNING("Client received unexpected packet type: " + std::to_string(static_cast<int>(response_header.type)), "Client");
                }
            } else {
                 LOG_WARNING("Client failed to deserialize PacketHeader from received data. Bytes: " + std::to_string(bytes) +
                             " Remaining in buffer: " + std::to_string(RcvClientBuffer.get_remaining()), "Client");
            }
            // Buffer is cleared by receive_packet before next use, or should be cleared here.
            // Based on current client.cpp, receive_packet calls RcvClientBuffer.clear().
        } else if (bytes == 0) {
            LOG_WARNING("Client receive timeout waiting for echo response.", "Client");
        } else {
            LOG_ERROR("Client receive failed.", "Client");
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    client.disconnect_from_server();
    server_should_run = false;
    if (server_thread_obj.joinable()) {
        server_thread_obj.join();
    }

    LOG_INFO("Netcode application completed.", "Main");
    return 0;
}