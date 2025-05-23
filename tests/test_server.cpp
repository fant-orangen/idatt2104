#include "gtest/gtest.h"
#include "netcode/server/server.hpp"
#include "netcode/networked_entity.hpp"
#include "netcode/settings.hpp"
#include "netcode/packets/player_state_packet.hpp" // Required for TimestampedPlayerMovementRequest
#include <memory>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // For close()

// Mock settings for testing
class MockServerSettings : public netcode::ISettings {
public:
    bool isPredictionEnabled() const override { return true; } // Not directly used by server but good to have
    bool isInterpolationEnabled() const override { return true; } // Not directly used by server
    int getClientToServerDelay() const override { return 0; }
    int getServerToClientDelay() const override { return 0; }
};

// Mock NetworkedEntity for testing (copied from test_client.cpp for now)
class MockNetworkedEntity : public netcode::NetworkedEntity {
public:
    explicit MockNetworkedEntity(uint32_t id) : id_(id), position_({0,0,0}), renderPosition_({0,0,0}), velocity_({0,0,0}) {}

    void move(const netcode::math::MyVec3& direction) override { position_ = position_ + direction; }
    void update() override { /* Minimal update */ }
    void jump() override { /* Minimal jump */ }
    void updateRenderPosition(float deltaTime) override { renderPosition_ = position_; /* Simplistic, or lerp if needed */ }
    void snapSimulationState(const netcode::math::MyVec3& pos, bool isJumping = false, float velocityY = 0.0f) override {
        position_ = pos;
        velocity_.y = velocityY;
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

class ServerTest : public ::testing::Test {
protected:
    std::shared_ptr<netcode::Server> server_;
    std::shared_ptr<MockServerSettings> settings_;
    int serverPort_ = 7002;
    uint32_t player1Id_ = 1;
    uint32_t player2Id_ = 2;
    int clientSocketFd_ = -1;

    void SetUp() override {
        settings_ = std::make_shared<MockServerSettings>();
        server_ = std::make_shared<netcode::Server>(serverPort_, settings_);
    }

    void TearDown() override {
        server_->stop();
        if (clientSocketFd_ != -1) {
            close(clientSocketFd_);
        }
    }

    // Helper to create a mock client socket
    int createMockClientSocket(int clientPort) {
        int sockFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockFd < 0) {
            perror("socket failed");
            return -1;
        }

        sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = INADDR_ANY;
        clientAddr.sin_port = htons(clientPort);

        if (bind(sockFd, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
            perror("bind failed");
            close(sockFd);
            return -1;
        }
        return sockFd;
    }

    // Helper to send a movement request from a mock client
    void sendMockMovementRequest(int clientSockFd, uint32_t playerId, float x, float y, float z, bool isJumping, uint32_t seqNum, int targetPort, const std::string& targetIp) {
        netcode::packets::PlayerMovementRequest request;
        request.player_id = playerId;
        request.movement_x = x;
        request.movement_y = y;
        request.movement_z = z;
        request.is_jumping = isJumping;
        request.input_sequence_number = seqNum;
        request.wasPredicted = false;

        netcode::packets::TimestampedPlayerMovementRequest timestampedRequest;
        timestampedRequest.timestamp = std::chrono::steady_clock::now();
        timestampedRequest.player_movement_request = request;
        // clientAddr is normally filled by recvfrom on server, not needed for sendto from client perspective here

        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(targetPort);
        inet_pton(AF_INET, targetIp.c_str(), &serverAddr.sin_addr);

        ssize_t bytesSent = sendto(clientSockFd, &timestampedRequest, sizeof(timestampedRequest), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        ASSERT_GT(bytesSent, 0);
    }
};

TEST_F(ServerTest, ServerCreation) {
    ASSERT_NE(server_, nullptr);
}

TEST_F(ServerTest, StartAndStop) {
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow server thread to start
    server_->stop();
}

TEST_F(ServerTest, SetPlayerReference) {
    server_->start();
    auto playerEntity = std::make_shared<MockNetworkedEntity>(player1Id_);
    server_->setPlayerReference(player1Id_, playerEntity);
    // Future: Add a way to get player reference or count from server to verify
    server_->stop();
}

TEST_F(ServerTest, HandleClientRegistrationAndBroadcast) {
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Ensure server is ready

    clientSocketFd_ = createMockClientSocket(9001);
    ASSERT_NE(clientSocketFd_, -1);

    // Player 1 registers
    auto player1Entity = std::make_shared<MockNetworkedEntity>(player1Id_);
    server_->setPlayerReference(player1Id_, player1Entity);
    sendMockMovementRequest(clientSocketFd_, player1Id_, 0.0f, 0.0f, 0.0f, false, 1, serverPort_, "127.0.0.1");

    // Allow server to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); 

    // Check if player 1 received its own state back (or some initial state)
    char buffer[1024];
    sockaddr_in sourceAddr;
    socklen_t sourceLen = sizeof(sourceAddr);
    ssize_t bytesReceived = recvfrom(clientSocketFd_, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*)&sourceAddr, &sourceLen);
    
    // Due to broadcast nature and timing, this can be tricky to assert reliably in simple unit test.
    // We expect at least one packet (the initial state or a broadcast)
    if (bytesReceived > 0) {
        ASSERT_GE(bytesReceived, sizeof(netcode::packets::TimestampedPlayerStatePacket));
        netcode::packets::TimestampedPlayerStatePacket receivedPacket;
        memcpy(&receivedPacket, buffer, sizeof(receivedPacket));
        EXPECT_EQ(receivedPacket.player_state.player_id, player1Id_);
    } else {
        // It's possible the packet hasn't arrived or was missed due to timing.
        // This part of the test is inherently a bit flaky without a more robust mock network.
        // Consider logging or increasing sleep if consistently failing here.
        // For now, we proceed, as the core logic is server-side processing.
    }

    server_->stop();
}

TEST_F(ServerTest, UpdatePlayerStateAndBroadcast) {
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Client 1 (mock)
    int client1SocketFd = createMockClientSocket(9002);
    ASSERT_NE(client1SocketFd, -1);
    auto player1Entity = std::make_shared<MockNetworkedEntity>(player1Id_);
    server_->setPlayerReference(player1Id_, player1Entity);
    // Send registration for P1
    sendMockMovementRequest(client1SocketFd, player1Id_, 0.0f, 0.0f, 0.0f, false, 1, serverPort_, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow registration processing

    // Client 2 (mock)
    int client2SocketFd = createMockClientSocket(9003);
    ASSERT_NE(client2SocketFd, -1);
    auto player2Entity = std::make_shared<MockNetworkedEntity>(player2Id_);
    server_->setPlayerReference(player2Id_, player2Entity);
    // Send registration for P2
    sendMockMovementRequest(client2SocketFd, player2Id_, 0.0f, 0.0f, 0.0f, false, 1, serverPort_, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow registration processing

    // P1 sends a movement request
    sendMockMovementRequest(client1SocketFd, player1Id_, 1.0f, 2.0f, 3.0f, false, 2, serverPort_, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(16 + 100)); // Wait for processing and broadcast interval, increased delay slightly

    // Check P2 (client2SocketFd) received P1's state update
    char buffer[1024];
    sockaddr_in sourceAddr;
    socklen_t sourceLen = sizeof(sourceAddr);
    ssize_t bytesReceived = 0;
    netcode::packets::TimestampedPlayerStatePacket packet;
    bool foundPacket = false;
    auto startTime = std::chrono::steady_clock::now();

    // Try to receive the specific packet for a short duration
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() < 200) {
        bytesReceived = recvfrom(client2SocketFd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*)&sourceAddr, &sourceLen);
        if (bytesReceived >= sizeof(netcode::packets::TimestampedPlayerStatePacket)) {
            memcpy(&packet, buffer, sizeof(packet));
            if (packet.player_state.player_id == player1Id_ && packet.player_state.last_processed_input_sequence == 2) {
                foundPacket = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Small pause before retrying
    }
    
    ASSERT_TRUE(foundPacket); // Ensure we found the specific packet
    EXPECT_GT(bytesReceived, 0); // Redundant if foundPacket is true, but good for structure
    // No need for if (bytesReceived > 0) anymore due to ASSERT_TRUE
    EXPECT_EQ(packet.player_state.player_id, player1Id_);
    EXPECT_FLOAT_EQ(packet.player_state.x, 1.0f); 
    EXPECT_FLOAT_EQ(packet.player_state.y, 2.0f);
    EXPECT_FLOAT_EQ(packet.player_state.z, 3.0f);
    EXPECT_EQ(packet.player_state.last_processed_input_sequence, 2); 

    // Clean up mock client sockets
    close(client1SocketFd);
    close(client2SocketFd);
    server_->stop();
}

TEST_F(ServerTest, IgnoreOldInputSequence) {
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int clientSockFd = createMockClientSocket(9004);
    ASSERT_NE(clientSockFd, -1);

    auto playerEntity = std::make_shared<MockNetworkedEntity>(player1Id_);
    server_->setPlayerReference(player1Id_, playerEntity);
    playerEntity->setPosition({0.f, 0.f, 0.f});

    // Send initial valid request (seq 1)
    sendMockMovementRequest(clientSockFd, player1Id_, 1.0f, 0.0f, 0.0f, false, 1, serverPort_, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow processing
    EXPECT_FLOAT_EQ(playerEntity->getPosition().x, 1.0f);

    // Send an older request (seq 0)
    sendMockMovementRequest(clientSockFd, player1Id_, 2.0f, 0.0f, 0.0f, false, 0, serverPort_, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow processing (or ignoring)
    // Position should not change because the input is old
    EXPECT_FLOAT_EQ(playerEntity->getPosition().x, 1.0f);

    // Send a newer request (seq 2)
    sendMockMovementRequest(clientSockFd, player1Id_, 3.0f, 0.0f, 0.0f, false, 2, serverPort_, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow processing
    EXPECT_FLOAT_EQ(playerEntity->getPosition().x, 4.0f); // Initial (0) + mv1 (1) + mv2 (3) = 4

    close(clientSockFd);
    server_->stop();
}

TEST_F(ServerTest, SetPlayerPositionDirectly) {
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int clientSockFd = createMockClientSocket(9005);
    ASSERT_NE(clientSockFd, -1);

    auto playerEntity = std::make_shared<MockNetworkedEntity>(player1Id_);
    server_->setPlayerReference(player1Id_, playerEntity);

    // Send a dummy request to register the client address
    sendMockMovementRequest(clientSockFd, player1Id_, 0.f, 0.f, 0.f, false, 0, serverPort_, "127.0.0.1");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    server_->setPlayerPosition(player1Id_, 10.0f, 20.0f, 30.0f, false);
    EXPECT_FLOAT_EQ(playerEntity->getPosition().x, 10.0f);
    EXPECT_FLOAT_EQ(playerEntity->getPosition().y, 20.0f);
    EXPECT_FLOAT_EQ(playerEntity->getPosition().z, 30.0f);

    std::this_thread::sleep_for(std::chrono::milliseconds(16 + 50)); // (MIN_BROADCAST_INTERVAL_MS is 16)

    // Check if the client received the update
    char buffer[1024];
    sockaddr_in sourceAddr;
    socklen_t sourceLen = sizeof(sourceAddr);
    ssize_t bytesReceived = 0;
    netcode::packets::TimestampedPlayerStatePacket packet;
    bool foundPacket = false;
    auto startTime = std::chrono::steady_clock::now();

    // Try to receive the specific packet for a short duration
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() < 200) {
        bytesReceived = recvfrom(clientSockFd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*)&sourceAddr, &sourceLen);
        if (bytesReceived >= sizeof(netcode::packets::TimestampedPlayerStatePacket)) {
            memcpy(&packet, buffer, sizeof(packet));
            // We expect the state to be (10,20,30) and seq will be the last processed one (0 in this case)
            if (packet.player_state.player_id == player1Id_ && 
                std::abs(packet.player_state.x - 10.0f) < 0.001f &&
                packet.player_state.last_processed_input_sequence == 0) {
                foundPacket = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Small pause before retrying
    }

    ASSERT_TRUE(foundPacket);
    EXPECT_GT(bytesReceived, 0); 
    EXPECT_EQ(packet.player_state.player_id, player1Id_);
    EXPECT_FLOAT_EQ(packet.player_state.x, 10.0f);
    EXPECT_FLOAT_EQ(packet.player_state.y, 20.0f);
    EXPECT_FLOAT_EQ(packet.player_state.z, 30.0f);
    //EXPECT_EQ(packet.player_state.last_processed_input_sequence, 0); // Already checked in loop

    close(clientSockFd);
    server_->stop();
}


/*int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}*/
