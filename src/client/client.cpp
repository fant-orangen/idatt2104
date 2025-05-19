#include "netcode/client.hpp"
#include "netcode/serialization.hpp""
#include <iostream>
#include <cstring>
#include <vector>
#include <cstring>
#include <cerrno>

Client::Client(const std::string& server_ip, int port)
    : server_ip_(server_ip), port_(port), connected_(false), socket_fd_(-1) {

    memset(&server_addr_, 0, sizeof(server_addr_));
}

Client::~Client() {
    disconnect_from_server();
}

bool Client::connect_to_server() {
    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_);

    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr) <= 0) {
        std::cerr << "Error parsing server IP address: " << strerror(errno) << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    connected_ = true;
    std::cout << "Client connected to " << server_ip_ << ":" << port_ << std::endl;
    return true;
}

void Client::disconnect_from_server() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
        std::cout << "Client disconnected" << std::endl;
    }
}

bool Client::is_connected() const {
    return connected_ && (socket_fd_ >= 0);
}

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

int Client::receive_packet(netcode::Buffer& buffer, size_t max_size) {
    if (!is_connected()) {
        std::cerr << "Cannot receive data: client not connected" << std::endl;
        return -1;
    }

    std::vector<char> temp_recv_buf(max_size);
    struct sockaddr_in from_address;
    socklen_t from_len = sizeof(from_address);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Client: Warning - Error setting socket receive timeout: " << strerror(errno) << std::endl;
    }

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), max_size, 0,
                            (struct sockaddr*)&from_address, &from_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        std::cerr << "Client: Error receiving data: " << strerror(errno) << std::endl;
        return -1;
    }

    buffer.clear();
    buffer.data.assign(temp_recv_buf.begin(), temp_recv_buf.begin() + bytes_received);
    buffer.read_offset = 0;

    return static_cast<int>(bytes_received);
}
