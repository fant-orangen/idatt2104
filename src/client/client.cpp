#include "netcode/client.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/visualization/settings.hpp" // TODO: Nothing from visualization should be here
#include "netcode/networked_entity.hpp"
#include "netcode/prediction.hpp"
#include "netcode/reconciliation.hpp"
#include "netcode/interpolation.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <queue>

namespace netcode {

Client::Client(uint32_t clientId, int port, const std::string& serverIp, int serverPort) 
    : clientId_(clientId), port_(port), serverIp_(serverIp), serverPort_(serverPort), socketFd_(-1), running_(false) {
    LOG_INFO("Client " + std::to_string(clientId_) + " created on port " + std::to_string(port_), "Client");
    
    // Setup server address
    memset(&serverAddr_, 0, sizeof(serverAddr_));
    serverAddr_.sin_family = AF_INET;
    serverAddr_.sin_port = htons(serverPort_);
    inet_pton(AF_INET, serverIp_.c_str(), &serverAddr_.sin_addr);
    
    // Initialize netcode systems
    snapshotManager_ = std::make_unique<SnapshotManager>();
    predictionSystem_ = std::make_unique<PredictionSystem>(*snapshotManager_);
    reconciliationSystem_ = std::make_unique<ReconciliationSystem>(*predictionSystem_);
    
    // Create interpolation system with appropriate settings
    InterpolationConfig interpolationConfig;
    interpolationConfig.interpolationDelay = 50; // ms, can be tuned
    interpolationConfig.maxInterpolationDistance = 3.0f; // Set a reasonable threshold for snapping
    interpolationSystem_ = std::make_unique<InterpolationSystem>(*snapshotManager_, interpolationConfig);
    
    // Configure reconciliation
    reconciliationSystem_->setReconciliationThreshold(0.5f);
    
    // Set up reconciliation callback for debugging
    reconciliationSystem_->setReconciliationCallback(
        [this](uint32_t entityId, const netcode::math::MyVec3& serverPos, const netcode::math::MyVec3& clientPos) {
            float distance = Magnitude(serverPos - clientPos);
            LOG_INFO("Reconciliation occurred for entity " + std::to_string(entityId) + 
                     " (diff: " + std::to_string(distance) + ")", "Client");
        }
    );
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
    
    // Send initial registration packet to server
    packets::PlayerMovementRequest initialRequest;
    initialRequest.player_id = clientId_;
    initialRequest.movement_x = 0.0f;
    initialRequest.movement_y = 0.0f;
    initialRequest.movement_z = 0.0f;
    initialRequest.velocity_y = 0.0f;
    initialRequest.is_jumping = false;
    initialRequest.input_sequence_number = 0; // Initial sequence
    
    // Create timestamped request with immediate processing
    packets::TimestampedPlayerMovementRequest timestampedRequest;
    timestampedRequest.timestamp = std::chrono::steady_clock::now(); // Process immediately
    timestampedRequest.player_movement_request = initialRequest;
    
    // Send registration request to server
    ssize_t bytesSent = sendto(socketFd_, &timestampedRequest, sizeof(timestampedRequest), 0,
                            (struct sockaddr*)&serverAddr_, sizeof(serverAddr_));
                            
    if (bytesSent < 0) {
        LOG_ERROR("Failed to send initial registration: " + std::string(strerror(errno)), "Client");
    } else {
        LOG_INFO("Client " + std::to_string(clientId_) + " sent initial registration to server", "Client");
    }
    
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

void Client::setPlayerReference(uint32_t playerId, std::shared_ptr<NetworkedEntity> player) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    players_[playerId] = player;
    
    // Register the entity with the snapshot manager for reconciliation
    snapshotManager_->registerEntity(playerId, player);
    
    LOG_INFO("Client " + std::to_string(clientId_) + " set player reference for ID: " + std::to_string(playerId), "Client");
}

void Client::updateEntities(float deltaTime) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    
    // Prune old snapshots to prevent memory buildup (keep 2 seconds of history)
    snapshotManager_->pruneOldSnapshots(2000);
    
    // Update reconciliation system for smooth corrections
    reconciliationSystem_->update(deltaTime);
    
    // Update all entities
    for (auto& [playerId, player] : players_) {
        // Update render positions for all entities including local player
        player->updateRenderPosition(deltaTime);
        
        // Only update remote players with interpolation for simulation state
        if (playerId != clientId_ && visualization::settings::ENABLE_INTERPOLATION) {
            interpolationSystem_->updateEntity(player, deltaTime);
        }
    }
}

