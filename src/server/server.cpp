/**
 * @file server.cpp
 * @brief Implements the UDP Server class for managing client connections,
 *        receiving and sending packets, and broadcasting messages.
 *
 * This file defines the methods of the Server class. The Server listens on
 * a UDP socket, tracks connected clients, and dispatches incoming messages
 * to the appropriate handler. Clients are identified by their IP and port,
 * and inactive clients can be purged after a configurable timeout.
 */

#include "netcode/server.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/serialization.hpp"
#include "netcode/packet_types.hpp"

#include <cstring>      // strerror, memset
#include <unistd.h>     // close
#include <cerrno>       // errno
#include <vector>
#include <arpa/inet.h>  // inet_ntop, ntohs, htonl, INADDR_ANY
#include <chrono>
#include <thread>
#include <mutex>

/**
 * @brief Generate a unique key string for a client based on IP and port.
 *
 * Uses inet_ntop to convert the IPv4 address to a human-readable string,
 * then appends the port number. Logs a warning on conversion failure.
 *
 * @param client_addr IPv4 client address structure.
 * @return A string of the form "<IP>:<port>", or "invalid_ip:<port>" on error.
 */
std::string Server::get_client_key(const struct sockaddr_in& client_addr) const {
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
        LOG_WARNING("inet_ntop failed in get_client_key", "Server");
        return "invalid_ip:" + std::to_string(ntohs(client_addr.sin_port));
    }
    return std::string(ip_str) + ":" + std::to_string(ntohs(client_addr.sin_port));
}

/**
 * @brief Construct a new Server object.
 *
 * Initializes the server port, sets the running flag to false,
 * and initializes the socket file descriptor to -1.
 *
 * @param port The port number to bind the UDP socket to (0 for ephemeral).
 */
Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) {
    std::memset(&server_addr_, 0, sizeof(server_addr_));
    LOG_INFO("Server created for port " + std::to_string(port_), "Server");
}

/**
 * @brief Destroy the Server object and release resources.
 *
 * Calls stop() to close the socket, join the listener thread,
 * and clear the client list.
 */
Server::~Server() {
    stop();
}

/**
 * @brief Start the UDP server.
 *
 * Creates and binds a UDP socket to the configured port (or an ephemeral port
 * if 0 was specified). Spawns the listener thread to process incoming packets.
 *
 * @return true if the server was started successfully, false otherwise.
 */
bool Server::start() {
    if (running_) {
        LOG_WARNING("Server already running.", "Server");
        return true;
    }

    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        LOG_ERROR("socket() failed: " + std::string(strerror(errno)), "Server");
        return false;
    }

    int reuse = 1;
    setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr_.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(socket_fd_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
        LOG_ERROR("bind() failed on port " + std::to_string(port_) + ": " + std::string(strerror(errno)), "Server");
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    if (port_ == 0) {
        struct sockaddr_in actual;
        socklen_t len = sizeof(actual);
        if (getsockname(socket_fd_, (struct sockaddr*)&actual, &len) == 0) {
            port_ = ntohs(actual.sin_port);
            LOG_INFO("Bound to ephemeral port " + std::to_string(port_), "Server");
        }
    }

    running_ = true;
    try {
        listener_thread_ = std::thread(&Server::listener_loop, this);
    } catch (const std::system_error& e) {
        LOG_ERROR("thread start failed: " + std::string(e.what()), "Server");
        running_ = false;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    LOG_INFO("Server started on port " + std::to_string(port_), "Server");
    return true;
}

/**
 * @brief Stop the UDP server.
 *
 * Sets the running flag to false, closes the socket to unblock the listener,
 * joins the listener thread, and clears the client list.
 * Safe to call multiple times.
 */
void Server::stop() {
    if (!running_.exchange(false)) {
        if (listener_thread_.joinable()) listener_thread_.join();
        LOG_INFO("Server stop called on non-running server.", "Server");
        return;
    }

    LOG_INFO("Stopping server...", "Server");
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    if (listener_thread_.joinable()) {
        try { listener_thread_.join(); }
        catch (const std::system_error& e) {
            LOG_ERROR("join failed: " + std::string(e.what()), "Server");
        }
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
    }
    LOG_INFO("Server stopped and clients cleared.", "Server");
}

/**
 * @brief Check if the server is currently running.
 *
 * @return true if the server is active and listening, false otherwise.
 */
bool Server::is_running() const noexcept {
    return running_;
}

/**
 * @brief Receive a UDP packet with a timeout.
 *
 * Attempts to read up to max_size bytes into a temporary buffer,
 * then writes those bytes into the provided netcode::Buffer.
 * Uses SO_RCVTIMEO to enforce a 1-second timeout, allowing the
 * listener loop to periodically check the running flag.
 *
 * @param buffer Reference to a Buffer to fill with the received data.
 * @param max_size Maximum number of bytes to attempt to read.
 * @param client_addr Output param for the sender's address.
 * @return >0 on success (number of bytes), 0 on timeout, -1 on error.
 */
int Server::receive_packet(netcode::Buffer& buffer, size_t max_size, struct sockaddr_in& client_addr) {
    if (!running_ || socket_fd_ < 0) return -1;

    std::vector<char> temp(max_size);
    struct timeval tv{1, 0};
    setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    socklen_t addrlen = sizeof(client_addr);
    ssize_t n = recvfrom(socket_fd_, temp.data(), temp.size(), 0,
                         (struct sockaddr*)&client_addr, &addrlen);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        if (running_) LOG_ERROR("recvfrom failed: " + std::string(strerror(errno)), "Server");
        return -1;
    }

    buffer.clear();
    buffer.write_bytes(temp.data(), static_cast<size_t>(n));
    return static_cast<int>(n);
}

