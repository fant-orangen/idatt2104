// server.hpp
#pragma once

#include <string>
#include <vector> // Added for std::vector in process_packet if needed by Buffer
#include <thread> // Added for std::thread
#include <mutex>  // Potentially needed if clients_ access becomes more complex
#include <atomic> // Added for std::atomic
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <chrono>

// Forward declaration
namespace netcode {
    class Buffer;
    struct PacketHeader; // Make sure this is included or forward-declared if used in header
}
#include "netcode/packet_types.hpp" // Include for PacketHeader and MessageType

struct ClientInfo {
    struct sockaddr_in address;
    std::chrono::steady_clock::time_point last_seen;
    std::string client_id;
};

class Server {
public:
    explicit Server(int port);
    ~Server();

    bool start();
    void stop();
    bool is_running() const;

    // Existing public methods
    bool send_packet(const netcode::Buffer& buffer, const struct sockaddr_in& client_addr);
    int receive_packet(netcode::Buffer& buffer, size_t max_size, struct sockaddr_in& client_addr); // This might become private or used by listener_loop
    void add_or_update_client(const struct sockaddr_in& client_addr);
    void remove_inactive_clients(int timeout_seconds = 60);
    void send_to_all_clients(const netcode::Buffer& buffer);


private:
    void listener_loop(); // Method for the listening thread
    void process_packet(netcode::Buffer& buffer, const ClientInfo& client_info); // Method to handle different packet types

    int port_;
    std::atomic<bool> running_; // Changed to atomic for thread safety
    int socket_fd_;
    struct sockaddr_in server_addr_;

    std::map<std::string, ClientInfo> clients_;
    std::mutex clients_mutex_; // Added mutex for protecting clients_ map

    std::string get_client_key(const struct sockaddr_in& client_addr) const;

    std::thread listener_thread_; // Thread for listener_loop
};