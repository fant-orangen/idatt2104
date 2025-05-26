#pragma once

#include "raylib.h"
#include "netcode/visualization/player.hpp"
#include "netcode/visualization/concrete_settings.hpp"
#include "netcode/server/server.hpp"
#include "netcode/client/client.hpp"
#include "netcode/packets/player_state_packet.hpp"
#include "netcode/math/my_vec3.hpp"
#include <memory>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

namespace netcode {
namespace visualization {

// Utility functions for Vector3 <-> MyVec3 conversion
/**
 * @brief Convert from raylib::Vector3 to netcode::math::MyVec3
 * @param vec The Vector3 to convert
 * @return The equivalent MyVec3
 */
inline netcode::math::MyVec3 toMyVec3(const Vector3& vec) {
    return netcode::math::MyVec3{vec.x, vec.y, vec.z};
}

/**
 * @brief Convert from netcode::math::MyVec3 to raylib::Vector3
 * @param vec The MyVec3 to convert
 * @return The equivalent Vector3
 */
inline Vector3 toVector3(const netcode::math::MyVec3& vec) {
    return Vector3{vec.x, vec.y, vec.z};
}

struct PendingUpdate {
    std::chrono::steady_clock::time_point updateTime;
    Vector3 position;
    std::shared_ptr<Player> player;
};

/**
 * Struct representing the input event sent by the client 
 */
struct InputEvent {
    Vector3 movement;
    bool jumpRequested;
    std::shared_ptr<Player> clientPlayer;
    std::shared_ptr<Player> serverPlayer;
};

/**
 * This class is used to either simulate or actually send and receive data over the network. 
 * Its purpose is to demonstrate game mechanics in a networked environment by processing user inputs
 * through a simulated client-server interaction. It does this by sending inputs from client to server 
 * and vice versa, and only updating player states once packages are acknowledged by the server.
 */
class NetworkUtility {
public:
    enum class Mode {
        TEST,      // Simple test mode with immediate updates
        STANDARD   // Uses actual network code for communication
    };

    // Constants for network configuration
    static constexpr int SERVER_PORT = 7000;
    static constexpr int CLIENT1_PORT = 7001;
    static constexpr int CLIENT2_PORT = 7002;
    
    // Player IDs
    static constexpr uint32_t SERVER_PLAYER_ID = 0;
    static constexpr uint32_t CLIENT1_PLAYER_ID = 1;
    static constexpr uint32_t CLIENT2_PLAYER_ID = 2;

    NetworkUtility(Mode mode = Mode::TEST);
    ~NetworkUtility();

    // Client to Server communication
    void clientToServerUpdate(std::shared_ptr<Player> clientPlayer, 
                            std::shared_ptr<Player> serverPlayer,
                            const Vector3& movement,
                            bool jumpRequested = false);

    // Server to Clients communication
    void serverToClientsUpdate(std::shared_ptr<Player> serverPlayer,
                             std::shared_ptr<Player> client1Player,
                             std::shared_ptr<Player> client2Player);

    // Process any pending updates
    void update();

    // Update a player position - called by server/client when they receive updates
    void updatePlayerPosition(uint32_t playerId, float x, float y, float z, bool isJumping);
    
    // Check if running in test mode
    bool isTestMode() const { return mode_ == Mode::TEST; }
    
    // Get client objects
    Client* getClient1() { return client1_.get(); }
    Client* getClient2() { return client2_.get(); }
    
    // Get server object
    Server* getServer() { return server_.get(); }
    
    // Get settings object for configuration
    ConcreteSettings* getSettings() { return settings_.get(); }

private:
    Mode mode_;
    std::queue<PendingUpdate> client1Updates_;
    std::queue<PendingUpdate> client2Updates_;
    std::queue<InputEvent> inputQueue_;

    // Settings implementation
    std::shared_ptr<ConcreteSettings> settings_;

    // Network components
    std::unique_ptr<Server> server_;        // The ONE server
    std::unique_ptr<Client> client1_;       // Client 1
    std::unique_ptr<Client> client2_;       // Client 2
    
    // Player references for network updates
    std::shared_ptr<Player> serverPlayerRef_;
    std::shared_ptr<Player> client1PlayerRef_;
    std::shared_ptr<Player> client2PlayerRef_;

    /**
     * The delay before the server receives the input from the client.
     * It is important to note that the client who sends the input is affected by this delay since their input must be acknowledged by the server.
     */
    const std::chrono::milliseconds SERVER_DELAY{10};

    /**
     * The delay before the clients receive the input from the server.
     */
    const std::chrono::milliseconds CLIENT_DELAY{400};
    
    // Threading
    std::thread networkThread_;
    std::mutex queueMutex_;
    std::atomic<bool> running_{true};
    
    // Initialize network components
    void initializeNetworking();
    
    // Process networking events
    void processNetworkEvents();
    
    // Send player state from client to server
    void sendPlayerStateToServer(uint32_t playerId, const Vector3& position, bool isJumping, Client* client);
};

}} // namespace netcode::visualization 