void Client::sendMovementRequest(const netcode::math::MyVec3& movement, bool jumpRequested) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    
    // Get local player reference
    auto it = players_.find(clientId_);
    if (it == players_.end()) {
        LOG_WARNING("No local player found for client ID: " + std::to_string(clientId_), "Client");
        return;
    }
    
    uint32_t sequenceNumber = 0;
    
    // Apply prediction to local player immediately only if prediction is enabled
    if (visualization::settings::ENABLE_PREDICTION) {
        sequenceNumber = predictionSystem_->applyInputPrediction(it->second, movement, jumpRequested);
    } else {
        // If prediction is disabled, just get the next sequence number without applying prediction
        sequenceNumber = predictionSystem_->getNextSequenceNumber();
    }
    
    // Fill in request data with the sequence number
    packets::PlayerMovementRequest request;
    request.player_id = clientId_;
    request.movement_x = movement.x;
    request.movement_y = movement.y;
    request.movement_z = movement.z;
    request.velocity_y = 0.0f;
    request.is_jumping = jumpRequested;
    request.input_sequence_number = sequenceNumber; // Include the sequence number
    
    // Create timestamped request
    packets::TimestampedPlayerMovementRequest timestampedRequest;
    timestampedRequest.timestamp = std::chrono::steady_clock::now() + 
        std::chrono::milliseconds(visualization::settings::CLIENT_TO_SERVER_DELAY);
    timestampedRequest.player_movement_request = request;
    
    // Send request to server
    ssize_t bytesSent = sendto(socketFd_, &timestampedRequest, sizeof(timestampedRequest), 0,
                            (struct sockaddr*)&serverAddr_, sizeof(serverAddr_));
                            
    if (bytesSent < 0) {
        LOG_ERROR("Failed to send movement request: " + std::string(strerror(errno)), "Client");
    } else {
        LOG_DEBUG("Client " + std::to_string(clientId_) + " sent movement request: [" + 
                  std::to_string(movement.x) + ", " + std::to_string(movement.y) + 
                  ", " + std::to_string(movement.z) + "], jump: " + 
                  (jumpRequested ? "true" : "false") + ", seq: " + 
                  std::to_string(sequenceNumber), "Client");
    }
}

void Client::updatePlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping, uint32_t serverSequence) {
    std::lock_guard<std::mutex> lock(playerMutex_);
    
    auto it = players_.find(playerId);
    if (it == players_.end()) {
        LOG_WARNING("Client " + std::to_string(clientId_) + 
                    " trying to update unknown player ID: " + std::to_string(playerId), "Client");
        return;
    }
    
    netcode::math::MyVec3 serverPosition(x, y, z);
    auto serverTimestamp = std::chrono::steady_clock::now(); // This should ideally come from server
    
    if (playerId == clientId_) {
        // For local player
        if (visualization::settings::ENABLE_PREDICTION) {
            // Apply reconciliation with the server's sequence number only if prediction is enabled
            reconciliationSystem_->reconcileState(
                it->second, 
                serverPosition, 
                serverSequence, 
                serverTimestamp
            );
        } else {
            // If prediction is disabled, directly update the position
            it->second->setPosition(serverPosition);
        }
    } else {
        // For remote players
        if (visualization::settings::ENABLE_INTERPOLATION) {
            // Record the position for interpolation only if interpolation is enabled
            interpolationSystem_->recordEntityPosition(
                playerId,
                serverPosition,
                serverTimestamp
            );
            // Note: Don't directly set position here, let interpolation handle it
            // The interpolationSystem will set positions during updateEntities() calls
        } else {
            // If interpolation is disabled, directly update the position
            it->second->setPosition(serverPosition);
        }
    }
    
    LOG_DEBUG("Client " + std::to_string(clientId_) + " received update for player " + 
              std::to_string(playerId) + " position: [" + std::to_string(x) + 
              ", " + std::to_string(y) + ", " + std::to_string(z) + "]" +
              ", seq: " + std::to_string(serverSequence), "Client");
}

void Client::processNetworkEvents() {
    sockaddr_in serverAddr;
    socklen_t serverLen = sizeof(serverAddr);
    constexpr size_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    
    while (running_) {
        // Process any queued packets that are ready
        auto currentTime = std::chrono::steady_clock::now();
        std::queue<packets::TimestampedPlayerStatePacket> remainingPackets;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            while (!packetQueue_.empty()) {
                auto& timestampedPacket = packetQueue_.front();
                if (currentTime >= timestampedPacket.timestamp) {
                    handleServerUpdate(timestampedPacket.player_state);
                    packetQueue_.pop();
                } else {
                    remainingPackets.push(timestampedPacket);
                    packetQueue_.pop();
                }
            }
            packetQueue_ = std::move(remainingPackets);
        }

        // Receive new data from server
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytesReceived = recvfrom(socketFd_, buffer, BUFFER_SIZE, 0,
                                     (struct sockaddr*)&serverAddr, &serverLen);
                                     
        if (bytesReceived > 0) {
            if (bytesReceived >= sizeof(packets::TimestampedPlayerStatePacket)) {
                packets::TimestampedPlayerStatePacket timestampedPacket;
                memcpy(&timestampedPacket, buffer, sizeof(timestampedPacket));
                
                std::lock_guard<std::mutex> lock(queueMutex_);
                packetQueue_.push(timestampedPacket);
            }
        } else if (bytesReceived < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("recvfrom failed: " + std::string(strerror(errno)), "Client");
        }
        
        // Sleep to prevent high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Client::handleServerUpdate(const packets::PlayerStatePacket& packet) {
    // Update player position based on server packet, including sequence number
    updatePlayerPosition(
        packet.player_id, 
        packet.x, 
        packet.y, 
        packet.z, 
        packet.is_jumping,
        packet.last_processed_input_sequence // Use the server's sequence number
    );
}

} // namespace netcode
