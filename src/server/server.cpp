#include "netcode/server.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/utils/network_logger.hpp"
#include "netcode/serialization.hpp" // For netcode::Buffer
#include <iostream>
#include <cstring> // For strerror, memset
#include <unistd.h> // For close
#include <cerrno>   // For errno
#include <vector>
#include <arpa/inet.h> // For inet_ntop, ntohs
#include <chrono>      // For std::chrono::steady_clock

// Helper function to create a unique string key from sockaddr_in
std::string Server::get_client_key(const struct sockaddr_in& client_addr) const {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    return std::string(ip_str) + ":" + std::to_string(ntohs(client_addr.sin_port));
}

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
        LOG_WARNING("Warning - setsockopt (SO_REUSEADDR) failed: " + std::string(strerror(errno)), "Server");
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
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
        socket_fd_ = -1; // Mark as closed
    }
    running_ = false; // Set running to false regardless of socket state
    clients_.clear(); // Clear client list
    LOG_INFO("Server stopped", "Server");
}

bool Server::is_running() const {
    return running_ && (socket_fd_ >= 0);
}

/**
 * @brief Adds a new client or updates the last_seen time of an existing client.
 *
 * A unique key is generated from the client's IP address and port.
 * If the client is new (based on this key), a ClientInfo struct is created,
 * storing its network address (`sockaddr_in`), the current time as `last_seen`,
 * and a `client_id` (derived from the key). This new ClientInfo object is then
 * added to an internal map of connected clients.
 * If the client already exists in the map, only its `last_seen` timestamp is updated
 * to the current time.
 *
 * @param client_addr The sockaddr_in structure containing the client's address information.
 */
void Server::add_or_update_client(const struct sockaddr_in& client_addr) {
    std::string client_key = get_client_key(client_addr);
    auto it = clients_.find(client_key);

    if (it == clients_.end()) {
        // New client
        ClientInfo new_client_info;
        new_client_info.address = client_addr;
        new_client_info.last_seen = std::chrono::steady_clock::now();
        new_client_info.client_id = client_key; // Use the key as a simple client_id for now
        clients_[client_key] = new_client_info;
        LOG_INFO("New client connected: " + client_key + " (ID: " + new_client_info.client_id + ")", "Server");
    } else {
        // Existing client, update last_seen time
        it->second.last_seen = std::chrono::steady_clock::now();
        // Optionally, update address if it can change (e.g., NAT rebinding), though client_key would also change then.
        // it->second.address = client_addr;
        LOG_DEBUG("Client " + client_key + " seen again.", "Server");
    }
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
        // Consider this a failure for reliable protocols, or handle as needed
        return false;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    LOG_DEBUG("Sent data, size: " + std::to_string(bytes_sent) +
                   " bytes to " + std::string(client_ip) + ":" +
                   std::to_string(ntohs(client_addr.sin_port)), "Server");

    return true; // Success if all bytes were sent
}

int Server::receive_packet(netcode::Buffer &buffer, size_t max_size, struct sockaddr_in &client_addr) {
    if (!is_running()) {
        LOG_ERROR("Cannot receive data: server is not running", "Server");
        return -1;
    }

    // Use a temporary buffer for recvfrom, then copy to the netcode::Buffer
    std::vector<char> temp_recv_buf(max_size);
    socklen_t client_addr_len = sizeof(client_addr);

    // Clear client_addr before recvfrom
    memset(&client_addr, 0, sizeof(client_addr));

    // Configure a short timeout for recvfrom to make it non-blocking or nearly so
    struct timeval tv;
    tv.tv_sec = 0; // Seconds
    tv.tv_usec = 10000; // Microseconds (10 ms), adjust as needed for responsiveness

    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        LOG_WARNING("Error setting socket timeout: " + std::string(strerror(errno)), "Server");
        // Depending on requirements, you might return -1 or continue without timeout
    }

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), temp_recv_buf.size(), 0, // Use temp_recv_buf.size()
                            (struct sockaddr*)&client_addr, &client_addr_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout occurred, this is not an error, just no data.
            // LOG_DEBUG("Timeout while receiving data", "Server"); // Can be verbose
            return 0; // Indicate no data received due to timeout
        }
        LOG_ERROR("Error receiving data: " + std::string(strerror(errno)), "Server");
        return -1; // Indicate an actual error
    }

    // Successfully received data, copy it to the provided netcode::Buffer
    buffer.clear(); // Clear any existing data and reset read_offset
    // Ensure you only assign the received number of bytes
    buffer.write_bytes(temp_recv_buf.data(), static_cast<size_t>(bytes_received));
    // buffer.read_offset is managed internally by Buffer class now

    if (bytes_received > 0) { // Log and update client info only if data was actually received
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        LOG_DEBUG("Received data, size: " + std::to_string(bytes_received) +
                       " bytes from " + std::string(client_ip) + ":" +
                       std::to_string(ntohs(client_addr.sin_port)), "Server");

        add_or_update_client(client_addr); // Manage client state
    }

    return static_cast<int>(bytes_received);
}

// Implementation for send_to_all_clients
void Server::send_to_all_clients(const netcode::Buffer& buffer) {
    if (!is_running()) {
        LOG_WARNING("Server not running, cannot send to all clients.", "Server");
        return;
    }
    if (clients_.empty()) {
        LOG_DEBUG("No clients connected to send message to.", "Server");
        return;
    }

    LOG_DEBUG("Broadcasting message to " + std::to_string(clients_.size()) + " clients.", "Server");
    for (const auto& pair : clients_) {
        // pair.first is the client_key (string)
        // pair.second is the ClientInfo struct
        if (!send_packet(buffer, pair.second.address)) {
            LOG_WARNING("Failed to send packet to client: " + pair.first, "Server");
            // Optionally, mark this client for removal or handle error
        }
    }
}

// Implementation for remove_inactive_clients
void Server::remove_inactive_clients(int timeout_seconds) {
    if (!is_running()) return;

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> clients_to_remove;

    for (const auto& pair : clients_) {
        auto last_seen_duration = std::chrono::duration_cast<std::chrono::seconds>(now - pair.second.last_seen);
        if (last_seen_duration.count() > timeout_seconds) {
            clients_to_remove.push_back(pair.first);
        }
    }

    for (const std::string& client_key : clients_to_remove) {
        clients_.erase(client_key);
        LOG_INFO("Removed inactive client: " + client_key, "Server");
    }
}