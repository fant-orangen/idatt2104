#include "netcode/visualization/network_utility.hpp"
#include <chrono>
#include <thread>

namespace netcode {
namespace visualization {

NetworkUtility::NetworkUtility(Mode mode) : mode_(mode) {}

void NetworkUtility::clientToServerUpdate(std::shared_ptr<Player> clientPlayer,
                                        std::shared_ptr<Player> serverPlayer,
                                        const Vector3& movement) {
    if (mode_ == Mode::TEST) {
        // In test mode, update server after SERVER_DELAY
        if (serverPlayer) {
            // Move the server player immediately
            serverPlayer->move(movement);
        }
    }
    // STANDARD mode would be implemented here with actual network code
}

void NetworkUtility::serverToClientsUpdate(std::shared_ptr<Player> serverPlayer,
                                         std::shared_ptr<Player> client1Player,
                                         std::shared_ptr<Player> client2Player) {
    if (mode_ == Mode::TEST) {
        if (serverPlayer) {
            Vector3 serverPos = serverPlayer->getPosition();
            
            // Schedule updates for both clients after CLIENT_DELAY
            auto updateTime = std::chrono::steady_clock::now() + CLIENT_DELAY;
            
            if (client1Player) {
                client1Updates_.push({updateTime, serverPos, client1Player});
            }
            if (client2Player) {
                client2Updates_.push({updateTime, serverPos, client2Player});
            }
        }
    }
    // STANDARD mode would be implemented here with actual network code
}

void NetworkUtility::update() {
    if (mode_ == Mode::TEST) {
        auto now = std::chrono::steady_clock::now();
        
        // Process client 1 updates
        while (!client1Updates_.empty() && now >= client1Updates_.front().updateTime) {
            auto& update = client1Updates_.front();
            update.player->setPosition(update.position);
            client1Updates_.pop();
        }
        
        // Process client 2 updates
        while (!client2Updates_.empty() && now >= client2Updates_.front().updateTime) {
            auto& update = client2Updates_.front();
            update.player->setPosition(update.position);
            client2Updates_.pop();
        }
    }
}

}} // namespace netcode::visualization 