#include "gtest/gtest.h"
#include "netcode/client/client.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/settings.hpp"
#include <memory>
#include <thread>
#include <chrono>

// Mock settings for testing
class MockSettings : public netcode::ISettings {
public:
    bool isPredictionEnabled() const override { return true; }
    bool isInterpolationEnabled() const override { return true; }
    int getClientToServerDelay() const override { return 0; }
    int getServerToClientDelay() const override { return 0; }
};

// Mock ISettings to disable prediction
class NoPredictionSettings : public netcode::ISettings {
public:
    bool isPredictionEnabled() const override { return false; }
    bool isInterpolationEnabled() const override { return true; }
    int getClientToServerDelay() const override { return 0; }
    int getServerToClientDelay() const override { return 0; }
};

// Mock ISettings to disable interpolation
class NoInterpolationSettings : public netcode::ISettings {
public:
    bool isPredictionEnabled() const override { return true; }
    bool isInterpolationEnabled() const override { return false; }
    int getClientToServerDelay() const override { return 0; }
    int getServerToClientDelay() const override { return 0; }
};

// Mock NetworkedEntity for testing
class MockNetworkedEntity : public netcode::NetworkedEntity {
public:
    explicit MockNetworkedEntity(uint32_t id) : id_(id), position_({0,0,0}), renderPosition_({0,0,0}), velocity_({0,0,0}) {}

    void move(const netcode::math::MyVec3& direction) override { position_ = position_ + direction; }
    void update() override { /* Minimal update */ }
    void jump() override { /* Minimal jump */ }
    void updateRenderPosition(float deltaTime) override { renderPosition_ = position_; /* Simplistic, or lerp if needed */ }
    void snapSimulationState(const netcode::math::MyVec3& pos, bool isJumping = false, float velocityY = 0.0f) override {
        position_ = pos;
        // velocity_ handling might need adjustment based on how it's used, 
        // for now, let's assume velocityY might update part of velocity_
        velocity_.y = velocityY;
        // TODO: Handle isJumping state if necessary in the mock
    }
    void initiateVisualBlend() override {}
    netcode::math::MyVec3 getPosition() const override { return position_; }
    netcode::math::MyVec3 getRenderPosition() const override { return renderPosition_; }
    void setPosition(const netcode::math::MyVec3& pos) override { position_ = pos; renderPosition_ = pos; }
    netcode::math::MyVec3 getVelocity() const override { return velocity_; }
    uint32_t getId() const override { return id_; }
    float getMoveSpeed() const override { return 1.0f; } 

private:
    uint32_t id_;
    netcode::math::MyVec3 position_;
    netcode::math::MyVec3 renderPosition_;
    netcode::math::MyVec3 velocity_;
};

class ClientTest : public ::testing::Test {
protected:
    std::shared_ptr<netcode::Client> client_;
    std::shared_ptr<MockSettings> settings_;
    uint32_t clientId_ = 1;
    int clientPort_ = 8001;
    std::string serverIp_ = "127.0.0.1";
    int serverPort_ = 7001; // Use a different port for mock server if needed

    void SetUp() override {
        settings_ = std::make_shared<MockSettings>();
        client_ = std::make_shared<netcode::Client>(clientId_, clientPort_, serverIp_, serverPort_, settings_);
    }

    void TearDown() override {
        client_->stop();
    }
};

TEST_F(ClientTest, ClientCreation) {
    ASSERT_NE(client_, nullptr);
    EXPECT_EQ(client_->getClientId(), clientId_);
}

TEST_F(ClientTest, StartAndStop) {
    client_->start();
    // Add a small delay to allow the thread to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // We can't directly check if the thread is running in a portable way without more complex setup.
    // For now, we just ensure start/stop don't crash.
    client_->stop();
}

TEST_F(ClientTest, SetPlayerReference) {
    client_->start();
    auto playerEntity = std::make_shared<MockNetworkedEntity>(clientId_);
    client_->setPlayerReference(clientId_, playerEntity);
    // Future: Add a way to get player reference or count to verify
    client_->stop();
}

