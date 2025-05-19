#include "netcode/server.hpp"
#include <iostream>
#include <cstring>

Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) {}

Server::~Server() {
    stop();
}

bool Server::start() {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY;
    server_addr_.sin_port = htons(port_);

    if (bind(socket_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    running_ = true;
    std::cout << "Server started on port " << port_ << std::endl;
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

    return bytes_sent == size;
}

int Server::receive_data(void *data, size_t buffer_size, struct sockaddr_in &client_addr) {
    if (!is_running()) {
        std::cerr << "Cannot receive data: server is not running" << std::endl;
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
    }

    socklen_t client_len = sizeof(struct  sockaddr_in);

    ssize_t bytes_received = recvfrom(socket_fd_, data, buffer_size, 0,
                            (struct sockaddr*)&client_addr, &client_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
        return -1;
    }

    return bytes_received;
}

