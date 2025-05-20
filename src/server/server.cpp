// server.cpp
#include "netcode/server.hpp"
#include "netcode/utils/logger.hpp"
// #include "netcode/utils/network_logger.hpp" // Already included in your provided snippet
#include "netcode/serialization.hpp"
#include "netcode/packet_types.hpp" // Make sure this is included for PacketHeader, MessageType

#include <iostream>
#include <cstring> // For strerror, memset
#include <unistd.h> // For close
#include <cerrno>   // For errno
#include <vector>
#include <arpa/inet.h> // For inet_ntop, ntohs, htonl, INADDR_ANY
#include <chrono>      // For std::chrono (already in your snippet)


// get_client_key - (Ensure this is robust, especially if client_addr might not be fully initialized)
std::string Server::get_client_key(const struct sockaddr_in& client_addr) const {
    char ip_str[INET_ADDRSTRLEN];
    // Ensure client_addr.sin_addr is valid before calling inet_ntop
    if (inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN) == nullptr) {
        // Handle error or return a default/error key
        LOG_WARNING("inet_ntop failed in get_client_key", "Server");
        return "invalid_ip:" + std::to_string(ntohs(client_addr.sin_port));
    }
    return std::string(ip_str) + ":" + std::to_string(ntohs(client_addr.sin_port));
}


Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) { // running_ initialized to false
    memset(&server_addr_, 0, sizeof(server_addr_));
    LOG_INFO("Server created for port " + std::to_string(port), "Server");
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running_) {
        LOG_WARNING("Server is already running.", "Server");
        return true;
    }

    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        LOG_ERROR("Error creating socket: " + std::string(strerror(errno)), "Server");
        return false;
    }

    int reuse_addr = 1;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) < 0) {
        LOG_WARNING("Warning - setsockopt (SO_REUSEADDR) failed: " + std::string(strerror(errno)), "Server");
        // Not fatal, continue
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all available interfaces
    server_addr_.sin_port = htons(static_cast<uint16_t>(port_)); // Use static_cast for safety if port_ could be negative

    if (bind(socket_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        LOG_ERROR("Error binding socket to port " + std::to_string(port_) + ": " + std::string(strerror(errno)), "Server");
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // If port was 0, get the assigned ephemeral port
    if (port_ == 0) {
        struct sockaddr_in actual_addr;
        socklen_t len = sizeof(actual_addr);
        if (getsockname(socket_fd_, (struct sockaddr *)&actual_addr, &len) == 0) {
            port_ = ntohs(actual_addr.sin_port);
            LOG_INFO("Server bound to ephemeral port: " + std::to_string(port_), "Server");
        } else {
            LOG_WARNING("Could not get ephemeral port number: " + std::string(strerror(errno)), "Server");
        }
    }


    running_ = true; // Set running to true before starting the thread
    try {
        listener_thread_ = std::thread(&Server::listener_loop, this);
    } catch (const std::system_error& e) {
        LOG_ERROR("Failed to start listener thread: " + std::string(e.what()), "Server");
        running_ = false;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    LOG_INFO("UDP Server started and listening on port " + std::to_string(port_), "Server");
    return true;
}

void Server::stop() {
    if (!running_.exchange(false)) { // Atomically set running to false and get previous value
        // If it was already false, server might be stopped or in the process of stopping
        if (listener_thread_.joinable()) {
            listener_thread_.join(); // Still try to join if stop was called concurrently
        }
        LOG_INFO("Server was already stopped or stop was called multiple times.", "Server");
        return;
    }

    LOG_INFO("Stopping server...", "Server");

    if (socket_fd_ >= 0) {
        // Optionally, shutdown can help unblock recvfrom immediately
        // SHUT_RDWR can be too aggressive if other threads are using the socket for sending.
        // For a simple UDP server, closing might be enough, or SHUT_RD.
        // shutdown(socket_fd_, SHUT_RD);
        close(socket_fd_);
        socket_fd_ = -1;
    }

    if (listener_thread_.joinable()) {
        try {
            listener_thread_.join();
            LOG_INFO("Listener thread joined.", "Server");
        } catch (const std::system_error& e) {
            LOG_ERROR("Error joining listener thread: " + std::string(e.what()), "Server");
        }
    }

    // clients_.clear(); // Already in your provided server.cpp snippet
    // This should be protected by the mutex if clearing here.
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
    }
    LOG_INFO("Server stopped and clients cleared.", "Server");
}


bool Server::is_running() const {
    // running_ flag is the primary indicator. socket_fd_ check is secondary.
    return running_;
}

// receive_packet as per your existing code, but now used by listener_loop
// This sets a short timeout for recvfrom.
int Server::receive_packet(netcode::Buffer &buffer, size_t max_size, struct sockaddr_in &client_addr) {
    if (!running_) { // Check the atomic running_ flag
        // LOG_DEBUG("Receive_packet called but server is not running.", "Server");
        return -1; // Or 0, to indicate not an error but just not operational
    }
    if (socket_fd_ < 0) {
        LOG_ERROR("Cannot receive data: socket is not valid", "Server");
        return -1;
    }

    std::vector<char> temp_recv_buf(max_size);
    socklen_t client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));

    // The timeout is set here for each call. This is fine.
    // For a dedicated listener thread, this timeout allows the loop to check `running_` flag.
    struct timeval tv;
    tv.tv_sec = 1; // 1 second timeout for recvfrom (adjust as needed)
    tv.tv_usec = 0;
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        LOG_WARNING("Error setting socket RCVTIMEO: " + std::string(strerror(errno)), "Server");
    }

    ssize_t bytes_received = recvfrom(socket_fd_, temp_recv_buf.data(), temp_recv_buf.size(), 0,
                            (struct sockaddr*)&client_addr, &client_addr_len);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // LOG_DEBUG("Timeout while receiving data (recvfrom)", "Server");
            return 0;
        }
        // Only log error if server is supposed to be running, to avoid spam on shutdown
        if (running_) {
            LOG_ERROR("Error in recvfrom: " + std::string(strerror(errno)), "Server");
        }
        return -1;
    }

    buffer.clear();
    buffer.write_bytes(temp_recv_buf.data(), static_cast<size_t>(bytes_received));

    // Note: add_or_update_client is called by listener_loop after successful receive
    return static_cast<int>(bytes_received);
}


