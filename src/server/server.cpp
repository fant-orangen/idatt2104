#include "netcode/server.hpp"
#include <iostream>
#include <cstring> // For strerror, memset
#include <unistd.h> // For close
#include <cerrno>   // For errno

Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) {
    // Initialize server_addr_ here or ensure it's zeroed out before use in start()
    memset(&server_addr_, 0, sizeof(server_addr_));
}

Server::~Server() {
    stop();
}

bool Server::start() {
    // 1. Change to SOCK_DGRAM for UDP
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    server_addr_.sin_port = htons(port_);

    // Bind the socket to the server address and port
    if (bind(socket_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(socket_fd_); // Clean up the socket if bind fails
        socket_fd_ = -1;
        return false;
    }

    running_ = true;
    std::cout << "UDP Server started on port " << port_ << std::endl;
    return true;
}

void Server::stop() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        running_ = false;
        std::cout << "Server stopped" << std::endl;
    }
}

bool Server::is_running() const {
    return running_ && (socket_fd_ >= 0);
}

// This function is already suitable for UDP as it uses sendto
bool Server::send_data(const void *data, size_t size, const struct sockaddr_in &client_addr) {
    if (!is_running()) {
        std::cerr << "Cannot send data: server is not running" << std::endl;
        return false;
    }

    ssize_t bytes_sent = sendto(socket_fd_, data, size, 0,
                            (struct sockaddr*)&client_addr, sizeof(client_addr));

    if (bytes_sent < 0) {
        std::cerr << "Error sending data: " << strerror(errno) << std::endl;
        return false;
    }
    if (static_cast<size_t>(bytes_sent) != size) {
        std::cerr << "Warning: Not all data was sent. Sent " << bytes_sent << " of " << size << std::endl;
        // You might still return true or false based on whether partial send is acceptable
    }

    return static_cast<size_t>(bytes_sent) == size;
}

// This function is already suitable for UDP as it uses recvfrom
// It will populate client_addr with the sender's address information
int Server::receive_data(void *data, size_t buffer_size, struct sockaddr_in &client_addr) {
    if (!is_running()) {
        std::cerr << "Cannot receive data: server is not running" << std::endl;
        return -1;
    }

    // Optional: Set a timeout for recvfrom
    struct timeval tv;
    tv.tv_sec = 0;  // Seconds
    tv.tv_usec = 500000; // Microseconds (0.5 seconds)

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
        // Continue without timeout or handle as critical error
    }

    socklen_t client_len = sizeof(client_addr); // Important: Initialize client_len

    ssize_t bytes_received = recvfrom(socket_fd_, data, buffer_size, 0,
                            (struct sockaddr*)&client_addr, &client_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout occurred, this is not necessarily an error,
            // it just means no data was received within the timeout period.
            return 0; // Indicate no data received
        }
        std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
        return -1; // Indicate an error
    }

    return bytes_received;
}

