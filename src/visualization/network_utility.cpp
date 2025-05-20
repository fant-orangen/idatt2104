#include "netcode/visualization/network_utility.hpp"
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include "netcode/utils/logger.hpp"
#include "netcode/serialization.hpp"
#include "netcode/serialization/player_state_serializer.hpp"

namespace netcode {
namespace visualization {

// TODO: Add UDPClient and UDPServer classes as fields. Call UDPClient to send packets and provide function to receive packets.

NetworkUtility::NetworkUtility(Mode mode) : mode_(mode) {
    // Initialize networking if in STANDARD mode
    if (mode_ == Mode::STANDARD) {
        initializeNetworking();
    }
    
    // Start network processing thread
    networkThread_ = std::thread(&NetworkUtility::processNetworkEvents, this);
}

NetworkUtility::~NetworkUtility() {
    running_ = false;
    if (networkThread_.joinable()) {
        networkThread_.join();
    }
}

void NetworkUtility::initializeNetworking() {
    // Initialize ONE server and TWO clients
    server_ = std::make_unique<Server>(SERVER_PORT);
    client1_ = std::make_unique<Client>("127.0.0.1", CLIENT1_PORT);
    client2_ = std::make_unique<Client>("127.0.0.1", CLIENT2_PORT);
    
    // Start server
    bool serverStarted = server_->start();
    
    // Connect clients to server
    bool client1Connected = client1_->connect_to_server();
    bool client2Connected = client2_->connect_to_server();
    
    if (!serverStarted || !client1Connected || !client2Connected) {
        utils::Logger::get_instance().LOG_ERROR("Failed to start network components", "NetworkUtility");
    } else {
        utils::Logger::get_instance().LOG_INFO("Network components started successfully", "NetworkUtility");
    }
    
    // Set up server callback for player updates
    server_->set_player_update_callback([this](const packets::PlayerStatePacket& packet) {
        // Server received a player state update
        // Update the server's player representation
        this->updatePlayerPosition(
            SERVER_PLAYER_ID, 
            packet.x, 
            packet.y, 
            packet.z, 
            packet.is_jumping
        );
        
        utils::Logger::get_instance().LOG_DEBUG("Server processed player " + 
            std::to_string(packet.player_id) + " update", "NetworkUtility");
    });
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
        // Only send update if there's actual movement or a jump request
        if (movement.x == 0 && movement.y == 0 && movement.z == 0 && !jumpRequested) {
            return;
        }

        // Store player references for future updates
        uint32_t playerId = 0;
        Client* client = nullptr;
        
        // Determine which client is sending the update
        if (clientPlayer == client1PlayerRef_) {
            playerId = CLIENT1_PLAYER_ID;
            client = client1_.get();
            serverPlayerRef_ = serverPlayer;
        } 
        else if (clientPlayer == client2PlayerRef_) {
            playerId = CLIENT2_PLAYER_ID;
            client = client2_.get();
            serverPlayerRef_ = serverPlayer;
        }
        else {
            utils::Logger::get_instance().LOG_WARNING("Unknown client player in clientToServerUpdate", "NetworkUtility");
            return;
        }
        
        // Get current position and add the movement
        Vector3 position = clientPlayer->getPosition();
        float newX = position.x + movement.x;
        float newY = position.y + movement.y;
        float newZ = position.z + movement.z;
        
        // Send the state to the server
        if (client && client->is_connected()) {
            sendPlayerStateToServer(playerId, {newX, newY, newZ}, jumpRequested, client);
        }
    }
}

void NetworkUtility::sendPlayerStateToServer(uint32_t playerId, const Vector3& position, bool isJumping, Client* client) {
    // Create the packet
    packets::PlayerStatePacket state;
    state.player_id = playerId;
    state.x = position.x;
    state.y = position.y;
    state.z = position.z;
    state.velocity_y = 0.0f; // We don't have velocity info
    state.is_jumping = isJumping;
    
    // Serialize the packet
    Buffer buffer;
    PacketHeader header;
    header.type = MessageType::PLAYER_STATE_UPDATE;
    header.sequenceNumber = 0; // Sequence number not currently used
    
    serialize(buffer, header);
    serialization::serialize(buffer, state);
    
    // Send the packet to the server
    if (client->send_packet(buffer)) {
        utils::Logger::get_instance().LOG_DEBUG("Sent player state to server: player_id=" + 
            std::to_string(playerId) + ", pos=(" + 
            std::to_string(position.x) + "," + 
            std::to_string(position.y) + "," + 
            std::to_string(position.z) + ")", "NetworkUtility");
    } else {
        utils::Logger::get_instance().LOG_WARNING("Failed to send player state to server", "NetworkUtility");
    }
}

void NetworkUtility::serverToClientsUpdate(std::shared_ptr<Player> serverPlayer,
                                         std::shared_ptr<Player> client1Player,
                                         std::shared_ptr<Player> client2Player) {
    if (mode_ == Mode::TEST) {
        auto updateTime = std::chrono::steady_clock::now() + CLIENT_DELAY;
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (client1Player) {
            client1Updates_.push({updateTime, serverPlayer->getPosition(), client1Player});
        }
        if (client2Player) {
            client2Updates_.push({updateTime, serverPlayer->getPosition(), client2Player});
        }
    }
    else if (mode_ == Mode::STANDARD) {
        // Store player references for future updates
        if (serverPlayer) {
            serverPlayerRef_ = serverPlayer;
            utils::Logger::get_instance().LOG_DEBUG("Server player reference updated", "NetworkUtility");
        }
        
        // Check if this is a red player update (based on player ID)
        if (client1Player && client1Player->getId() == 1) {
            client1PlayerRef_ = client1Player;
            utils::Logger::get_instance().LOG_DEBUG("Client 1 (red) player reference updated", "NetworkUtility");
        }
        
        // Check if this is a blue player update (based on player ID)
        if (client2Player && client2Player->getId() == 2) {
            client2PlayerRef_ = client2Player;
            utils::Logger::get_instance().LOG_DEBUG("Client 2 (blue) player reference updated", "NetworkUtility");
        }
        
        // In STANDARD mode, updates are sent via the UDP network when clients make requests
        // The server broadcasts updates to all clients
    }
}

void NetworkUtility::updatePlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping) {
    std::shared_ptr<Player> targetPlayer = nullptr;
    
    // Determine which player to update
    if (playerId == SERVER_PLAYER_ID && serverPlayerRef_) {
        targetPlayer = serverPlayerRef_;
    }
    else if (playerId == CLIENT1_PLAYER_ID && client1PlayerRef_) {
        targetPlayer = client1PlayerRef_;
    }
    else if (playerId == CLIENT2_PLAYER_ID && client2PlayerRef_) {
        targetPlayer = client2PlayerRef_;
    }
    
    // Update the player if found
    if (targetPlayer) {
        Vector3 position = {x, y, z};
        targetPlayer->setPosition(position);
        if (isJumping) {
            targetPlayer->jump();
        }
        targetPlayer->update();
        
        utils::Logger::get_instance().LOG_DEBUG("Updated player " + std::to_string(playerId) + 
            " position to " + std::to_string(x) + "," + 
            std::to_string(y) + "," + std::to_string(z), "NetworkUtility");
    }
    else {
        utils::Logger::get_instance().LOG_WARNING("Player ID " + std::to_string(playerId) + 
            " not found for update", "NetworkUtility");
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
                        event.serverPlayer->move(event.movement);
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
    else if (mode_ == Mode::STANDARD) {
        Buffer buffer;
        
        while (running_) {
            // Process server packets in the server's listener thread
            
            // Check for client1 packets from server
            if (client1_ && client1_->is_connected()) {
                int received = client1_->receive_packet(buffer, 1024);
                if (received > 0) {
                    // Process packet
                    if (buffer.get_size() >= sizeof(PacketHeader)) {
                        PacketHeader header = buffer.read_header();
                        if (header.type == MessageType::PLAYER_STATE_UPDATE) {
                            packets::PlayerStatePacket packet;
                            if (serialization::deserialize(buffer, packet)) {
                                // Update client1's player position
                                updatePlayerPosition(
                                    packet.player_id, 
                                    packet.x, 
                                    packet.y, 
                                    packet.z, 
                                    packet.is_jumping
                                );
                            }
                        }
                    }
                }
            }
            
            // Check for client2 packets from server
            if (client2_ && client2_->is_connected()) {
                int received = client2_->receive_packet(buffer, 1024);
                if (received > 0) {
                    // Process packet
                    if (buffer.get_size() >= sizeof(PacketHeader)) {
                        PacketHeader header = buffer.read_header();
                        if (header.type == MessageType::PLAYER_STATE_UPDATE) {
                            packets::PlayerStatePacket packet;
                            if (serialization::deserialize(buffer, packet)) {
                                // Update client2's player position
                                updatePlayerPosition(
                                    packet.player_id, 
                                    packet.x, 
                                    packet.y, 
                                    packet.z, 
                                    packet.is_jumping
                                );
                            }
                        }
                    }
                }
            }
            
            // Small sleep to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void NetworkUtility::update() {
    if (mode_ == Mode::TEST) {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        // Process client 1 updates
        while (!client1Updates_.empty() && now >= client1Updates_.front().updateTime) {
            auto& update = client1Updates_.front();
            update.player->setPosition(update.position);
            update.player->update();
            client1Updates_.pop();
        }
        
        // Process client 2 updates
        while (!client2Updates_.empty() && now >= client2Updates_.front().updateTime) {
            auto& update = client2Updates_.front();
            update.player->setPosition(update.position);
            update.player->update();
            client2Updates_.pop();
        }
    }
    // In STANDARD mode, updates are handled through the network thread
}

}} // namespace netcode::visualization 