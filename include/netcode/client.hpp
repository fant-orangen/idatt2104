#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "netcode/visualization/player.hpp"
#include "netcode/packets/player_state_packet.hpp"
#include "raylib.h" // For Vector3

namespace netcode {

class Client {
public:
    Client(uint32_t clientId, int port, const std::string& serverIp = "127.0.0.1", int serverPort = 7000);
    ~Client();

    void start();
    void stop();
    
    // Send movement request to server
    void sendMovementRequest(const Vector3& movement, bool jumpRequested);
    
    // Set player references for client to update
    void setPlayerReference(uint32_t playerId, std::shared_ptr<visualization::Player> player);
    
    // Update player position from server update
    void updatePlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping);
    
    // Get client ID
    uint32_t getClientId() const { return clientId_; }

private:
    uint32_t clientId_;
    int port_;
    std::string serverIp_;
    int serverPort_;
    int socketFd_;
    sockaddr_in serverAddr_;
    
    std::atomic<bool> running_;
    std::thread clientThread_;
    
    // Map of player ID to player object
    std::mutex playerMutex_;
    std::unordered_map<uint32_t, std::shared_ptr<visualization::Player>> players_;
    
    // Network processing
    void processNetworkEvents();
    void handleServerUpdate(const packets::PlayerStatePacket& packet);
};

} // namespace netcode
