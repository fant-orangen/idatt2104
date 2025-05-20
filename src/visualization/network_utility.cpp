#include "netcode/visualization/network_utility.hpp"
#include <chrono>
#include <thread>

namespace netcode {
namespace visualization {

NetworkUtility::NetworkUtility(Mode mode) : mode_(mode) {
    // Start network processing thread
    networkThread_ = std::thread(&NetworkUtility::processNetworkEvents, this);
}

NetworkUtility::~NetworkUtility() {
    running_ = false;
    if (networkThread_.joinable()) {
        networkThread_.join();
    }
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
}

void NetworkUtility::serverToClientsUpdate(std::shared_ptr<Player> serverPlayer,
                                         std::shared_ptr<Player> client1Player,
                                         std::shared_ptr<Player> client2Player) {
    if (mode_ == Mode::TEST && serverPlayer) {
        auto updateTime = std::chrono::steady_clock::now() + CLIENT_DELAY;
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (client1Player) {
            client1Updates_.push({updateTime, serverPlayer->getPosition(), client1Player});
        }
        if (client2Player) {
            client2Updates_.push({updateTime, serverPlayer->getPosition(), client2Player});
        }
    }
}

void NetworkUtility::processNetworkEvents() {
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
}

}} // namespace netcode::visualization 