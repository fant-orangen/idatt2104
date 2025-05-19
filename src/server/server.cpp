#include "netcode/server.hpp"
#include <iostream>

Server::Server(int port) : port_(port), running_(false) {}

bool Server::start() {
    running_ = true;
    std::cout << "Server started on port " << port_ << std::endl;
    return true;
}

void Server::stop() {
    running_ = false;
    std::cout << "Server stopped" << std::endl;
}

bool Server::is_running() const {
    return running_;
} 