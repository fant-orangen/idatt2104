#include "netcode/server.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/serialization.hpp"
#include "netcode/packet_types.hpp"

#include <iostream>
#include <cstring>      // strerror, memset
#include <unistd.h>     // close
#include <cerrno>       // errno
#include <vector>
#include <arpa/inet.h>  // inet_ntop, ntohs, htonl, INADDR_ANY
#include <chrono>
#include <thread>
#include <mutex>

// Generate a unique key for each client (IP:port)
std::string Server::get_client_key(const struct sockaddr_in& client_addr) const {
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str)) == nullptr) {
        LOG_WARNING("inet_ntop failed in get_client_key", "Server");
        return "invalid_ip:" + std::to_string(ntohs(client_addr.sin_port));
    }
    return std::string(ip_str) + ":" + std::to_string(ntohs(client_addr.sin_port));
}

Server::Server(int port)
    : port_(port), running_(false), socket_fd_(-1) {
    memset(&server_addr_, 0, sizeof(server_addr_));
    LOG_INFO("Server created for port " + std::to_string(port_), "Server");
}

Server::~Server() {
    stop();
}

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

void Server::stop() {
    // Idempotent shutdown
    if (!running_.exchange(false)) {
        // not running or already stopping
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

bool Server::is_running() const noexcept {
    return running_;
}

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
        // on n == 0 (timeout), just loop
    }
    LOG_INFO("Listener thread exiting.", "Server");
}

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

bool Server::send_packet(const netcode::Buffer& buf, const struct sockaddr_in& addr) {
    if (!running_ || socket_fd_ < 0) return false;
    ssize_t n = sendto(socket_fd_, buf.get_data(), buf.get_size(), 0,
                       (struct sockaddr*)&addr, sizeof(addr));
    if (n != static_cast<ssize_t>(buf.get_size())) {
        LOG_WARNING("sendto sent " + std::to_string(n) + " of " + std::to_string(buf.get_size()), "Server");
        return false;
    }
    return true;
}

void Server::send_to_all_clients(const netcode::Buffer& buf) {
    if (!running_) return;
    std::vector<ClientInfo> copy;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& kv : clients_) copy.push_back(kv.second);
    }
    for (auto& ci : copy) {
        send_packet(buf, ci.address);
    }
}

void Server::remove_inactive_clients(int timeout_seconds) {
    if (!running_) return;
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> to_remove;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& kv : clients_) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - kv.second.last_seen).count();
            if (age > timeout_seconds) to_remove.push_back(kv.first);
        }
    }

    if (!to_remove.empty()) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& key : to_remove) {
            clients_.erase(key);
            LOG_INFO("Removed inactive " + key, "Server");
        }
    }
}