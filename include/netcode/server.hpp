#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <queue>
#include <unordered_map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "netcode/visualization/player.hpp"
#include "netcode/packets/player_state_packet.hpp"

namespace netcode {

class Server {
public:
    Server(int port = 7000);
    ~Server();

    void start();
    void stop();
    
    // Update server's player state based on client request
    void updatePlayerState(const packets::PlayerMovementRequest& request);
    
    // Set player references for server to update
    void setPlayerReference(uint32_t playerId, std::shared_ptr<visualization::Player> player);
    
    // Directly update a player position
    void setPlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping);

private:
    int port_;
    int socketFd_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    
    // Map of player ID to player object
    std::mutex playerMutex_;
    std::unordered_map<uint32_t, std::shared_ptr<visualization::Player>> players_;
    
    // Network processing
    void processNetworkEvents();
    void handleClientRequest(const sockaddr_in& clientAddr, const packets::PlayerMovementRequest& request);
    void broadcastPlayerState(uint32_t playerId, float x, float y, float z, bool isJumping);
    
    // Client addresses for responding
    std::unordered_map<uint32_t, sockaddr_in> clientAddresses_;
    
    // Queue for delayed packet processing
    std::queue<packets::TimestampedPlayerMovementRequest> packetQueue_;
    // Mutex for protecting packet queue access
    std::mutex queueMutex_;
};

} // namespace netcode