/**
 * @brief Main listener loop running in its own thread.
 *
 * Continuously calls receive_packet, handles timeouts and errors,
 * updates client state, and dispatches incoming data to process_packet().
 */
void Server::listener_loop() {
    LOG_INFO("Listener thread started.", "Server");
    netcode::Buffer recv_buf;
    struct sockaddr_in client_addr;

    while (running_) {
        int n = receive_packet(recv_buf, 2048, client_addr);
        if (n > 0) {
            std::string key = get_client_key(client_addr);
            add_or_update_client(client_addr);

            ClientInfo info;
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                info = clients_[key];
            }
            process_packet(recv_buf, info);
        }
        else if (n < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // n == 0: timeout, immediately loop again
    }
    LOG_INFO("Listener thread exiting.", "Server");
}

/**
 * @brief Process a single packet from a client.
 *
 * Parses the PacketHeader, then switches on the MessageType.
 * Currently supports ECHO_REQUEST -> ECHO_RESPONSE functionality.
 *
 * @param buf Buffer containing the received packet (header + payload).
 * @param ci ClientInfo of the sender, including address and ID.
 */
void Server::process_packet(netcode::Buffer& buf, const ClientInfo& ci) {
    if (buf.get_size() < sizeof(netcode::PacketHeader)) {
        LOG_WARNING("Packet too small from " + ci.client_id, "Server");
        return;
    }

    netcode::PacketHeader hdr = buf.read_header();
    switch (hdr.type) {
    case netcode::MessageType::ECHO_REQUEST: {
        netcode::Buffer resp;
        netcode::PacketHeader rh;
        rh.type = netcode::MessageType::ECHO_RESPONSE;
        rh.sequenceNumber = hdr.sequenceNumber;
        resp.write_header(rh);

        size_t rem = buf.get_remaining();
        if (rem) {
            std::vector<char> payload(rem);
            buf.read_bytes(payload.data(), rem);
            resp.write_bytes(payload.data(), rem);
        }
        send_packet(resp, ci.address);
        break;
    }
    default:
        LOG_WARNING("Unhandled type " + std::to_string(int(hdr.type)) + " from " + ci.client_id, "Server");
        break;
    }
}

/**
 * @brief Add a new client or update last_seen for an existing one.
 *
 * Generates a key for the client, locks the client's map, then either
 * inserts a new ClientInfo or updates the timestamp on the existing one.
 *
 * @param addr The client's sockaddr_in address.
 */
void Server::add_or_update_client(const struct sockaddr_in& addr) {
    std::string key = get_client_key(addr);
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(key);
    if (it == clients_.end()) {
        clients_[key] = ClientInfo{addr, now, key};
        LOG_INFO("New client " + key, "Server");
    } else {
        it->second.last_seen = now;
    }
}

/**
 * @brief Send a packet to a specific client.
 *
 * Wraps sendto(), logging if the send fails or only partial data is sent.
 *
 * @param buf Buffer containing the packet to send.
 * @param addr Destination client address.
 * @return true if full packet was sent, false otherwise.
 */
bool Server::send_packet(const netcode::Buffer& buf, const struct sockaddr_in& addr) {
    if (!running_ || socket_fd_ < 0) return false;
    ssize_t n = sendto(socket_fd_, buf.get_data(), buf.get_size(), 0,
                       (const struct sockaddr*)&addr, sizeof(addr));
    if (n != static_cast<ssize_t>(buf.get_size())) {
        LOG_WARNING("sendto sent " + std::to_string(n) + " of " + std::to_string(buf.get_size()), "Server");
        return false;
    }
    return true;
}

/**
 * @brief Broadcast a packet to all connected clients.
 *
 * Takes a snapshot of clients under lock, then iterates outside the lock
 * to send the packet to each, avoiding long-held mutex.
 *
 * @param buf Buffer containing the packet to broadcast.
 */
void Server::send_to_all_clients(const netcode::Buffer& buf) {
    if (!running_) return;
    std::vector<ClientInfo> copy;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (const auto& kv : clients_) copy.push_back(kv.second);
    }
    for (const auto& ci : copy) {
        send_packet(buf, ci.address);
    }
}

/**
 * @brief Remove clients inactive for longer than timeout.
 *
 * Collects inactive client keys under lock, then removes them in a second
 * lock to avoid iterator invalidation. Logs each removal.
 *
 * @param timeout_seconds Maximum allowed inactivity in seconds.
 */
void Server::remove_inactive_clients(int timeout_seconds) {
    if (!running_) return;
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> to_remove;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (const auto& kv : clients_) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - kv.second.last_seen).count();
            if (age > timeout_seconds) to_remove.push_back(kv.first);
        }
    }

    if (!to_remove.empty()) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (const auto& key : to_remove) {
            clients_.erase(key);
            LOG_INFO("Removed inactive " + key, "Server");
        }
    }
}