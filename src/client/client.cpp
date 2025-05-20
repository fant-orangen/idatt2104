/**
* @file client.cpp
 * @brief Implements the Client class for UDP communication.
 *
 * This file contains the method definitions for the Client class, which handles
 * setting up a UDP socket, connecting to a server, sending, and receiving packets.
 */

#include "netcode/client.hpp"
#include "netcode/serialization.hpp"
#include <iostream>
#include <cstring>
#include <vector>
#include <cerrno>

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
    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }
    // Configure server address
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_); // Convert port to network byte order
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr) <= 0) {
        std::cerr << "Error parsing server IP address: " << strerror(errno) << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Note: For UDP, "connect" is not strictly necessary for sendto/recvfrom if the destination is specified each time.
    // However, calling connect() on a UDP socket allows the use of send() and recv() and sets a default destination.
    // This implementation uses sendto/recvfrom directly with server_addr_, so a call to connect() is omitted here.

    connected_ = true;
    std::cout << "Client connected to " << server_ip_ << ":" << port_ << std::endl;
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
        std::cout << "Client disconnected" << std::endl;
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
        std::cerr << "Cannot send data: client not connected" << std::endl;
        return false;
    }

    ssize_t bytes_sent = sendto(socket_fd_, buffer.get_data(), buffer.get_size(), 0,
                              (struct sockaddr*)&server_addr_, sizeof(server_addr_));

    if (static_cast<size_t>(bytes_sent) != buffer.get_size()) {
        std::cerr << "Error sending data: " << strerror(errno) << std::endl;
        return false;
    }

    return static_cast<size_t>(bytes_sent) == buffer.get_size();
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
        std::cerr << "Cannot receive data: client not connected" << std::endl;
        return -1;
    }

    std::vector<char> temp_recv_buf(max_size);
    struct sockaddr_in from_address;
    socklen_t from_len = sizeof(from_address);

    // Set a very short timeout for the receive operation to make it non-blocking or nearly so.
    // This specific timeout (0 sec, 0 usec) might behave differently on various systems.
    // For true non-blocking, the socket itself could be set to O_NONBLOCK.
    // For a reliable timeout, a positive value should be used.
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Client: Warning - Error setting socket receive timeout: " << strerror(errno) << std::endl;
        // Continue anyway, but behavior might be blocking
    }

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), max_size, 0,
                            (struct sockaddr*)&from_address, &from_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // This means timeout occurred with the socket option SO_RCVTIMEO
            // std::cout << "Client: Receive timeout." << std::endl;
            return 0;
        }
        std::cerr << "Client: Error receiving data: " << strerror(errno) << std::endl;
        return -1;
    }

    // Successfully received data, copy it to the provided netcode::Buffer
    buffer.clear(); // Clear any existing data in the user's buffer
    buffer.data.assign(temp_recv_buf.begin(), temp_recv_buf.begin() + bytes_received);
    buffer.read_offset = 0; // Reset read offset for the new data

    return static_cast<int>(bytes_received);
}