void Server::listener_loop() {
    LOG_INFO("Listener loop started.", "Server");
    netcode::Buffer recv_buffer;
    struct sockaddr_in client_saddr; // Corrected variable name

    while (running_) {
        // Max packet size to expect
        int bytes_received = receive_packet(recv_buffer, 2048, client_saddr); // Corrected variable name

        if (bytes_received > 0) {
            // Successfully received a packet
            char client_ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_saddr.sin_addr, client_ip_str, INET_ADDRSTRLEN); // Corrected variable name
            LOG_DEBUG("Listener loop: Received " + std::to_string(bytes_received) + " bytes from " +
                       std::string(client_ip_str) + ":" + std::to_string(ntohs(client_saddr.sin_port)), "Server");  // Corrected variable name

            std::string client_key_str = get_client_key(client_saddr); // Corrected variable name

            ClientInfo current_client_info; // Populate this to pass to process_packet
            current_client_info.address = client_saddr; // Corrected variable name
            current_client_info.client_id = client_key_str;
            // last_seen will be updated by add_or_update_client

            add_or_update_client(client_saddr); // Manage client state (updates last_seen) // Corrected variable name

            // Retrieve the potentially updated ClientInfo for process_packet
            // (or pass client_saddr directly if process_packet can work with that)
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto it = clients_.find(client_key_str);
                if (it != clients_.end()) {
                    current_client_info = it->second; // Get the full ClientInfo
                } else {
                    // Should not happen if add_or_update_client worked, but handle defensively
                    LOG_WARNING("Client info not found after add_or_update for: " + client_key_str, "Server");
                    continue;
                }
            }
            process_packet(recv_buffer, current_client_info);

        } else if (bytes_received == 0) {
            // Timeout, loop again to check running_ flag
            continue;
        } else { // bytes_received < 0
            // Error in receive_packet (already logged there if running_)
            // Potentially add a small delay here to prevent rapid spin on persistent errors
            if (running_) { // Only sleep if we are supposed to be running
                 std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    LOG_INFO("Listener loop stopped.", "Server");
}

void Server::process_packet(netcode::Buffer& buffer, const ClientInfo& client_info) {
    // client_info contains client_info.address which is struct sockaddr_in
    if (buffer.get_size() < sizeof(netcode::PacketHeader)) {
        LOG_WARNING("Received packet too small to be a valid packet from " + client_info.client_id, "Server");
        return;
    }

    // It's safer to read from a copy of the buffer's read offset for peeking, or ensure read_header doesn't permanently advance offset if not processed
    // For now, assume read_header is fine for direct use here.
    netcode::PacketHeader header = buffer.read_header(); // This advances the buffer's read offset

    LOG_DEBUG("Processing packet type " + std::to_string(static_cast<int>(header.type)) +
               " with seq " + std::to_string(header.sequenceNumber) +
               " from client " + client_info.client_id, "Server");

    switch (header.type) {
        case netcode::MessageType::ECHO_REQUEST: {
            LOG_INFO("Received ECHO_REQUEST from " + client_info.client_id, "Server");
            netcode::Buffer response_buffer;
            netcode::PacketHeader response_header;
            response_header.type = netcode::MessageType::ECHO_RESPONSE;
            response_header.sequenceNumber = header.sequenceNumber; // Echo back the sequence number

            response_buffer.write_header(response_header);
            // Echo back the rest of the payload from the original buffer
            // The original buffer's read_offset is already past the header.
            // We need to get the remaining data from the received buffer.
            size_t payload_size = buffer.get_remaining();
            if (payload_size > 0) {
                std::vector<char> payload_data(payload_size);
                buffer.read_bytes(payload_data.data(), payload_size);
                response_buffer.write_bytes(payload_data.data(), payload_size);
            }

            send_packet(response_buffer, client_info.address);
            break;
        }
        // Add cases for other message types here
        // case netcode::MessageType::PLAYER_INPUT:
        //    // ...
        //    break;
        default:
            LOG_WARNING("Received unhandled packet type: " + std::to_string(static_cast<int>(header.type)) +
                       " from " + client_info.client_id, "Server");
            break;
    }
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
    // Lock the mutex before accessing clients_ map
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(client_key);
    if (it == clients_.end()) {
        ClientInfo new_client_info;
        new_client_info.address = client_addr;
        new_client_info.last_seen = std::chrono::steady_clock::now();
        new_client_info.client_id = client_key;
        clients_[client_key] = new_client_info;
        LOG_INFO("New client added: " + client_key + " (ID: " + new_client_info.client_id + "). Total clients: " + std::to_string(clients_.size()), "Server");
    } else {
        it->second.last_seen = std::chrono::steady_clock::now();
        // LOG_DEBUG("Client " + client_key + " seen again.", "Server"); // Can be verbose
    }
}

// send_packet (from your snippet, ensure it's using client_info.address correctly)
bool Server::send_packet(const netcode::Buffer &buffer, const struct sockaddr_in &client_addr) {
    if (!running_ || socket_fd_ < 0) {
        LOG_ERROR("Cannot send packet: server is not running or socket invalid", "Server");
        return false;
    }

    ssize_t bytes_sent = sendto(socket_fd_, buffer.get_data(), buffer.get_size(), 0,
                            (struct sockaddr*)&client_addr, sizeof(client_addr));

    if (bytes_sent < 0) {
        LOG_ERROR("Error sending packet: " + std::string(strerror(errno)), "Server");
        return false;
    }
    if (static_cast<size_t>(bytes_sent) != buffer.get_size()) {
        LOG_WARNING("Not all data sent. Sent " + std::to_string(bytes_sent) + " of " + std::to_string(buffer.get_size()) + " bytes.", "Server");
        return false;
    }
    // Logging for sent data can be verbose, enable if needed for debugging
    // char client_ip_str[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &client_addr.sin_addr, client_ip_str, INET_ADDRSTRLEN);
    // LOG_DEBUG("Sent packet, size: " + std::to_string(bytes_sent) + " bytes to " + std::string(client_ip_str) + ":" + std::to_string(ntohs(client_addr.sin_port)), "Server");
    return true;
}


// send_to_all_clients (from your snippet, ensure it uses the mutex)
void Server::send_to_all_clients(const netcode::Buffer& buffer) {
    if (!running_) {
        LOG_WARNING("Server not running, cannot send to all clients.", "Server");
        return;
    }

    std::vector<ClientInfo> clients_copy;
    { // Scope for the lock
        std::lock_guard<std::mutex> lock(clients_mutex_);
        if (clients_.empty()) {
            LOG_DEBUG("No clients connected to send message to.", "Server");
            return;
        }
        LOG_DEBUG("Preparing to broadcast message to " + std::to_string(clients_.size()) + " clients.", "Server");
        for (const auto& pair : clients_) {
            clients_copy.push_back(pair.second);
        }
    } // Mutex released here

    for (const auto& client_info : clients_copy) {
        if (!send_packet(buffer, client_info.address)) {
            LOG_WARNING("Failed to send packet to client: " + client_info.client_id, "Server");
        }
    }
}

// remove_inactive_clients (from your snippet, ensure it uses the mutex)
void Server::remove_inactive_clients(int timeout_seconds) {
    if (!running_) return;

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> clients_to_remove_keys;

    // Safely collect keys of clients to remove
    { // Scope for the lock
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (const auto& pair : clients_) {
            auto last_seen_duration = std::chrono::duration_cast<std::chrono::seconds>(now - pair.second.last_seen);
            if (last_seen_duration.count() > timeout_seconds) {
                clients_to_remove_keys.push_back(pair.first);
            }
        }
    } // Mutex released

    // Remove clients outside the initial lock to avoid iterator invalidation issues with map
    if (!clients_to_remove_keys.empty()) {
        std::lock_guard<std::mutex> lock(clients_mutex_); // Re-lock for modification
        for (const std::string& client_key : clients_to_remove_keys) {
            clients_.erase(client_key);
            LOG_INFO("Removed inactive client: " + client_key, "Server");
        }
    }
}