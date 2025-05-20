#pragma once

#include "raylib.h"
#include "netcode/visualization/player.hpp"
#include <memory>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>

namespace netcode {
namespace visualization {

struct PendingUpdate {
    std::chrono::steady_clock::time_point updateTime;
    Vector3 position;
    std::shared_ptr<Player> player;
};

struct InputEvent {
    Vector3 movement;
    bool jumpRequested;
    std::shared_ptr<Player> clientPlayer;
    std::shared_ptr<Player> serverPlayer;
};

class NetworkUtility {
public:
    enum class Mode {
        TEST,    // Simple test mode with immediate updates
        STANDARD // For future implementation with actual network code
    };

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

private:
    Mode mode_;
    std::queue<PendingUpdate> client1Updates_;
    std::queue<PendingUpdate> client2Updates_;
    std::queue<InputEvent> inputQueue_;
    const std::chrono::milliseconds SERVER_DELAY{10};
    const std::chrono::milliseconds CLIENT_DELAY{10};
    
    // Threading
    std::thread networkThread_;
    std::mutex queueMutex_;
    std::atomic<bool> running_{true};
    
    void processNetworkEvents();
};

}} // namespace netcode::visualization 