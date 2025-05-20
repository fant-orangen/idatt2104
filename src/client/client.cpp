#include "netcode/client.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/utils/network_logger.hpp"
#include <iostream>
#include <cstring>

Client::Client(const std::string& server_ip, int port)
    : server_ip_(server_ip), port_(port), connected_(false), socket_fd_(-1) {
    LOG_INFO("Client created for " + server_ip + ":" + std::to_string(port), "Client");
}

Client::~Client() {
    disconnect();
}

bool Client::connect() {
    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        LOG_ERROR("Error creating socket: " + std::string(strerror(errno)), "Client");
        return false;
    }

    // Initialize server address structure
    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_);

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr) <= 0) {
        LOG_ERROR("Invalid address: " + std::string(strerror(errno)), "Client");
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    connected_ = true;
    LOG_INFO("Client connected to " + server_ip_ + ":" + std::to_string(port_), "Client");
    return true;
}

void Client::disconnect() {
    if (socket_fd_ >= 0) {
        // Send a disconnection message (optional)
        if (connected_) {
            const char* disconnect_msg = "DISCONNECT";
            sendto(socket_fd_, disconnect_msg, strlen(disconnect_msg), 0,
                 (struct sockaddr*)&server_addr_, sizeof(server_addr_));
            LOG_INFO("Disconnection message sent ", "Client");
        }

        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
        LOG_INFO("Client disconnected", "Client");
    }
}

bool Client::is_connected() const {
    return connected_ && (socket_fd_ >= 0);
}

bool Client::send_data(const void* data, size_t size) {
    if (!is_connected()) {
        LOG_ERROR("Cannot send data: client not connected", "Client");
        return false;
    }

    ssize_t bytes_sent = sendto(socket_fd_, data, size, 0,
                              (struct sockaddr*)&server_addr_, sizeof(server_addr_));

    if (bytes_sent < 0) {
        LOG_ERROR("Error sending data: " + std::string(strerror(errno)), "Client");
        return false;
    }

    LOG_DEBUG("Sent data, size: " + std::to_string(bytes_sent) + " bytes", "Client");
    return bytes_sent == size;
}

int Client::receive_data(void* buffer, size_t buffer_size) {
    if (!is_connected()) {
        LOG_ERROR("Cannot receive data: client not connected", "Client");
        return -1;
    }

    // Set up a timeout for receive operation
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000; // 500ms

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        LOG_WARNING("Error setting socket timeout: " + std::string(strerror(errno)), "Client");
    }

    sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    ssize_t bytes_received = recvfrom(socket_fd_, buffer, buffer_size, 0,
                                     (struct sockaddr*)&from_addr, &from_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout occurred, not necessarily an error
            LOG_DEBUG("Timeout when receiving data", "Client");
            return 0;
        }
        LOG_ERROR("Error receiving data: " + std::string(strerror(errno)), "Client");
        return -1;
    }

    char from_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from_addr.sin_addr, from_ip, INET_ADDRSTRLEN);
    LOG_DEBUG("Received data, size: " + std::to_string(bytes_received) +
                   " bytes from " + std::string(from_ip) + ":" +
                   std::to_string(ntohs(from_addr.sin_port)), "Client");


    return bytes_received;
}
