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
#include <functional>

// Forward declaration
namespace netcode {
    class Buffer;
    struct PacketHeader;
    
    namespace packets {
        struct PlayerStatePacket;
    }
}
#include "netcode/packet_types.hpp"

struct ClientInfo {
    struct sockaddr_in address;
    std::chrono::steady_clock::time_point last_seen;
    std::string client_id;
};

class Server {
public:
    using PlayerUpdateCallback = std::function<void(const netcode::packets::PlayerStatePacket&)>;
    
    explicit Server(int port);
    ~Server();

    bool start();
    void stop();
    bool is_running() const noexcept;

    // Existing public methods
    bool send_packet(const netcode::Buffer& buffer, const struct sockaddr_in& client_addr);
    int receive_packet(netcode::Buffer& buffer, size_t max_size, struct sockaddr_in& client_addr);
    void add_or_update_client(const struct sockaddr_in& client_addr);
    void remove_inactive_clients(int timeout_seconds = 60);
    void send_to_all_clients(const netcode::Buffer& buffer);
    
    // Process incoming packet
    void process_packet(netcode::Buffer& buffer, const ClientInfo& client_info);
    
    // Set callback for player updates
    void set_player_update_callback(const PlayerUpdateCallback& callback);
    
    // Get client key from address
    std::string get_client_key(const struct sockaddr_in& client_addr) const;

private:
    void listener_loop();

    int port_;
    std::atomic<bool> running_;
    int socket_fd_;
    struct sockaddr_in server_addr_;

    std::map<std::string, ClientInfo> clients_;
    std::mutex clients_mutex_;
    
    // Callback for player updates
    PlayerUpdateCallback player_update_callback_;

    std::thread listener_thread_;
};