TEST_F(ClientTest, SendMovementRequest) {
    client_->start();
    auto playerEntity = std::make_shared<MockNetworkedEntity>(clientId_);
    client_->setPlayerReference(clientId_, playerEntity);

    // Simulate a simple server socket to receive the packet
    int mockServerSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(mockServerSocketFd, 0);

    sockaddr_in mockServerAddr;
    memset(&mockServerAddr, 0, sizeof(mockServerAddr));
    mockServerAddr.sin_family = AF_INET;
    mockServerAddr.sin_addr.s_addr = inet_addr(serverIp_.c_str());
    mockServerAddr.sin_port = htons(serverPort_);

    ASSERT_GE(bind(mockServerSocketFd, (struct sockaddr*)&mockServerAddr, sizeof(mockServerAddr)), 0);

    netcode::math::MyVec3 movement = {1.0f, 0.0f, 0.0f};
    client_->sendMovementRequest(movement, false);

    // Try to receive the packet on the mock server
    char buffer[1024];
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    ssize_t bytesReceived = recvfrom(mockServerSocketFd, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddr, &clientLen);

    // Depending on network conditions and timing, this might be flaky.
    // For robust testing, a more controlled mock server or loopback setup is ideal.
    // For now, we check if *any* data was sent.
    EXPECT_GT(bytesReceived, 0);
    if (bytesReceived > 0) {
        netcode::packets::TimestampedPlayerMovementRequest receivedRequest;
        ASSERT_GE(bytesReceived, sizeof(receivedRequest));
        memcpy(&receivedRequest, buffer, sizeof(receivedRequest));
        EXPECT_EQ(receivedRequest.player_movement_request.player_id, clientId_);
        EXPECT_EQ(receivedRequest.player_movement_request.movement_x, movement.x);
    }

    close(mockServerSocketFd);
    client_->stop();
}

TEST_F(ClientTest, UpdatePlayerPositionRemotePlayerWithInterpolation) {
    client_->start();
    uint32_t remotePlayerId = 2;
    auto remotePlayerEntity = std::make_shared<MockNetworkedEntity>(remotePlayerId);
    client_->setPlayerReference(remotePlayerId, remotePlayerEntity);
    remotePlayerEntity->setPosition({0.0f, 0.0f, 0.0f});

    client_->updateEntities(0.1f); // Simulate some time passing
    // After updateEntities, interpolation should have moved the entity.
    // This depends on the interpolation delay and other factors.
    // For a simple test, we might not see a change if the delay is too high or deltaTime too low.
    // For this test, we mostly check that it runs.
    // A more robust test would check the InterpolationSystem's internal state or mock time.

    client_->stop();
}

TEST_F(ClientTest, UpdatePlayerPositionLocalPlayerNoPrediction) {
    auto noPredSettings = std::make_shared<NoPredictionSettings>();
    client_->setSettings(noPredSettings); // Apply new settings

    client_->start();
    auto playerEntity = std::make_shared<MockNetworkedEntity>(clientId_);
    client_->setPlayerReference(clientId_, playerEntity);
    netcode::math::MyVec3 initialPos = {0.0f, 0.0f, 0.0f};
    playerEntity->setPosition(initialPos);

    netcode::math::MyVec3 serverPos = {1.0f, 2.0f, 3.0f};
    client_->updatePlayerPosition(clientId_, serverPos.x, serverPos.y, serverPos.z, false, 1);
    
    // Without prediction, position should be directly set to server's state
    EXPECT_EQ(playerEntity->getPosition().x, serverPos.x);
    EXPECT_EQ(playerEntity->getPosition().y, serverPos.y);
    EXPECT_EQ(playerEntity->getPosition().z, serverPos.z);
    
    client_->stop();
}

TEST_F(ClientTest, UpdatePlayerPositionRemotePlayerNoInterpolation) {
    auto noInterpSettings = std::make_shared<NoInterpolationSettings>();
    client_->setSettings(noInterpSettings);

    client_->start();
    uint32_t remotePlayerId = 2;
    auto remotePlayerEntity = std::make_shared<MockNetworkedEntity>(remotePlayerId);
    client_->setPlayerReference(remotePlayerId, remotePlayerEntity);
    netcode::math::MyVec3 initialPos = {0.0f, 0.0f, 0.0f};
    remotePlayerEntity->setPosition(initialPos);

    netcode::math::MyVec3 serverPos = {5.0f, 6.0f, 7.0f};
    client_->updatePlayerPosition(remotePlayerId, serverPos.x, serverPos.y, serverPos.z, false, 1);
    
    // Without interpolation, position should be directly set to server's state
    EXPECT_EQ(remotePlayerEntity->getPosition().x, serverPos.x);
    EXPECT_EQ(remotePlayerEntity->getPosition().y, serverPos.y);
    EXPECT_EQ(remotePlayerEntity->getPosition().z, serverPos.z);
    
    client_->stop();
}

// Test handling of server updates (simulated)
// This requires a way to inject packets into the client's processing queue
// or mocking the recvfrom call, which is more involved.
// For now, we can test the handleServerUpdate method more directly if possible,
// or simplify by focusing on the updatePlayerPosition logic as done above.
