#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Client {
public:
    Client(const std::string& server_ip, int port);
    ~Client();

    bool connect();
    void disconnect();
    bool is_connected() const;

    bool send_data(const void* data, size_t size);
    int receive_data(void* data, size_t buffer_size);

private:
    std::string server_ip_;
    int port_;
    bool connected_;
    int socket_fd_;
    struct sockaddr_in server_addr_;
}; 