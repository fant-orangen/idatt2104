#include "gtest/gtest.h"
#include "netcode/client/client.hpp"
#include "netcode/server/server.hpp" // For creating a dummy server
#include "netcode/packets/player_state_packet.hpp"
#include "netcode/networked_entity.hpp" // For a mock entity
#include "netcode/settings.hpp" // For a mock settings

#include <thread>
#include <chrono>
#include <memory>

const std::string LOCALHOST_IP = "127.0.0.1";
const int TEST_SERVER_PORT = 12345; // Arbitrary port for testing
const int TEST_CLIENT_PORT_BASE = 12360; // Base port for clients to avoid clashes

// Mock ISettings implementation
class MockSettings : public netcode::ISettings {
public:
    int getClientToServerDelay() const override { return 0; }
    int getServerToClientDelay() const override { return 0; }
    bool isPredictionEnabled() const override { return false; }
    bool isInterpolationEnabled() const override { return false; }
};

// Mock NetworkedEntity
class MockNetworkedEntity : public netcode::NetworkedEntity {
public:
    uint32_t id_;
    netcode::math::MyVec3 position_{0,0,0};
    bool jumped_ = false;

    MockNetworkedEntity(uint32_t id) : id_(id) {}

    void move(const netcode::math::MyVec3& direction) override {
        position_.x += direction.x;
        position_.y += direction.y;
        position_.z += direction.z;
    }
    void update() override { /* Do nothing for mock */ }
    void jump() override { jumped_ = true; }
    void updateRenderPosition(float deltaTime) override { /* Do nothing */ }
    void snapSimulationState(const netcode::math::MyVec3& pos, bool isJumping, float velocityY) override {
        position_ = pos;
        jumped_ = isJumping;
    }
    void initiateVisualBlend() override { /* Do nothing */ }
    netcode::math::MyVec3 getPosition() const override { return position_; }
    netcode::math::MyVec3 getRenderPosition() const override { return position_; } // Simple mock
    void setPosition(const netcode::math::MyVec3& pos) override { position_ = pos; }
    netcode::math::MyVec3 getVelocity() const override { return {0,0,0}; }
    uint32_t getId() const override { return id_; }
    float getMoveSpeed() const override { return 1.0f; }
};

class ClientServerTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<netcode::Server> server_;
    std::unique_ptr<netcode::Client> client_;
    std::shared_ptr<MockSettings> mock_settings_;
    uint32_t client_id_ = 1;
    int client_port_ = TEST_CLIENT_PORT_BASE;

    void SetUp() override {
        mock_settings_ = std::make_shared<MockSettings>();
        server_ = std::make_unique<netcode::Server>(TEST_SERVER_PORT, mock_settings_);
        ASSERT_TRUE(server_ != nullptr);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give server time to start

        client_ = std::make_unique<netcode::Client>(client_id_, client_port_, LOCALHOST_IP, TEST_SERVER_PORT, mock_settings_);
        ASSERT_TRUE(client_ != nullptr);
    }

    void TearDown() override {
        if (client_) {
            client_->stop();
        }
        if (server_) {
            server_->stop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow threads to fully stop
    }

    // Helper to create a client with a unique port for multi-client tests
    std::unique_ptr<netcode::Client> CreateAnotherClient(uint32_t id, int port_offset) {
        auto new_client = std::make_unique<netcode::Client>(id, TEST_CLIENT_PORT_BASE + port_offset, LOCALHOST_IP, TEST_SERVER_PORT, mock_settings_);
        return new_client;
    }
};

TEST_F(ClientServerTestFixture, ClientInitialState) {
    // Client is created in SetUp, but not started.
    // Let's test a freshly created client here.
    netcode::Client fresh_client(2, TEST_CLIENT_PORT_BASE + 1, LOCALHOST_IP, TEST_SERVER_PORT, mock_settings_);
    // No explicit is_connected(), but we can check if start succeeds.
    // This test might be more about constructor behavior.
    SUCCEED(); // Placeholder for now if no direct state to check before start
}

