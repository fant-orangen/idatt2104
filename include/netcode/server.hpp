#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace netcode {
    class Buffer;
}

class Server {
public:
    Server(int port);
    ~Server();

    bool start();
    void stop();
    bool is_running() const;

    bool send_packet(const netcode::Buffer& buffer, const struct sockaddr_in& client_addr);
    int receive_packet(netcode::Buffer& buffer, size_t max_size, struct sockaddr_in& client_addr);

private:
    int port_;
    bool running_;
    int socket_fd_;
    struct sockaddr_in server_addr_;
}; 