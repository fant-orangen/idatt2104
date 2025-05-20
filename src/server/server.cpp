/**
 * @file server.cpp
 * @brief Implementation of the Server class for UDP communication.
 *
 * This file contains the method definitions for the Server class, which
 * provides functionality to start a UDP server, listen for incoming data from clients,
 * and send data back to clients.
 */

#include "netcode/server.hpp" // Corresponding header file
#include <iostream>             // For std::cerr, std::endl, std::cout
#include <cstring>              // For strerror, memset
#include <unistd.h>             // For close
#include <cerrno>               // For errno

/**
 * @brief Constructs a new Server object.
 *
 * Initializes the server with the port number on which it will listen.
 * The server is not started upon construction. The server address structure
 * is zeroed out.
 *
 * @param port The port number for the server to listen on.
 */
Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) {
    // Initialize server_addr_ to ensure it's zeroed out before use in start()
    memset(&server_addr_, 0, sizeof(server_addr_));
}

/**
 * @brief Destroys the Server object.
 *
 * Ensures that the server is stopped and the socket is closed
 * when the object goes out of scope.
 */
Server::~Server() {
    stop();
}

/**
 * @brief Starts the UDP server.
 *
 * Creates a UDP socket (SOCK_DGRAM), sets it to listen on all available network interfaces
 * (INADDR_ANY) on the port specified during construction, and binds the socket.
 *
 * @return `true` if the server was started successfully (socket created and bound),
 * `false` otherwise (e.g., error creating socket or binding).
 */
bool Server::start() {
    // 1. Create a UDP socket (SOCK_DGRAM)
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Configure server address structure
    server_addr_.sin_family = AF_INET;                  // IPv4
    server_addr_.sin_addr.s_addr = INADDR_ANY;          // Listen on all available interfaces
    server_addr_.sin_port = htons(port_);               // Convert port to network byte order

    // Bind the socket to the server address and port
    if (bind(socket_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(socket_fd_); // Clean up the socket if bind fails
        socket_fd_ = -1;   // Mark socket as invalid
        return false;
    }

    running_ = true; // Set server state to running
    std::cout << "UDP Server started on port " << port_ << std::endl;
    return true;
}

/**
 * @brief Stops the UDP server.
 *
 * Closes the server's listening socket and sets its running state to false.
 */
void Server::stop() {
    if (socket_fd_ >= 0) { // Check if the socket is valid
        close(socket_fd_);  // Close the socket descriptor
        socket_fd_ = -1;    // Mark socket as invalid
        running_ = false;   // Update running status
        std::cout << "Server stopped" << std::endl;
    }
}

/**
 * @brief Checks if the server is currently running.
 *
 * @return `true` if the server has a valid socket descriptor and its state is running,
 * `false` otherwise.
 */
bool Server::is_running() const {
    return running_ && (socket_fd_ >= 0);
}

/**
 * @brief Sends data to a specific client.
 *
 * Uses `sendto` to send data to the client identified by `client_addr`.
 *
 * @param data Pointer to the data buffer to send.
 * @param size The number of bytes to send from the data buffer.
 * @param client_addr The address structure of the client to send data to.
 * @return `true` if the data was sent successfully (all specified bytes were queued),
 * `false` if the server is not running or an error occurred during sending.
 * A warning is printed if not all data was sent.
 */
bool Server::send_data(const void *data, size_t size, const struct sockaddr_in &client_addr) {
    if (!is_running()) {
        std::cerr << "Cannot send data: server is not running" << std::endl;
        return false;
    }

    // Send data using sendto, specifying the client's address
    ssize_t bytes_sent = sendto(socket_fd_, data, size, 0,
                            (struct sockaddr*)&client_addr, sizeof(client_addr));

    if (bytes_sent < 0) {
        std::cerr << "Error sending data: " << strerror(errno) << std::endl;
        return false;
    }
    // Check if the number of bytes sent matches the requested size
    if (static_cast<size_t>(bytes_sent) != size) {
        std::cerr << "Warning: Not all data was sent. Sent " << bytes_sent << " of " << size << std::endl;
        // Depending on requirements, partial send might be treated as an error or success.
        // Here, it's treated as success if bytes_sent >= 0, but a warning is issued.
    }

    return static_cast<size_t>(bytes_sent) == size; // Return true only if all data was sent
}

/**
 * @brief Receives data from a client.
 *
 * Attempts to receive data into the provided buffer using `recvfrom`.
 * This method also populates `client_addr` with the address of the client
 * from which data was received. Includes an optional socket timeout (SO_RCVTIMEO)
 * of 500ms to prevent blocking indefinitely.
 *
 * @param data Pointer to the buffer where received data will be stored.
 * @param buffer_size The maximum number of bytes to read into the buffer.
 * @param client_addr Reference to a `sockaddr_in` structure that will be
 * populated with the client's address information.
 * @return The number of bytes received, or 0 if a timeout occurred,
 * or -1 if an error occurred (other than timeout).
 */
int Server::receive_data(void *data, size_t buffer_size, struct sockaddr_in &client_addr) {
    if (!is_running()) {
        std::cerr << "Cannot receive data: server is not running" << std::endl;
        return -1;
    }

    // Optional: Set a timeout for the recvfrom operation
    struct timeval tv;
    tv.tv_sec = 0;        // 0 seconds
    tv.tv_usec = 500000;  // 500,000 microseconds = 0.5 seconds

    // Apply the timeout option to the socket
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
        // Depending on requirements, this could be a critical error or ignored.
    }

    socklen_t client_len = sizeof(client_addr); // Important: Initialize client_len for recvfrom

    // Attempt to receive data, client_addr will be populated by recvfrom
    ssize_t bytes_received = recvfrom(socket_fd_, data, buffer_size, 0,
                            (struct sockaddr*)&client_addr, &client_len);

    if (bytes_received < 0) {
        // Check for timeout (EAGAIN or EWOULDBLOCK are common indicators)
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // Timeout occurred, no data received
        }
        // Another error occurred
        std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
        return -1; // Indicate an error
    }

    return static_cast<int>(bytes_received); // Return the number of bytes received
}
