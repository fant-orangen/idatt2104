#include "netcode/client.hpp"
#include <iostream>
#include <cstring>

Client::Client(const std::string& server_ip, int port) : server_ip_(server_ip), port_(port), connected_(false), socket_fd_(-1) {}

Client::~Client() {
  disconnect();
}

bool Client::connect() {
  socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd_ < 0) {
    std::cerr << "Error creating socket: "<< strerror(errno) << std::endl;
    return false;
  }

  memset(&server_addr_,0,sizeof(server_addr_));
  server_addr_.sin_family = AF_INET;
  server_addr_.sin_port = htons(port_);

  if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr) <= 0) {
    std::cerr << "Invalid address: " <<strerror(errno) << std::endl;
    close(socket_fd_);
    socket_fd_ = -1;
    return false;
  }

  const char* connect_msg = "CONNECT";
  if (sendto(socket_fd_, connect_msg, strlen(connect_msg), 0,
              (struct sockaddr*)&server_addr_, sizeof(server_addr_)) < 0) {
    std::cerr << "Error sending connection message: " << strerror(errno) << std::endl;
    close(socket_fd_);
    socket_fd_ = -1;
    return false;
              }
  connected_ = true;
  std::cout << "Client connected to " << server_ip_ << ":" << port_ <<std::endl;
  return true;


}

void Client::disconnect() {
  if (socket_fd_ >= 0) {
    // Send a disconnection message (optional)
    if (connected_) {
      const char* disconnect_msg = "DISCONNECT";
      sendto(socket_fd_, disconnect_msg, strlen(disconnect_msg), 0,
           (struct sockaddr*)&server_addr_, sizeof(server_addr_));
    }

    close(socket_fd_);
    socket_fd_ = -1;
    connected_ = false;
    std::cout << "Client disconnected" << std::endl;
  }
}


bool Client::is_connected() const {
  return connected_ && (socket_fd_ >= 0);
}

int Client::receive_data(void* buffer, size_t buffer_size) {
  if (!is_connected()) {
    std::cerr << "Cannot receive data: client not connected" << std::endl;
    return -1;
  }

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;

  if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    std::cerr << "Error setting socket timeout:" << strerror(errno) << std::endl;
  }

  sockaddr_in from_addr;
  socklen_t from_len = sizeof(from_addr);

  ssize_t bytes_received = recvfrom(socket_fd_, buffer, buffer_size, 0, (struct sockaddr*)&from_addr, &from_len);

  if (bytes_received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
    return -1;
  }
  return bytes_received;
}
