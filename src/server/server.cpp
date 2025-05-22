#include "netcode/server.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/visualization/settings.hpp"
#include "netcode/networked_entity.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <queue>

namespace netcode {

Server::Server(int port) : port_(port), socketFd_(-1), running_(false) {
    LOG_INFO("Server created on port " + std::to_string(port_), "Server");
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (running_) {
        LOG_WARNING("Server already running", "Server");
        return;
    }
    
    // Create UDP socket
    socketFd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd_ < 0) {
        LOG_ERROR("Failed to create socket: " + std::string(strerror(errno)), "Server");
        return;
    }
    
    // Set non-blocking mode
    int flags = fcntl(socketFd_, F_GETFL, 0);
    fcntl(socketFd_, F_SETFL, flags | O_NONBLOCK);
    
    // Setup server address
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    // Bind socket to address
    if (bind(socketFd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        LOG_ERROR("Failed to bind socket: " + std::string(strerror(errno)), "Server");
        close(socketFd_);
        return;
    }
    
    LOG_INFO("Server started on port " + std::to_string(port_), "Server");
    
    // Start network processing thread
    running_ = true;
    serverThread_ = std::thread(&Server::processNetworkEvents, this);
}

void Server::stop() {
    if (running_) {
        running_ = false;
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
        
        if (socketFd_ != -1) {
            close(socketFd_);
            socketFd_ = -1;
        }
        
        LOG_INFO("Server stopped", "Server");
    }
}

void Server::setPlayerReference(uint32_t playerId, std::shared_ptr<NetworkedEntity> player) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    players_[playerId] = player;
    LOG_INFO("Set player reference for ID: " + std::to_string(playerId), "Server");
}

void Server::updatePlayerState(const packets::PlayerMovementRequest& request) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    
    auto it = players_.find(request.player_id);
    if (it == players_.end()) {
        LOG_WARNING("Received update for unknown player ID: " + std::to_string(request.player_id), "Server");
        return;
    }
    
    auto player = it->second;
    
    // Create a normalized movement vector
    netcode::math::MyVec3 movement = {
        request.movement_x,
        request.movement_y,
        request.movement_z
    };
    
    // Update player position
    player->move(movement);
    if (request.is_jumping) {
        player->jump();
    }
    player->update();
    
    // Get updated position and broadcast to all clients
    auto pos = player->getPosition();
    broadcastPlayerState(request.player_id, pos.x, pos.y, pos.z, request.is_jumping);
    
    LOG_DEBUG("Updated player " + std::to_string(request.player_id) + 
              " position: [" + std::to_string(pos.x) + ", " + 
              std::to_string(pos.y) + ", " + std::to_string(pos.z) + "]", "Server");
}

void Server::setPlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        LOG_WARNING("Trying to set position for unknown player ID: " + std::to_string(playerId), "Server");
        return;
    }
    
    // Update player position
    it->second->setPosition({x, y, z});
    if (isJumping) {
        it->second->jump();
    }
    it->second->update();
    
    // Broadcast updated state to clients
    broadcastPlayerState(playerId, x, y, z, isJumping);
}

void Server::processNetworkEvents() {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    constexpr size_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    
    while (running_) {
        // Process any queued packets that are ready
        auto currentTime = std::chrono::steady_clock::now();
        std::queue<packets::TimestampedPlayerMovementRequest> remainingPackets;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            while (!packetQueue_.empty()) {
                auto& timestampedRequest = packetQueue_.front();
                if (currentTime >= timestampedRequest.timestamp) {
                    // Only add new client addresses, never update existing ones
                    uint32_t playerId = timestampedRequest.player_movement_request.player_id;
                    if (clientAddresses_.find(playerId) == clientAddresses_.end()) {
                        clientAddresses_[playerId] = clientAddr;
                        LOG_INFO("Registered new client with ID: " + std::to_string(playerId), "Server");
                    }
                    
                    // Process client request with the stored client address
                    handleClientRequest(clientAddresses_[playerId], timestampedRequest.player_movement_request);
                    
                    // For initial registration (zero movement), send immediate position update
                    auto& request = timestampedRequest.player_movement_request;
                    if (request.movement_x == 0.0f && request.movement_y == 0.0f && 
                        request.movement_z == 0.0f && !request.is_jumping) {
                        auto it = players_.find(request.player_id);
                        if (it != players_.end()) {
                            auto pos = it->second->getPosition();
                            broadcastPlayerState(request.player_id, pos.x, pos.y, pos.z, false);
                        }
                    }
                    
                    packetQueue_.pop();
                } else {
                    remainingPackets.push(timestampedRequest);
                    packetQueue_.pop();
                }
            }
            packetQueue_ = std::move(remainingPackets);
        }

        // Receive new data from clients
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytesReceived = recvfrom(socketFd_, buffer, BUFFER_SIZE, 0,
                                     (struct sockaddr*)&clientAddr, &clientLen);
                                     
        if (bytesReceived > 0) {
            if (bytesReceived >= sizeof(packets::TimestampedPlayerMovementRequest)) {
                packets::TimestampedPlayerMovementRequest timestampedRequest;
                memcpy(&timestampedRequest, buffer, sizeof(timestampedRequest));
                
                // Store client address immediately for registration packets (only if not already registered)
                if (timestampedRequest.player_movement_request.movement_x == 0.0f &&
                    timestampedRequest.player_movement_request.movement_y == 0.0f &&
                    timestampedRequest.player_movement_request.movement_z == 0.0f &&
                    !timestampedRequest.player_movement_request.is_jumping) {
                    uint32_t playerId = timestampedRequest.player_movement_request.player_id;
                    if (clientAddresses_.find(playerId) == clientAddresses_.end()) {
                        clientAddresses_[playerId] = clientAddr;
                        LOG_INFO("Registered new client with ID: " + std::to_string(playerId), "Server");
                    }
                }
                
                std::lock_guard<std::mutex> lock(queueMutex_);
                packetQueue_.push(timestampedRequest);
            }
        } else if (bytesReceived < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("recvfrom failed: " + std::string(strerror(errno)), "Server");
        }
        
        // Sleep to prevent high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Server::handleClientRequest(const sockaddr_in& clientAddr, const packets::PlayerMovementRequest& request) {
    // Process player movement request
    updatePlayerState(request);
}

void Server::broadcastPlayerState(uint32_t playerId, float x, float y, float z, bool isJumping) {
    packets::PlayerStatePacket packet;
    packet.player_id = playerId;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.velocity_y = 0.0f;
    packet.is_jumping = isJumping;
    
    // Create timestamped packet
    packets::TimestampedPlayerStatePacket timestampedPacket;
    timestampedPacket.timestamp = std::chrono::steady_clock::now() + 
        std::chrono::milliseconds(visualization::settings::SERVER_TO_CLIENT_DELAY);
    timestampedPacket.player_state = packet;
    
    // Send update to all known clients
    for (const auto& client : clientAddresses_) {
        sendto(socketFd_, &timestampedPacket, sizeof(timestampedPacket), 0,
               (struct sockaddr*)&client.second, sizeof(client.second));
    }
    
    LOG_DEBUG("Broadcast player " + std::to_string(playerId) + " state to " + 
              std::to_string(clientAddresses_.size()) + " clients", "Server");
}

} // namespace netcode
