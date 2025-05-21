#include "netcode/client.hpp"
#include "netcode/utils/logger.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>

namespace netcode {

Client::Client(uint32_t clientId, int port, const std::string& serverIp, int serverPort) 
    : clientId_(clientId), port_(port), serverIp_(serverIp), serverPort_(serverPort), socketFd_(-1), running_(false) {
    LOG_INFO("Client " + std::to_string(clientId_) + " created on port " + std::to_string(port_), "Client");
    
    // Setup server address
    memset(&serverAddr_, 0, sizeof(serverAddr_));
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(serverPort_);
    inet_pton(AF_INET, serverIp_.c_str(), &serverAddr_.sin_addr);
}

Client::~Client() {
    stop();
}

void Client::start() {
    if (running_) {
        LOG_WARNING("Client already running", "Client");
        return;
    }
    
    // Create UDP socket
    socketFd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd_ < 0) {
        LOG_ERROR("Failed to create socket: " + std::string(strerror(errno)), "Client");
        return;
    }
    
    // Set non-blocking mode
    int flags = fcntl(socketFd_, F_GETFL, 0);
    fcntl(socketFd_, F_SETFL, flags | O_NONBLOCK);
    
    // Setup client address for binding
    sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(port_);
    
    // Bind socket to address
    if (bind(socketFd_, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
        LOG_ERROR("Failed to bind socket: " + std::string(strerror(errno)), "Client");
        close(socketFd_);
        return;
    }
    
    LOG_INFO("Client " + std::to_string(clientId_) + " started on port " + std::to_string(port_), "Client");
    
    // Start network processing thread
    running_ = true;
    clientThread_ = std::thread(&Client::processNetworkEvents, this);
}

void Client::stop() {
    if (running_) {
        running_ = false;
        if (clientThread_.joinable()) {
            clientThread_.join();
        }
        
        if (socketFd_ != -1) {
            close(socketFd_);
            socketFd_ = -1;
        }
        
        LOG_INFO("Client " + std::to_string(clientId_) + " stopped", "Client");
    }
}

void Client::setPlayerReference(uint32_t playerId, std::shared_ptr<visualization::Player> player) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    players_[playerId] = player;
    LOG_INFO("Client " + std::to_string(clientId_) + " set player reference for ID: " + std::to_string(playerId), "Client");
}

void Client::sendMovementRequest(const Vector3& movement, bool jumpRequested) {
    packets::PlayerMovementRequest request;
    
    // Fill in request data
    request.player_id = clientId_;
    request.movement_x = movement.x;
    request.movement_y = movement.y;
    request.movement_z = movement.z;
    request.velocity_y = 0.0f; // Could set this if needed
    request.is_jumping = jumpRequested;
    
    // Send request to server
    ssize_t bytesSent = sendto(socketFd_, &request, sizeof(request), 0,
                            (struct sockaddr*)&serverAddr_, sizeof(serverAddr_));
                            
    if (bytesSent < 0) {
        LOG_ERROR("Failed to send movement request: " + std::string(strerror(errno)), "Client");
    } else {
        LOG_DEBUG("Client " + std::to_string(clientId_) + " sent movement request: [" + 
                  std::to_string(movement.x) + ", " + std::to_string(movement.y) + 
                  ", " + std::to_string(movement.z) + "], jump: " + 
                  (jumpRequested ? "true" : "false"), "Client");
    }
}

void Client::updatePlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        LOG_WARNING("Client " + std::to_string(clientId_) + 
                    " trying to update unknown player ID: " + std::to_string(playerId), "Client");
        return;
    }
    
    // Update player position
    it->second->setPosition({x, y, z});
    if (isJumping) {
        it->second->jump();
    }
    it->second->update();
    
    LOG_DEBUG("Client " + std::to_string(clientId_) + " updated player " + 
              std::to_string(playerId) + " position: [" + std::to_string(x) + 
              ", " + std::to_string(y) + ", " + std::to_string(z) + "]", "Client");
}

void Client::processNetworkEvents() {
    sockaddr_in serverAddr;
    socklen_t serverLen = sizeof(serverAddr);
    constexpr size_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    
    while (running_) {
        // Receive data from server
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytesReceived = recvfrom(socketFd_, buffer, BUFFER_SIZE, 0,
                                     (struct sockaddr*)&serverAddr, &serverLen);
                                     
        if (bytesReceived > 0) {
            // Assuming the received data is a PlayerStatePacket
            if (bytesReceived >= sizeof(packets::PlayerStatePacket)) {
                packets::PlayerStatePacket packet;
                memcpy(&packet, buffer, sizeof(packets::PlayerStatePacket));
                
                // Process server update
                handleServerUpdate(packet);
            }
        } else if (bytesReceived < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("recvfrom failed: " + std::string(strerror(errno)), "Client");
        }
        
        // Sleep to prevent high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Client::handleServerUpdate(const packets::PlayerStatePacket& packet) {
    // Update player position based on server packet
    updatePlayerPosition(packet.player_id, packet.x, packet.y, packet.z, packet.is_jumping);
}

} // namespace netcode
