#include "gtest/gtest.h"
#include "netcode/server/server.hpp"
#include "netcode/client/client.hpp" // To use as a dummy client
#include "netcode/packets/player_state_packet.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/settings.hpp"

#include <thread>
#include <chrono>
#include <memory>
#include <vector>
#include <arpa/inet.h> // For inet_ntop, sockaddr_in

const std::string TEST_S_LOCALHOST_IP = "127.0.0.1";
const int TEST_S_SERVER_PORT = 13370; // Unique port for these server tests
const int TEST_S_CLIENT_PORT_BASE = 13380;

// Mock ISettings implementation (can be shared or defined per test file)
class MockServerSettings : public netcode::ISettings {
public:
    int getClientToServerDelay() const override { return 0; } // Simulate no delay for direct processing
    int getServerToClientDelay() const override { return 0; } // Simulate no delay
    bool isPredictionEnabled() const override { return false; } // Not directly used by server logic but good to have
    bool isInterpolationEnabled() const override { return false; } // Same as above
};

// Mock NetworkedEntity (can be shared or defined per test file)
class MockServerNetworkedEntity : public netcode::NetworkedEntity {
public:
    uint32_t id_;
    netcode::math::MyVec3 position_{0,0,0};
    bool jumped_ = false;
    uint32_t move_count_ = 0;
    uint32_t jump_count_ = 0;
    uint32_t update_count_ = 0;

    MockServerNetworkedEntity(uint32_t id) : id_(id) {}

    void move(const netcode::math::MyVec3& direction) override {
        position_.x += direction.x;
        position_.y += direction.y;
        position_.z += direction.z;
        move_count_++;
    }
    void update() override { update_count_++; }
    void jump() override { jumped_ = true; jump_count_++; }
    void updateRenderPosition(float deltaTime) override { /* Not called by server directly */ }
    void snapSimulationState(const netcode::math::MyVec3& pos, bool isJumping, float velocityY) override {
        position_ = pos;
        jumped_ = isJumping;
    }
    void initiateVisualBlend() override { /* Client-side */ }
    netcode::math::MyVec3 getPosition() const override { return position_; }
    netcode::math::MyVec3 getRenderPosition() const override { return position_; }
    void setPosition(const netcode::math::MyVec3& pos) override { position_ = pos; }
    netcode::math::MyVec3 getVelocity() const override { return {0,0,0}; }
    uint32_t getId() const override { return id_; }
    float getMoveSpeed() const override { return 1.0f; }
};

class ServerClientTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<netcode::Server> server_;
    std::vector<std::unique_ptr<netcode::Client>> clients_; // Manages ownership of clients created in tests
    std::shared_ptr<MockServerSettings> mock_settings_;

    void SetUp() override {
        mock_settings_ = std::make_shared<MockServerSettings>();
        server_ = std::make_unique<netcode::Server>(TEST_S_SERVER_PORT, mock_settings_);
        ASSERT_TRUE(server_ != nullptr);
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Server start time
    }

    void TearDown() override {
        for (auto& client : clients_) {
            if (client) client->stop();
        }
        clients_.clear();

        if (server_) {
            server_->stop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Threads stop time
    }

    // Helper to create, start, and store a client for the duration of a test
    netcode::Client* CreateAndStartClient(uint32_t id, int port_offset) {
        auto client = std::make_unique<netcode::Client>(id, TEST_S_CLIENT_PORT_BASE + port_offset, TEST_S_LOCALHOST_IP, TEST_S_SERVER_PORT, mock_settings_);
        client->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Client start time
        clients_.push_back(std::move(client));
        return clients_.back().get(); // Return raw pointer for test usage
    }

    int SendAndReceive(netcode::Client* client, const netcode::packets::PlayerMovementRequest& request, netcode::packets::PlayerStatePacket& response_packet, bool expect_response = true) {
        if (!client) return -1;

        client->sendMovementRequest({request.movement_x, request.movement_y, request.movement_z}, request.is_jumping);

        if (!expect_response) return 0;

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        return 0;
    }
};

TEST_F(ServerClientTestFixture, ServerInitialState) {
    SUCCEED();
}

TEST_F(ServerClientTestFixture, ServerStartAndStop) {
    ASSERT_NO_THROW(server_->stop());
}

TEST_F(ServerClientTestFixture, ServerHandlesClientRegistration) {
    netcode::Client* client1 = CreateAndStartClient(1, 0); // Changed to netcode::Client*
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto server_player_mock = std::make_shared<MockServerNetworkedEntity>(1);
    server_->setPlayerReference(1, server_player_mock);

    netcode::packets::PlayerMovementRequest req;
    req.player_id = 1;
    req.movement_x = 1.0f;
    req.input_sequence_number = 1;

    netcode::packets::PlayerStatePacket dummy_response; // Declare a variable
    SendAndReceive(client1, req, dummy_response, false); // Pass the variable

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(server_player_mock->move_count_, 1);
    EXPECT_FLOAT_EQ(server_player_mock->getPosition().x, 1.0f);
}

