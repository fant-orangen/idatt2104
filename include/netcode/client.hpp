#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace netcode {
    class Buffer;
}

class Client {
public:
    Client(const std::string& server_ip, int port);
    ~Client();

    bool connect_to_server();
    void disconnect_from_server();
    bool is_connected() const;
    // Send a packet constructed in a netcode::Buffer
    bool send_packet(const netcode::Buffer& buffer);
    // Receive a packet into a netcode::Buffer
    int receive_packet(netcode::Buffer& buffer, size_t max_size);

private:
    std::string server_ip_;
    int port_;
    bool connected_;
    int socket_fd_;
    struct sockaddr_in server_addr_;
}; 