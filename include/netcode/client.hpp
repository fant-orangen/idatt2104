#pragma once

#include "netcode/math/my_vec3.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/snapshot.hpp"
#include "netcode/prediction.hpp"
#include "netcode/reconciliation.hpp"
#include "netcode/interpolation.hpp"
#include "netcode/packets/player_state_packet.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <queue>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace netcode {

/**
 * @brief Client class for handling network communication with the game server
 * 
 * This class manages UDP socket communication between the client and server,
 * handling player movement requests and state updates. It maintains a thread
 * for processing network events and manages player references for visualization.
 */
class Client {
public:
    /**
     * @brief Construct a new Client object
     * 
     * @param clientId Unique identifier for this client
     * @param port Local port to bind the client socket to
     * @param serverIp IP address of the server (default: "127.0.0.1")
     * @param serverPort Port number of the server (default: 7000)
     */
    Client(uint32_t clientId, int port, const std::string& serverIp = "127.0.0.1", int serverPort = 7000);
    
    /**
     * @brief Destroy the Client object and cleanup resources
     */
    ~Client();

    /**
     * @brief Start the client and begin network communication
     * 
     * Creates a UDP socket, binds it to the specified port, and starts
     * the network processing thread.
     */
    void start();
    
    /**
     * @brief Stop the client and cleanup resources
     * 
     * Stops the network processing thread and closes the socket.
     */
    void stop();
    
    /**
     * @brief Send a movement request to the server
     * 
     * @param movement MyVec3 containing movement direction and magnitude
     * @param jumpRequested Whether the player is requesting to jump
     */
    void sendMovementRequest(const netcode::math::MyVec3& movement, bool jumpRequested);
    
    /**
     * @brief Set a reference to a networked entity for position updates
     * 
     * @param playerId ID of the player to set reference for
     * @param player Shared pointer to the networked entity
     */
    void setPlayerReference(uint32_t playerId, std::shared_ptr<NetworkedEntity> player);
    
    /**
     * @brief Update a player's position based on server data
     * 
     * @param playerId ID of the player to update
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param isJumping Whether the player is currently jumping
     * @param serverSequence The sequence number of the last input processed by the server
     */
    void updatePlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping, uint32_t serverSequence);
    
    /**
     * @brief Update all entities using interpolation
     * 
     * Call this method in your game loop to update the positions of
     * all remote entities using interpolation
     * 
     * @param deltaTime Time elapsed since last update in seconds
     */
    void updateEntities(float deltaTime);
    
    /**
     * @brief Get the client's unique identifier
     * 
     * @return uint32_t The client ID
     */
    uint32_t getClientId() const { return clientId_; }

private:
    uint32_t clientId_;        ///< Unique identifier for this client
    int port_;                 ///< Local port number
    std::string serverIp_;     ///< Server IP address
    int serverPort_;           ///< Server port number
    int socketFd_;             ///< UDP socket file descriptor
    std::atomic<bool> running_; ///< Flag indicating if client is running
    
    // Thread for network processing
    std::thread clientThread_; ///< Thread for processing network events
    
    // Server address
    sockaddr_in serverAddr_;   ///< Server address structure
    
    ///< Mutex for protecting player map access
    std::mutex playerMutex_;
    ///< Map of player IDs to their visualization objects
    std::map<uint32_t, std::shared_ptr<NetworkedEntity>> players_;
    
    ///< Queue for delayed packet processing
    std::queue<packets::TimestampedPlayerStatePacket> packetQueue_;
    ///< Mutex for protecting packet queue access
    std::mutex queueMutex_;
    
    // Netcode systems for prediction and reconciliation
    std::unique_ptr<SnapshotManager> snapshotManager_;
    std::unique_ptr<PredictionSystem> predictionSystem_;
    std::unique_ptr<ReconciliationSystem> reconciliationSystem_;
    std::unique_ptr<InterpolationSystem> interpolationSystem_;
    
    /**
     * @brief Process incoming network events continuously.
     * 
     * Main network processing loop that receives and handles server updates
     */
    void processNetworkEvents();
    
    /**
     * @brief Handle a server update packet. Apply the update to the player's position.
     * 
     * @param packet The received player state packet
     */
    void handleServerUpdate(const packets::PlayerStatePacket& packet);
};

} // namespace netcode
