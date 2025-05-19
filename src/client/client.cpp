#include "netcode/client.hpp"
#include <iostream>

Client::Client(const std::string& server_ip, int port)
    : server_ip_(server_ip), port_(port), connected_(false) {}

bool Client::connect() {
    connected_ = true;
    std::cout << "Client connected to " << server_ip_ << ":" << port_ << std::endl;
    return true;
}

void Client::disconnect() {
    connected_ = false;
    std::cout << "Client disconnected" << std::endl;
}

bool Client::is_connected() const {
    return connected_;
} 