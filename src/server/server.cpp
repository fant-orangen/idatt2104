#include "netcode/server.hpp"
#include "netcode/serialization.hpp"
#include <iostream>
#include <cstring> // For strerror, memset
#include <unistd.h> // For close
#include <cerrno>   // For errno
#include <vector>


// Helper function to create a unique string key from sockaddr_in
// Needs to be part of the Server class or accessible to it
std::string Server::get_client_key(const struct sockaddr_in& client_addr) const {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    return std::string(ip_str) + ":" + std::to_string(ntohs(client_addr.sin_port));
}

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
        clients_.clear();
        std::cout << "Server stopped" << std::endl;
    }
}

bool Server::is_running() const {
    return running_ && (socket_fd_ >= 0);
}

void Server::add_or_update_client(const struct sockaddr_in& client_addr) {
    std::string client_key = get_client_key(client_addr);
    clients_[client_key] = {client_addr, std::chrono::steady_clock::now()};
}

void Server::remove_inactive_clients(int timeout_seconds) {
    auto now = std::chrono::steady_clock::now();
    for (auto it = clients_.begin(); it != clients_.end();) {
        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen).count() > timeout_seconds) {
            std::cout << "Server: Client " << it->first << " timed out. Removing." << std::endl;
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::send_to_all_clients(const netcode::Buffer& buffer) {
    if (!is_running()) {
        std::cerr << "Server: Cannot send to all clients: server not runnign" << std::endl;
        return;
    }
    if (clients_.empty()) {
        std::cout << "Server: No clients to send to." << std::endl;
        return;
    }

    std::cout << "Server: Broadcasting packet to " << clients_.size() << " clients." << std::endl;
    for (const auto& pair : clients_) {
        send_packet(buffer, pair.second.address);
    }
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

    // Clear client_addr before recvfrom
    memset(&client_addr, 0, sizeof(client_addr));

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000; // Microseconds (10 ms)

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Server: Warning - Error setting socket timeout: " << strerror(errno) << std::endl;
    }

    // socklen_t client_len = sizeof(client_addr);

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), max_size, 0,
                            (struct sockaddr*)&client_addr, &client_addr_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        std::cerr << "Server: Error receiving data: " << strerror(errno) << std::endl;
        return -1;
    }

    if (bytes_received > 0) {
        add_or_update_client(client_addr);
        buffer.clear();
        buffer.data.assign(temp_recv_buf.begin(), temp_recv_buf.begin() + bytes_received);
        buffer.read_offset = 0;
    }
    return static_cast<int>(bytes_received);
}

