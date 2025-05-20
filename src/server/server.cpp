#include "netcode/server.hpp"
#include "netcode/serialization.hpp"
#include <iostream>
#include <cstring> // For strerror, memset
#include <unistd.h> // For close
#include <cerrno>   // For errno

Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) {
    memset(&server_addr_, 0, sizeof(server_addr_));
}

Server::~Server() {
    stop();
}

bool Server::start() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    int reuse_addr = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        std::cerr << "Server: Warning - setsockopt (SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(port_);

    // Bind the socket to the server address and port
    if (bind(socket_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(socket_fd_); // Clean up the socket if bind fails
        socket_fd_ = -1;
        return false;
    }

    running_ = true;
    std::cout << "Server: UDP Server started on port " << port_ << std::endl;
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

bool Server::send_packet(const netcode::Buffer &buffer, const struct sockaddr_in &client_addr) {
    if (!is_running()) {
        std::cerr << "Cannot send data: server is not running" << std::endl;
        return false;
    }

    ssize_t bytes_sent = sendto(socket_fd_, buffer.get_data(), buffer.get_size(), 0,
                            (struct sockaddr*)&client_addr, sizeof(client_addr));

    if (bytes_sent < 0) {
        std::cerr << "Error sending data: " << strerror(errno) << std::endl;
        return false;
    }
    if (static_cast<size_t>(bytes_sent) != buffer.get_size()) {
        std::cerr << "Warning: Not all data was sent. Sent " << bytes_sent << " of " << buffer.get_size() << std::endl;
    }

    return static_cast<size_t>(bytes_sent) == buffer.get_size();
}

int Server::receive_packet(netcode::Buffer &buffer, size_t max_size, struct sockaddr_in &client_addr) {
    if (!is_running()) {
        std::cerr << "Server: Cannot receive data: server is not running" << std::endl;
        return -1;
    }

    std::vector<char> temp_recv_buf(max_size);
    socklen_t client_addr_len = sizeof(client_addr);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // Microseconds (0.5 seconds)

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Server: Warning - Error setting socket timeout: " << strerror(errno) << std::endl;
    }

    socklen_t client_len = sizeof(client_addr); // Important: Initialize client_len

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), max_size, 0,
                            (struct sockaddr*)&client_addr, &client_addr_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        std::cerr << "Server: Error receiving data: " << strerror(errno) << std::endl;
        return -1;
    }

    buffer.clear();
    buffer.data.assign(temp_recv_buf.begin(), temp_recv_buf.begin() + bytes_received);
    buffer.read_offset = 0;

    return static_cast<int>(bytes_received);
}

