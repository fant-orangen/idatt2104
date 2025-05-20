/**
* @file client.cpp
 * @brief Implements the Client class for UDP communication.
 *
 * This file contains the method definitions for the Client class, which handles
 * setting up a UDP socket, connecting to a server, sending, and receiving packets.
 */

#include "netcode/client.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/utils/network_logger.hpp"
#include "netcode/serialization.hpp"
#include <iostream>
#include <cstring>
#include <vector>
#include <cerrno>
#include <unistd.h> // For close
#include <sys/socket.h> // For SOL_SOCKET, SO_RCVTIMEO
#include <netinet/in.h> // For sockaddr_in, htons, ntohs
#include <arpa/inet.h>  // For inet_pton, inet_ntop

/**
 * @brief Constructs a Client object.
 *
 * Initializes client settings, such as server IP and port.
 * The actual socket connection is established by calling connect_to_server().
 *
 * @param server_ip The IP address of the server to connect to.
 * @param port The port number of the server.
 */
Client::Client(const std::string& server_ip, int port)
    : server_ip_(server_ip), port_(port), connected_(false), socket_fd_(-1) {
    LOG_INFO("Client created for " + server_ip + ":" + std::to_string(port), "Client");
    memset(&server_addr_, 0, sizeof(server_addr_));
}

/**
 * @brief Destroys the Client object.
 *
 * Ensures that the client is disconnected and the socket is closed if it was open.
 */
Client::~Client() {
    disconnect_from_server();
}

/**
 * @brief Connects the client to the server.
 *
 * Creates a UDP socket and configures the server address structure
 * for communication. Sets the connected_ flag upon success.
 *
 * @return True if the socket was created and configured successfully, false otherwise.
 */
bool Client::connect_to_server() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        LOG_ERROR("Error creating socket: " + std::string(strerror(errno)), "Client");
        return false;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_);
    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr) <= 0) {
        LOG_ERROR("Invalid address or address not supported: " + server_ip_ + " Error: " + std::string(strerror(errno)), "Client");
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 1; // 1-second timeout for receive operations
    tv.tv_usec = 0;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        LOG_WARNING("Error setting socket timeout: " + std::string(strerror(errno)), "Client");
    }

    connected_ = true;
    LOG_INFO("Client 'connected' (UDP socket configured for " + server_ip_ + ":" + std::to_string(port_) + ")", "Client");
    return true;
}


/**
 * @brief Disconnects the client from the server.
 *
 * Closes the socket if it is open and resets the connection status.
 */
void Client::disconnect_from_server() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
        LOG_INFO("Client disconnected and socket closed.", "Client");
    }
}

/**
 * @brief Checks if the client is currently connected.
 *
 * A client is considered connected if the socket file descriptor is valid
 * and the connected_ flag is true.
 *
 * @return True if the client is connected, false otherwise.
 */
bool Client::is_connected() const {
    return connected_ && (socket_fd_ >= 0);
}

/**
 * @brief Sends a packet (buffer) to the connected server.
 *
 * @param buffer The netcode::Buffer containing the data to send.
 * The buffer should be prepared with a header and payload.
 * @return True if the packet was sent successfully, false otherwise.
 */
bool Client::send_packet(const netcode::Buffer& buffer) {
    if (!is_connected()) {
        LOG_ERROR("Cannot send packet: client not connected.", "Client:" + std::to_string(port_));
        return false;
    }

    LOG_INFO("Client:" + std::to_string(port_) + " sending packet to " + server_ip_ + ":" + std::to_string(ntohs(server_addr_.sin_port)) + 
             ", size: " + std::to_string(buffer.get_size()) + " bytes", "Client");

    ssize_t bytes_sent = sendto(socket_fd_, buffer.get_data(), buffer.get_size(), 0,
                              (struct sockaddr*)&server_addr_, sizeof(server_addr_));

    if (bytes_sent < 0) {
        LOG_ERROR("Error sending packet: " + std::string(strerror(errno)), "Client:" + std::to_string(port_));
        return false;
    }
    if (static_cast<size_t>(bytes_sent) != buffer.get_size()) {
        LOG_WARNING("Not all data sent. Sent " + std::to_string(bytes_sent) + " of " + std::to_string(buffer.get_size()) + " bytes.", "Client:" + std::to_string(port_));
        return false;
    }

    LOG_DEBUG("Successfully sent packet, size: " + std::to_string(bytes_sent) + " bytes.", "Client:" + std::to_string(port_));
    return true;
}

/**
 * @brief Receives a packet into the provided buffer.
 *
 * Attempts to receive a packet from the server. This implementation includes
 * a very short, effectively non-blocking timeout (tv_sec = 0, tv_usec = 0) by default.
 *
 * @param buffer Reference to a netcode::Buffer where the received data will be stored.
 * The buffer is cleared and then filled with the received packet data.
 * @param max_size The maximum number of bytes to read into the temporary receive buffer.
 * @return The number of bytes received on success.
 * Returns 0 if a timeout occurred (EAGAIN or EWOULDBLOCK with non-blocking sockets or short timeout).
 * Returns -1 on error (and prints an error message).
 */
int Client::receive_packet(netcode::Buffer& buffer, size_t max_size) {
    if (!is_connected()) {
        LOG_ERROR("Cannot receive packet: client not connected.", "Client:" + std::to_string(port_));
        return -1;
    }

    std::vector<char> temp_recv_buf(max_size);
    struct sockaddr_in from_address;
    socklen_t from_len = sizeof(from_address);

    LOG_DEBUG("Client:" + std::to_string(port_) + " waiting for packet from server, max size: " + std::to_string(max_size) + " bytes", "Client");

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), temp_recv_buf.size(), 0,
                                    (struct sockaddr*)&from_address, &from_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG_DEBUG("Receive timeout.", "Client:" + std::to_string(port_));
            return 0;
        }
        LOG_ERROR("Error receiving packet: " + std::string(strerror(errno)), "Client:" + std::to_string(port_));
        return -1;
    }

    buffer.clear(); // This resets internal read_offset to 0
    buffer.write_bytes(temp_recv_buf.data(), static_cast<size_t>(bytes_received));

    if (bytes_received > 0) {
        char from_ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from_address.sin_addr, from_ip_str, INET_ADDRSTRLEN);
        LOG_INFO("Client:" + std::to_string(port_) + " received packet, size: " + std::to_string(bytes_received) +
                 " bytes from " + std::string(from_ip_str) + ":" +
                 std::to_string(ntohs(from_address.sin_port)), "Client");
    }
    return static_cast<int>(bytes_received);
}