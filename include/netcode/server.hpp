#pragma once

#include "netcode/math/my_vec3.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/packets/player_state_packet.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <memory>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <map>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace netcode {

/**
 * @brief Server class for handling network communication with game clients
 * 
 * This class manages UDP socket communication between the server and clients,
 * processing player movement requests and broadcasting state updates. It
 * maintains a thread for processing network events and tracks connected clients.
 */
class Server {
public:
    /**
     * @brief Construct a new Server object
     * 
     * @param port Port number to listen on (default: 7000)
     */
    explicit Server(int port = 7000);
    
    /**
     * @brief Destroy the Server object and clean up resources
     */
    ~Server();
    
    /**
     * @brief Start the server and begin listening for client connections
     * 
     * Creates a UDP socket, binds it to the specified port, and starts
     * the network processing thread.
     */
    void start();
    
    /**
     * @brief Stop the server and clean up resources
     * 
     * Stops the network processing thread and closes the socket.
     */
    void stop();
    
    /**
     * @brief Set a reference to a player entity for position updates
     * 
     * @param playerId ID of the player to set reference for
     * @param player Shared pointer to the networked entity
     */
    void setPlayerReference(uint32_t playerId, std::shared_ptr<NetworkedEntity> player);
    
    /**
     * @brief Update a player's state based on a movement request
     * 
     * @param request Player movement request packet
     */
    void updatePlayerState(const packets::PlayerMovementRequest& request);
    
    /**
     * @brief Set a player's position directly
     * 
     * @param playerId ID of the player to update
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param isJumping Whether the player is currently jumping
     */
    void setPlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping);
    
    /**
     * @brief Update all entities' render positions
     * 
     * Call this method in your game loop to update the render positions
     * of all server entities 
     * 
     * @param deltaTime Time elapsed since last update in seconds
     */
    void updateEntities(float deltaTime);
    
private:
    int port_;                 ///< Port number to listen on
    int socketFd_;             ///< UDP socket file descriptor
    std::atomic<bool> running_; ///< Flag indicating if server is running
    
    // Thread for network processing
    std::thread serverThread_; ///< Thread for processing network events
    
    // Mutex for protecting player map access
    std::mutex playerMutex_;
    
    // Map of player IDs to their networked entity objects
    std::map<uint32_t, std::shared_ptr<NetworkedEntity>> players_;
    
    // Map of player IDs to their last processed input sequence number
    std::map<uint32_t, uint32_t> lastProcessedInputSequence_;
    
    // Map of player IDs to their client addresses
    std::unordered_map<uint32_t, sockaddr_in> clientAddresses_;
    
    // Map of player IDs to their last broadcast time for throttling
    std::map<uint32_t, std::chrono::steady_clock::time_point> lastBroadcastTimes_;
    
    // Minimum interval between broadcasts (in milliseconds)
    static constexpr uint32_t MIN_BROADCAST_INTERVAL_MS = 16; // ~60 FPS
    
    // Queue for delayed packet processing
    std::queue<packets::TimestampedPlayerMovementRequest> packetQueue_;
    
    // Mutex for protecting packet queue access
    std::mutex queueMutex_;
    
    /**
     * @brief Process incoming network events continuously
     * 
     * Main network processing loop that receives and handles client requests
     */
    void processNetworkEvents();
    
    /**
     * @brief Handle a client request packet
     * 
     * @param clientAddr The client's address
     * @param request The player movement request packet
     */
    void handleClientRequest(const sockaddr_in& clientAddr, const packets::PlayerMovementRequest& request);
    
    /**
     * @brief Broadcast a player's state to all connected clients
     * 
     * @param playerId ID of the player whose state to broadcast
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param isJumping Whether the player is currently jumping
     * @param sequenceNumber The sequence number of the last processed input
     */
    void broadcastPlayerState(uint32_t playerId, float x, float y, float z, bool isJumping, uint32_t sequenceNumber);
};

} // namespace netcode
