#include "netcode/server.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/utils/network_logger.hpp"
#include "netcode/serialization.hpp"
#include <iostream>
#include <cstring> // For strerror, memset
#include <unistd.h> // For close
#include <cerrno>   // For errno

Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) {
    memset(&server_addr_, 0, sizeof(server_addr_));
    LOG_INFO("Server created at port " + std::to_string(port), "Server");
}

Server::~Server() {
    stop();
}

bool Server::start() {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        LOG_ERROR("Error creating socket: " + std::string(strerror(errno)), "Server");
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
        LOG_ERROR("Error binding socket: " + std::string(strerror(errno)), "Server");
        close(socket_fd_); // Clean up the socket if bind fails
        socket_fd_ = -1;
        return false;
    }

    running_ = true;
    LOG_INFO("UDP Server started on port " + std::to_string(port_), "Server");
    return true;
}

void Server::stop() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        running_ = false;
        LOG_INFO("Server stopped", "Server");
    }
}

bool Server::is_running() const {
    return running_ && (socket_fd_ >= 0);
}

bool Server::send_packet(const netcode::Buffer &buffer, const struct sockaddr_in &client_addr) {
    if (!is_running()) {
        LOG_ERROR("Cannot send data: server is not running", "Server");
        return false;
    }

    ssize_t bytes_sent = sendto(socket_fd_, buffer.get_data(), buffer.get_size(), 0,
                            (struct sockaddr*)&client_addr, sizeof(client_addr));

    if (bytes_sent < 0) {
        LOG_ERROR("Error sending data: " + std::string(strerror(errno)), "Server");
        return false;
    }
    if (static_cast<size_t>(bytes_sent) != buffer.get_size()) {
        LOG_WARNING("Warning: Not all data was sent. Sent " +
                       std::to_string(bytes_sent) + " of " + std::to_string(buffer.get_size()), "Server");
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    LOG_DEBUG("Sent data, size: " + std::to_string(bytes_sent) +
                   " bytes to " + std::string(client_ip) + ":" +
                   std::to_string(ntohs(client_addr.sin_port)), "Server");

    return static_cast<size_t>(bytes_sent) == buffer.get_size();
}

int Server::receive_packet(netcode::Buffer &buffer, size_t max_size, struct sockaddr_in &client_addr) {
    if (!is_running()) {
        LOG_ERROR("Cannot receive data: server is not running", "Server");
        return -1;
    }

    std::vector<char> temp_recv_buf(max_size);
    socklen_t client_addr_len = sizeof(client_addr);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // Microseconds (0.5 seconds)

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        LOG_WARNING("Error setting socket timeout:  " + std::string(strerror(errno)), "Server");
        // Continue without timeout or handle as critical error
    }

    socklen_t client_len = sizeof(client_addr); // Important: Initialize client_len

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), max_size, 0,
                            (struct sockaddr*)&client_addr, &client_addr_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout occurred, this is not necessarily an error,
            // it just means no data was received within the timeout period.
            LOG_DEBUG("Timeout while receicing data", "Server");
            return 0; // Indicate no data received
        }
        LOG_ERROR("Error receiving data: " + std::string(strerror(errno)), "Server");
        return -1; // Indicate an error
    }

    buffer.clear();
    buffer.data.assign(temp_recv_buf.begin(), temp_recv_buf.begin() + bytes_received);
    buffer.read_offset = 0;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    LOG_DEBUG("Recevied data, size: " + std::to_string(bytes_received) +
                   " bytes from " + std::string(client_ip) + ":" +
                   std::to_string(ntohs(client_addr.sin_port)), "Server");

    return static_cast<int>(bytes_received);
}