TEST_F(ServerClientTestFixture, ServerProcessesMovementRequest) {
    netcode::Client* client1 = CreateAndStartClient(1, 0); // Changed
    auto server_player1_mock = std::make_shared<MockServerNetworkedEntity>(1);
    server_->setPlayerReference(1, server_player1_mock);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    netcode::packets::PlayerMovementRequest req;
    req.player_id = 1;
    req.movement_x = 2.5f;
    req.movement_y = 0.0f;
    req.movement_z = -1.0f;
    req.is_jumping = true;
    req.input_sequence_number = 1;

    netcode::packets::PlayerStatePacket dummy_response; // Declare
    SendAndReceive(client1, req, dummy_response, false); // Pass

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(server_player1_mock->move_count_, 1);
    EXPECT_EQ(server_player1_mock->jump_count_, 1);
    EXPECT_EQ(server_player1_mock->update_count_, 1);
    EXPECT_FLOAT_EQ(server_player1_mock->getPosition().x, 2.5f);
    EXPECT_FLOAT_EQ(server_player1_mock->getPosition().z, -1.0f);
    EXPECT_TRUE(server_player1_mock->jumped_);
}

TEST_F(ServerClientTestFixture, ServerBroadcastsPlayerState) {
    netcode::Client* client1 = CreateAndStartClient(1, 0); // Changed
    netcode::Client* client2 = CreateAndStartClient(2, 1); // Changed

    auto server_player1_mock = std::make_shared<MockServerNetworkedEntity>(1);
    auto client2_view_of_player1 = std::make_shared<MockServerNetworkedEntity>(1);

    server_->setPlayerReference(1, server_player1_mock);
    client2->setPlayerReference(1, client2_view_of_player1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    netcode::packets::PlayerMovementRequest req_p1;
    req_p1.player_id = 1;
    req_p1.movement_x = 5.0f;
    req_p1.input_sequence_number = 1;

    netcode::packets::PlayerStatePacket dummy_response; // Declare
    SendAndReceive(client1, req_p1, dummy_response, false); // Pass

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    client2->updateEntities(0.1f);

    EXPECT_FLOAT_EQ(server_player1_mock->getPosition().x, 5.0f);
    EXPECT_FLOAT_EQ(client2_view_of_player1->getPosition().x, 5.0f);
}


TEST_F(ServerClientTestFixture, ServerHandlesMultipleClients) {
    const int num_clients = 3;
    std::vector<std::shared_ptr<MockServerNetworkedEntity>> server_player_mocks;
    std::vector<netcode::Client*> test_clients_raw_pointers; // Store raw pointers for SendAndReceive

    for (int i = 0; i < num_clients; ++i) {
        uint32_t current_client_id = i + 1;
        // CreateAndStartClient now returns a raw pointer, ownership is in ServerClientTestFixture::clients_
        netcode::Client* client_ptr = CreateAndStartClient(current_client_id, i);
        test_clients_raw_pointers.push_back(client_ptr);

        auto server_mock = std::make_shared<MockServerNetworkedEntity>(current_client_id);
        server_player_mocks.push_back(server_mock);
        server_->setPlayerReference(current_client_id, server_mock);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Allow all to register

    netcode::packets::PlayerStatePacket dummy_response; // Declare once
    for (int i = 0; i < num_clients; ++i) {
        netcode::packets::PlayerMovementRequest req;
        req.player_id = i + 1;
        req.movement_x = 1.0f * (i + 1);
        req.input_sequence_number = 1;
        SendAndReceive(test_clients_raw_pointers[i], req, dummy_response, false); // Pass
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    for (int i = 0; i < num_clients; ++i) {
        EXPECT_FLOAT_EQ(server_player_mocks[i]->getPosition().x, 1.0f * (i + 1));
        EXPECT_EQ(server_player_mocks[i]->move_count_, 1);
    }
    // Clients are stopped by the fixture's TearDown
}

TEST_F(ServerClientTestFixture, ServerIgnoresOldInputSequence) {
    netcode::Client* client1 = CreateAndStartClient(1, 0); // Changed
    auto server_player1_mock = std::make_shared<MockServerNetworkedEntity>(1);
    server_->setPlayerReference(1, server_player1_mock);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    netcode::packets::PlayerStatePacket dummy_response; // Declare

    netcode::packets::PlayerMovementRequest req1;
    req1.player_id = 1;
    req1.movement_x = 1.0f;
    req1.input_sequence_number = 5;
    SendAndReceive(client1, req1, dummy_response, false); // Pass
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(server_player1_mock->move_count_, 1);
    EXPECT_FLOAT_EQ(server_player1_mock->getPosition().x, 1.0f);

    netcode::packets::PlayerMovementRequest req2_old;
    req2_old.player_id = 1;
    req2_old.movement_x = 2.0f;
    req2_old.input_sequence_number = 3;
    SendAndReceive(client1, req2_old, dummy_response, false); // Pass
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(server_player1_mock->move_count_, 1);
    EXPECT_FLOAT_EQ(server_player1_mock->getPosition().x, 1.0f);

    netcode::packets::PlayerMovementRequest req3_new;
    req3_new.player_id = 1;
    req3_new.movement_x = 3.0f;
    req3_new.input_sequence_number = 6;
    SendAndReceive(client1, req3_new, dummy_response, false); // Pass
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(server_player1_mock->move_count_, 2);
    EXPECT_FLOAT_EQ(server_player1_mock->getPosition().x, 1.0f + 3.0f);
}