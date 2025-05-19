#pragma once
#include <string>

class Client {
public:
    Client(const std::string& server_ip, int port);
    bool connect();
    void disconnect();
    bool is_connected() const;

private:
    std::string server_ip_;
    int port_;
    bool connected_;
}; 