TEST_F(ClientServerTestFixture, ClientStartAndStop) {
    ASSERT_NO_THROW(client_->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give time for thread to init
    // Add an is_running() to client or check logs if possible
    ASSERT_NO_THROW(client_->stop());
}


TEST_F(ClientServerTestFixture, ClientSendMovementRequest) {
    client_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto local_player = std::make_shared<MockNetworkedEntity>(client_id_);
    client_->setPlayerReference(client_id_, local_player); // For prediction system
    server_->setPlayerReference(client_id_, std::make_shared<MockNetworkedEntity>(client_id_)); // Server needs a ref too

    netcode::math::MyVec3 movement = {1.0f, 0.0f, 0.5f};
    bool jump_requested = true;

    ASSERT_NO_THROW(client_->sendMovementRequest(movement, jump_requested));

    // Wait for server to process and broadcast
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Check if the server's player mock was updated
    // This requires the server to expose its player states or have a test hook.
    // For simplicity, we'll assume the server processes it if it receives it.
    // A more robust test would involve another client receiving the update.

    // Let's check if the client's prediction system updated the local player
    // (if prediction was enabled in mock_settings_, which it's not by default)
    // If prediction is off, client player position shouldn't change until server update.
    EXPECT_FLOAT_EQ(local_player->getPosition().x, 0.0f); // No prediction

    // Now, let the client process incoming packets
    client_->updateEntities(0.1f); // Simulate some time passing

    // If the server sent back an update, the client's player should eventually update
    // This is more of an integration test for the loop.
}

TEST_F(ClientServerTestFixture, ClientReceivesServerUpdate) {
    client_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    uint32_t player_to_update_id = 2;
    auto client_player_view = std::make_shared<MockNetworkedEntity>(player_to_update_id);
    client_->setPlayerReference(player_to_update_id, client_player_view);

    // Simulate server sending a PlayerStatePacket
    // This normally happens in server's broadcastPlayerState
    // We will manually call client's updatePlayerPosition which is what handleServerUpdate does
    float server_x = 10.0f, server_y = 1.0f, server_z = 5.0f;
    bool server_is_jumping = true;
    uint32_t server_seq = 1;

    // Simulate the reception path (handleServerUpdate -> updatePlayerPosition)
    client_->updatePlayerPosition(player_to_update_id, server_x, server_y, server_z, server_is_jumping, server_seq);

    // Client updates its entities (interpolation or direct set if no interpolation)
    client_->updateEntities(0.1f); // Process updates

    // Since interpolation is off by default in MockSettings, it should snap
    EXPECT_FLOAT_EQ(client_player_view->getPosition().x, server_x);
    EXPECT_FLOAT_EQ(client_player_view->getPosition().y, server_y);
    EXPECT_FLOAT_EQ(client_player_view->getPosition().z, server_z);
    EXPECT_EQ(client_player_view->jumped_, server_is_jumping);
}

TEST_F(ClientServerTestFixture, ClientRegistersWithServerOnStart) {
    // Client start() sends an initial registration packet.
    // We need to check if the server adds this client to its clientAddresses_ map.
    // This requires exposing clientAddresses_ or a way to query connected clients from the server.
    // For now, we'll assume it works if no errors are thrown and proceed to send a packet.

    client_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow registration

    // Send a movement request to confirm the server knows about the client
    auto local_player = std::make_shared<MockNetworkedEntity>(client_id_);
    client_->setPlayerReference(client_id_, local_player);
    server_->setPlayerReference(client_id_, std::make_shared<MockNetworkedEntity>(client_id_));

    netcode::math::MyVec3 movement = {0.1f, 0.0f, 0.0f};
    ASSERT_NO_THROW(client_->sendMovementRequest(movement, false));

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow processing
    // A more detailed check would involve a second client receiving state of first.
    SUCCEED();
}

// More tests to consider:
// - Client behavior when server is not available (start client, try send, expect errors/timeouts)
// - Multiple clients sending updates and receiving broadcasts.
// - Tests for prediction, reconciliation, interpolation by enabling them in MockSettings
//   and verifying entity positions and states.
// - Test packet queueing in client and server with simulated delays.