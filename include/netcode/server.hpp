#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Server {
public:
    Server(int port);
    ~Server();

    bool start();
    void stop();
    bool is_running() const;

    bool send_data(const void* data, size_t size, const struct sockaddr_in& client_addr);
    int receive_data(void* data, size_t buffer_size, struct sockaddr_in& client_addr);

private:
    int port_;
    bool running_;
    int socket_fd_;
    struct sockaddr_in server_addr_;
}; 