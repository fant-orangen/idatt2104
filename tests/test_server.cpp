#include "gtest/gtest.h"
#include "netcode/server.hpp"       //
#include "netcode/client.hpp"       // To use as a dummy client
#include "netcode/serialization.hpp" // For netcode::Buffer
#include "netcode/packet_types.hpp"  // For PacketHeader, MessageType

#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

// Use a different port for server tests to avoid conflict with client tests if run concurrently (though gtest usually runs them in one process)
const int SERVER_TEST_PORT = 23456;
const std::string LOCALHOST_IP = "127.0.0.1";

class ServerTest : public ::testing::Test {
protected:
    // Using port 0 will let the OS choose an available ephemeral port,
    // reducing chances of "address already in use" errors in tests.
    // We can retrieve the actual port later if needed.
    Server server_{0}; // Start with port 0 for dynamic assignment
    netcode::Buffer test_buffer_;
    uint16_t actual_server_port_ = 0;

    void SetUp() override {
        // The server is created but not started here.
        // Tests that need a running server will call server_.start().
        // If server_ starts on port 0, we need to get the actual port after start.
    }

    void TearDown() override {
        if (server_.is_running()) { //
            server_.stop(); //
        }
    }

    // Helper to get the actual port the server bound to
    // This is a bit of a hack; a better way would be for the Server class to provide this.
    // For now, we assume the test client will connect to what was requested if not 0.
    // If Server uses port 0, the test client needs to know the actual chosen port.
    // The Server class currently doesn't expose the chosen port if 0 is passed.
    // For simplicity in these tests, we'll assume a fixed known port if not 0,
    // or modify server to allow getting the port.

    // For tests that need to know the ephemeral port, the server would need a getter.
    // Let's assume for now we test with a fixed port, or the client part of the test
    // can discover the server if the server announces itself (not current design).
    // For now, let's create another server instance in tests that need specific port knowledge
    // or rely on the fact that our dummy client will connect to localhost:actual_server_port.
};

TEST_F(ServerTest, InitialState) {
    EXPECT_FALSE(server_.is_running()) << "Server should not be running initially."; //
}

TEST_F(ServerTest, StartAndStopServer) {
    Server local_server(SERVER_TEST_PORT);
    EXPECT_TRUE(local_server.start()) << "Server failed to start."; //
    EXPECT_TRUE(local_server.is_running()) << "Server should be running after start."; //
    
    // Give the listener thread a moment to fully initialize (e.g. enter its loop)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    local_server.stop(); //
    EXPECT_FALSE(local_server.is_running()) << "Server should not be running after stop."; //
}

TEST_F(ServerTest, StopServerWhenNotRunning) {
    EXPECT_FALSE(server_.is_running()); //
    ASSERT_NO_THROW(server_.stop()); //
    EXPECT_FALSE(server_.is_running()); //
}

TEST_F(ServerTest, StartServerOnPortZero) {
    // Server 'server_' from fixture is initialized with port 0
    EXPECT_TRUE(server_.start()) << "Server failed to start on port 0."; //
    EXPECT_TRUE(server_.is_running()); //
    // Note: We don't know the actual port here unless the Server class exposes it.
    // This test just checks it can start.
    server_.stop(); //
}


TEST_F(ServerTest, ReceiveTimeoutInLoop) {
    // This test relies on the 1-second timeout in the server's recvfrom.
    // The server should loop and log timeouts without exiting its listener_loop immediately.
    Server local_server(SERVER_TEST_PORT + 1); // Use a unique port
    ASSERT_TRUE(local_server.start()); //
    
    // Let the server run for a bit longer than its socket timeout to see if it logs timeouts
    // and continues running. The server logs "Receive timeout."
    // We can't directly assert the log output here without a more complex logging mock/capture.
    // We primarily check that the server remains running.
    std::cout << "Waiting to observe server timeout logging (check console output)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3)); 

    EXPECT_TRUE(local_server.is_running()) << "Server should still be running after potential timeouts."; //
    local_server.stop(); //
}


