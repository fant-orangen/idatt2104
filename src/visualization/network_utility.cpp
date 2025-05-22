#include "netcode/visualization/network_utility.hpp"
#include "netcode/utils/logger.hpp"
#include <chrono>
#include <thread>
#include "netcode/server.hpp"
#include "netcode/client.hpp"

namespace netcode {
namespace visualization {

NetworkUtility::NetworkUtility(Mode mode) : mode_(mode) {
    // Initialize networking components based on mode
    if (mode_ == Mode::STANDARD) {
        initializeNetworking();
    } else {
        // Start network processing thread for TEST mode
        networkThread_ = std::thread(&NetworkUtility::processNetworkEvents, this);
    }
}

NetworkUtility::~NetworkUtility() {
    running_ = false;
    if (networkThread_.joinable()) {
        networkThread_.join();
    }
    
    // Stop networking components
    if (server_) server_->stop();
    if (client1_) client1_->stop();
    if (client2_) client2_->stop();
}

void NetworkUtility::initializeNetworking() {
    LOG_INFO("Initializing networking in STANDARD mode", "NetworkUtility");
    
    // Create server on port 7000
    server_ = std::make_unique<Server>(SERVER_PORT);
    
    // Create client 1 on port 7001
    client1_ = std::make_unique<Client>(CLIENT1_PLAYER_ID, CLIENT1_PORT);
    
    // Create client 2 on port 7002
    client2_ = std::make_unique<Client>(CLIENT2_PLAYER_ID, CLIENT2_PORT);
    
    // Start server and clients
    server_->start();
    client1_->start();
    client2_->start();
}

void NetworkUtility::clientToServerUpdate(std::shared_ptr<Player> clientPlayer,
                                        std::shared_ptr<Player> serverPlayer,
                                        const Vector3& movement,
                                        bool jumpRequested) {
    if (mode_ == Mode::TEST) {
        // Queue the input event for processing by network thread
        std::lock_guard<std::mutex> lock(queueMutex_);
        inputQueue_.push({movement, jumpRequested, clientPlayer, serverPlayer});
    } 
    else if (mode_ == Mode::STANDARD) {
        // Store player references for access in other methods
        uint32_t playerId = clientPlayer->getId();
        
        if (playerId == CLIENT1_PLAYER_ID && client1_) {
            // Store references for future use
            serverPlayerRef_ = serverPlayer;
            client1PlayerRef_ = clientPlayer;
            
            // Send movement request from client1 to server
            // Convert Vector3 to MyVec3 for the network layer
            client1_->sendMovementRequest(toMyVec3(movement), jumpRequested);
        }
        else if (playerId == CLIENT2_PLAYER_ID && client2_) {
            // Store references for future use
            serverPlayerRef_ = serverPlayer;
            client2PlayerRef_ = clientPlayer;
            
            // Send movement request from client2 to server
            // Convert Vector3 to MyVec3 for the network layer
            client2_->sendMovementRequest(toMyVec3(movement), jumpRequested);
        }
    }
}

void NetworkUtility::serverToClientsUpdate(std::shared_ptr<Player> serverPlayer,
                                         std::shared_ptr<Player> client1Player,
                                         std::shared_ptr<Player> client2Player) {
    if (mode_ == Mode::TEST) {
        auto updateTime = std::chrono::steady_clock::now() + CLIENT_DELAY;
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (client1Player) {
            // Convert MyVec3 to Vector3 when getting position from Player
            client1Updates_.push({updateTime, toVector3(serverPlayer->getPosition()), client1Player});
        }
        if (client2Player) {
            // Convert MyVec3 to Vector3 when getting position from Player
            client2Updates_.push({updateTime, toVector3(serverPlayer->getPosition()), client2Player});
        }
    }
    else if (mode_ == Mode::STANDARD) {
        // Setup player references for each component
        uint32_t playerId = serverPlayer->getId();
        
        // Store references for network components
        if (server_) {
            server_->setPlayerReference(playerId, serverPlayer);
        }
        
        if (client1_ && client1Player) {
            client1_->setPlayerReference(playerId, client1Player);
        }
        
        if (client2_ && client2Player) {
            client2_->setPlayerReference(playerId, client2Player);
        }
    }
}

void NetworkUtility::processNetworkEvents() {
    if (mode_ == Mode::TEST) {
        while (running_) {
            // Process input queue
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                while (!inputQueue_.empty()) {
                    auto& event = inputQueue_.front();
                    
                    // Simulate network delay
                    std::this_thread::sleep_for(SERVER_DELAY);
                    
                    if (event.serverPlayer) {
                        // Convert Vector3 to MyVec3 when calling move on Player
                        event.serverPlayer->move(toMyVec3(event.movement));
                        if (event.jumpRequested) {
                            event.serverPlayer->jump();
                        }
                        event.serverPlayer->update();
                    }
                    
                    inputQueue_.pop();
                }
            }
            
            // Small sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    // STANDARD mode uses real network events on dedicated threads
}

void NetworkUtility::update() {
    if (mode_ == Mode::TEST) {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        // Process client 1 updates
        while (!client1Updates_.empty() && now >= client1Updates_.front().updateTime) {
            auto& update = client1Updates_.front();
            // Convert Vector3 to MyVec3 when setting position on Player
            update.player->setPosition(toMyVec3(update.position));
            update.player->update();
            client1Updates_.pop();
        }
        
        // Process client 2 updates
        while (!client2Updates_.empty() && now >= client2Updates_.front().updateTime) {
            auto& update = client2Updates_.front();
            // Convert Vector3 to MyVec3 when setting position on Player
            update.player->setPosition(toMyVec3(update.position));
            update.player->update();
            client2Updates_.pop();
        }
    }
    // STANDARD mode updates are handled by network threads
}

void NetworkUtility::updatePlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping) {
    // This method is called by network components when they receive position updates
    LOG_DEBUG("Updating player " + std::to_string(playerId) + " position from network", "NetworkUtility");
}

void NetworkUtility::sendPlayerStateToServer(uint32_t playerId, const Vector3& position, bool isJumping, Client* client) {
    if (client) {
        // Send movement request based on current position
        Vector3 movement = {0.0f, 0.0f, 0.0f}; // Just sending current position
        // Convert Vector3 to MyVec3 when sending to client
        client->sendMovementRequest(toMyVec3(movement), isJumping);
    }
}

}} // namespace netcode::visualization