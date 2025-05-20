#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <chrono>

namespace netcode {
    class Buffer;
}

struct ClientInfo {
    struct sockaddr_in address;
    std::chrono::steady_clock::time_point last_seen;
    std::string client_id;
};

class Server {
public:
    Server(int port);
    ~Server();

    bool start();
    void stop();
    bool is_running() const;

    bool send_packet(const netcode::Buffer& buffer, const struct sockaddr_in& client_addr);
    int receive_packet(netcode::Buffer& buffer, size_t max_size, struct sockaddr_in& client_addr);

    // Client management methods
    void add_or_update_client(const struct sockaddr_in& client_addr);
    void remove_inactive_clients(int timeout_seconds = 60);
    void send_to_all_clients(const netcode::Buffer& buffer);

private:
    int port_;
    bool running_;
    int socket_fd_;
    struct sockaddr_in server_addr_;
    std::map<std::string, ClientInfo> clients_;
    std::string get_client_key(const struct sockaddr_in& client_addr) const;
}; 