TEST_F(ServerTest, HandleEchoRequest) {
    Server echo_server(SERVER_TEST_PORT + 2);
    ASSERT_TRUE(echo_server.start()); //
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Give server a moment to start

    Client dummy_client(LOCALHOST_IP, SERVER_TEST_PORT + 2);
    ASSERT_TRUE(dummy_client.connect_to_server());

    netcode::Buffer send_buf;
    netcode::PacketHeader header_send;
    header_send.type = netcode::MessageType::ECHO_REQUEST; //
    header_send.sequenceNumber = 789; //
    send_buf.write_header(header_send);
    std::string payload = "Echo this message!";
    send_buf.write_string(payload);

    ASSERT_TRUE(dummy_client.send_packet(send_buf));

    netcode::Buffer recv_buf;
    int bytes_received = -1;
    // Try a few times to receive, client has 1s timeout, server should respond quickly
    for (int i=0; i<5; ++i) {
        bytes_received = dummy_client.receive_packet(recv_buf, 1024);
        if (bytes_received > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait a bit if timeout (0)
    }
    
    ASSERT_GT(bytes_received, 0) << "Client did not receive an echo response.";

    netcode::PacketHeader header_recv = recv_buf.read_header();
    EXPECT_EQ(header_recv.type, netcode::MessageType::ECHO_RESPONSE); //
    EXPECT_EQ(header_recv.sequenceNumber, header_send.sequenceNumber);
    std::string received_payload = recv_buf.read_string();
    EXPECT_EQ(received_payload, payload);

    dummy_client.disconnect_from_server();
    echo_server.stop(); //
}

TEST_F(ServerTest, HandleUnknownPacketType) {
    Server test_server(SERVER_TEST_PORT + 3);
    ASSERT_TRUE(test_server.start()); //
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    Client dummy_client(LOCALHOST_IP, SERVER_TEST_PORT + 3);
    ASSERT_TRUE(dummy_client.connect_to_server());

    netcode::Buffer send_buf;
    netcode::PacketHeader header_send;
    header_send.type = netcode::MessageType::UNDEFINED; //
    header_send.sequenceNumber = 101; //
    send_buf.write_header(header_send);
    send_buf.write_string("Unknown payload");

    ASSERT_TRUE(dummy_client.send_packet(send_buf));

    // Server should log "Received unknown packet type"
    // We expect no response from the server for an UNKNOWN type.
    // The client's receive should timeout.
    netcode::Buffer recv_buf;
    EXPECT_EQ(dummy_client.receive_packet(recv_buf, 1024), 0) << "Client should not receive a response for UNKNOWN packet type (timeout).";
    
    // Check if server is still running (it shouldn't crash)
    EXPECT_TRUE(test_server.is_running()); //

    dummy_client.disconnect_from_server();
    test_server.stop(); //
}
TEST_F(ServerTest, BroadcastServerAnnouncement) {
    Server broadcast_server(SERVER_TEST_PORT + 4);
    ASSERT_TRUE(broadcast_server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Give server time to start

    // Client 1
    Client client1(LOCALHOST_IP, SERVER_TEST_PORT + 4);
    ASSERT_TRUE(client1.connect_to_server());
    netcode::Buffer init_msg1_client; // Use a different buffer name to avoid conflict if any
    netcode::PacketHeader init_header1;
    init_header1.type = netcode::MessageType::ECHO_REQUEST;
    init_header1.sequenceNumber = 1;
    init_msg1_client.write_header(init_header1);
    client1.send_packet(init_msg1_client); // Send something so server knows about client1
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow server to process

    // Client 2
    Client client2(LOCALHOST_IP, SERVER_TEST_PORT + 4);
    ASSERT_TRUE(client2.connect_to_server());
    netcode::Buffer init_msg2_client; // Use a different buffer name
    netcode::PacketHeader init_header2;
    init_header2.type = netcode::MessageType::ECHO_REQUEST;
    init_header2.sequenceNumber = 2;
    init_msg2_client.write_header(init_header2);
    client2.send_packet(init_msg2_client); // Send something so server knows about client2
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow server to process

    // Clear any echo responses from clients' buffers
    netcode::Buffer temp_clear_buf;
    if (client1.is_connected()) client1.receive_packet(temp_clear_buf, 1024);
    if (client2.is_connected()) client2.receive_packet(temp_clear_buf, 1024);

    // Prepare the announcement buffer manually
    std::string announcement_msg_text = "Server is broadcasting via send_to_all_clients!";
    netcode::Buffer announcement_buffer_to_send;
    netcode::PacketHeader announcement_header;
    announcement_header.type = netcode::MessageType::SERVER_ANNOUNCEMENT;
    announcement_header.sequenceNumber = 0; // Or some relevant sequence

    announcement_buffer_to_send.write_header(announcement_header);
    announcement_buffer_to_send.write_string(announcement_msg_text);

    // Use the new method: send_to_all_clients
    broadcast_server.send_to_all_clients(announcement_buffer_to_send); //
    std::cout << "Broadcast (send_to_all_clients) initiated. Allowing time for clients to receive..." << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Give time for packets to arrive

    // Check Client 1
    netcode::Buffer recv_buf1;
    int bytes1 = client1.receive_packet(recv_buf1, 1024);
    ASSERT_GT(bytes1, 0) << "Client 1 did not receive broadcast.";
    if (bytes1 > 0) {
        netcode::PacketHeader header1 = recv_buf1.read_header();
        EXPECT_EQ(header1.type, netcode::MessageType::SERVER_ANNOUNCEMENT);
        std::string msg1 = recv_buf1.read_string();
        EXPECT_EQ(msg1, announcement_msg_text);
    }

    // Check Client 2
    netcode::Buffer recv_buf2;
    int bytes2 = client2.receive_packet(recv_buf2, 1024);
    ASSERT_GT(bytes2, 0) << "Client 2 did not receive broadcast.";
    if (bytes2 > 0) {
        netcode::PacketHeader header2 = recv_buf2.read_header();
        EXPECT_EQ(header2.type, netcode::MessageType::SERVER_ANNOUNCEMENT);
        std::string msg2 = recv_buf2.read_string();
        EXPECT_EQ(msg2, announcement_msg_text);
    }

    client1.disconnect_from_server();
    client2.disconnect_from_server();
    broadcast_server.stop();
}

// Attempt to start two servers on the same port to test bind failure
TEST_F(ServerTest, StartServerPortInUse) {
    Server server1(SERVER_TEST_PORT + 5);
    ASSERT_TRUE(server1.start()) << "First server failed to start."; //
    EXPECT_TRUE(server1.is_running()); //

    Server server2(SERVER_TEST_PORT + 5); // Same port
    EXPECT_FALSE(server2.start()) << "Second server should fail to start on an already bound port."; //
    EXPECT_FALSE(server2.is_running()); //

    server1.stop(); //
}