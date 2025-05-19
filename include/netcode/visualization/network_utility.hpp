#pragma once

#include "raylib.h"
#include "netcode/visualization/player.hpp"
#include <memory>
#include <chrono>
#include <queue>

namespace netcode {
namespace visualization {

struct PendingUpdate {
    std::chrono::steady_clock::time_point updateTime;
    Vector3 position;
    std::shared_ptr<Player> player;
};

class NetworkUtility {
public:
    enum class Mode {
        TEST,    // Simple test mode with immediate updates
        STANDARD // For future implementation with actual network code
    };

    NetworkUtility(Mode mode = Mode::TEST);

    // Client to Server communication
    void clientToServerUpdate(std::shared_ptr<Player> clientPlayer, 
                            std::shared_ptr<Player> serverPlayer,
                            const Vector3& movement);

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
    const std::chrono::milliseconds SERVER_DELAY{100};
    const std::chrono::milliseconds CLIENT_DELAY{100};
};

}} // namespace netcode::visualization 