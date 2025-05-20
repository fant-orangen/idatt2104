#include "gtest/gtest.h"
#include "netcode/client.hpp" //
#include "netcode/serialization.hpp" // For netcode::Buffer
#include "netcode/packet_types.hpp"  // For PacketHeader, MessageType

#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds
#include <sys/socket.h> // For socket functions if creating a dummy server
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h>  // For inet_addr
#include <unistd.h>     // For close (dummy server)

// A known good IP for loopback tests
const std::string LOCALHOST_IP = "127.0.0.1";
const int TEST_PORT = 12345; // An arbitrary port for testing
const int INVALID_PORT = -1;


class ClientTest : public ::testing::Test {
protected:
    Client client_{LOCALHOST_IP, TEST_PORT}; //
    netcode::Buffer send_buffer_;
    netcode::Buffer recv_buffer_;

    // Helper to run a simple UDP echo server for a short duration
    // Returns the port it managed to bind to, or -1 on failure
    int run_dummy_echo_server(uint16_t port, int& server_socket_fd, bool& running_flag, std::thread& server_thread) {
        server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (server_socket_fd < 0) {
            perror("Dummy server socket creation failed");
            return -1;
        }

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on any interface for simplicity
        serv_addr.sin_port = htons(port);

        if (bind(server_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            // If specific port is busy, try ephemeral port
            if (port != 0) {
                std::cerr << "Dummy server bind failed on port " << port << ", trying ephemeral." << std::endl;
                serv_addr.sin_port = htons(0); // OS assigns port
                 if (bind(server_socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                    perror("Dummy server bind failed even on ephemeral port");
                    close(server_socket_fd);
                    server_socket_fd = -1;
                    return -1;
                 }
            } else {
                perror("Dummy server bind failed");
                close(server_socket_fd);
                server_socket_fd = -1;
                return -1;
            }
        }
        
        socklen_t len = sizeof(serv_addr);
        if (getsockname(server_socket_fd, (struct sockaddr *)&serv_addr, &len) == -1) {
            perror("getsockname");
            close(server_socket_fd);
            server_socket_fd = -1;
            return -1;
        }
        uint16_t actual_port = ntohs(serv_addr.sin_port);
        std::cout << "Dummy server listening on port: " << actual_port << std::endl;


        // Set a short timeout for the server's recvfrom
        struct timeval tv;
        tv.tv_sec = 2; // Server waits up to 2 seconds for a packet
        tv.tv_usec = 0;
        setsockopt(server_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        server_thread = std::thread([&, server_socket_fd, actual_port] {
            char buffer[1024];
            struct sockaddr_in client_addr_echo;
            socklen_t client_len_echo = sizeof(client_addr_echo);
            std::cout << "Dummy server thread started for port " << actual_port << std::endl;
            while (running_flag) {
                ssize_t n = recvfrom(server_socket_fd, buffer, sizeof(buffer) -1, 0,
                                     (struct sockaddr *)&client_addr_echo, &client_len_echo);
                if (n > 0) {
                    std::cout << "Dummy server received " << n << " bytes. Echoing back." << std::endl;
                    sendto(server_socket_fd, buffer, n, 0,
                           (struct sockaddr *)&client_addr_echo, client_len_echo);
                } else if (n == 0) {
                     // std::cout << "Dummy server recvfrom returned 0." << std::endl;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // Timeout, check running_flag and continue
                        if (!running_flag) break;
                        continue;
                    }
                    // perror("Dummy server recvfrom error"); // Can be noisy
                    if (!running_flag) break; // Exit if stop signaled during error
                }
                 // Brief sleep to prevent busy-waiting if running_flag is the only escape
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            std::cout << "Dummy server thread stopping for port " << actual_port << std::endl;
        });
        return actual_port;
    }

    void stop_dummy_server(int& server_socket_fd, bool& running_flag, std::thread& server_thread) {
        running_flag = false;
        if (server_socket_fd >= 0) {
            //shutdown(server_socket_fd, SHUT_RDWR); // Help unblock recvfrom
            close(server_socket_fd);
            server_socket_fd = -1;
        }
        if (server_thread.joinable()) {
            server_thread.join();
        }
         std::cout << "Dummy server fully stopped." << std::endl;
    }
};

TEST_F(ClientTest, InitialState) {
    EXPECT_FALSE(client_.is_connected()) << "Client should not be connected initially."; //
}

TEST_F(ClientTest, ConnectToServerSuccess) {
    EXPECT_TRUE(client_.connect_to_server()) << "Should connect successfully to a valid setup."; //
    EXPECT_TRUE(client_.is_connected()) << "Client should be connected after successful connect_to_server."; //
    client_.disconnect_from_server(); //
}

TEST_F(ClientTest, ConnectToServerInvalidIpFormat) {
    Client invalid_client("this.is.not.an.ip", TEST_PORT);
    EXPECT_FALSE(invalid_client.connect_to_server()) << "Connection should fail with invalid IP format."; //
    EXPECT_FALSE(invalid_client.is_connected()); //
}


TEST_F(ClientTest, DisconnectFromServer) {
    client_.connect_to_server(); //
    ASSERT_TRUE(client_.is_connected()) << "Client should be connected before disconnect."; //
    client_.disconnect_from_server(); //
    EXPECT_FALSE(client_.is_connected()) << "Client should be disconnected after disconnect_from_server."; //
}

TEST_F(ClientTest, DisconnectWhenNotConnected) {
    EXPECT_FALSE(client_.is_connected()); //
    // disconnect_from_server() should not throw or cause issues if called when not connected.
    ASSERT_NO_THROW(client_.disconnect_from_server()); //
    EXPECT_FALSE(client_.is_connected()); //
}

TEST_F(ClientTest, SendPacketWhenNotConnected) {
    netcode::Buffer buffer;
    buffer.write_uint32(123); // Some dummy data
    EXPECT_FALSE(client_.send_packet(buffer)) << "send_packet should fail if not connected."; //
}

TEST_F(ClientTest, ReceivePacketWhenNotConnected) {
    netcode::Buffer buffer;
    EXPECT_EQ(client_.receive_packet(buffer, 1024), -1) << "receive_packet should return -1 if not connected."; //
}

TEST_F(ClientTest, ReceivePacketTimeout) {
    // Connect client to a non-existent server or a port where nobody is sending
    // The client has a 1-second receive timeout set in connect_to_server()
    Client client_for_timeout(LOCALHOST_IP, TEST_PORT + 10); // Use a different port
    ASSERT_TRUE(client_for_timeout.connect_to_server()); //
    
    netcode::Buffer buffer;
    // This relies on the 1-second timeout set in Client::connect_to_server
    EXPECT_EQ(client_for_timeout.receive_packet(buffer, 1024), 0) << "receive_packet should return 0 on timeout."; //
    client_for_timeout.disconnect_from_server(); //
}


// More complex test: Send and Receive with a dummy local server
TEST_F(ClientTest, SendAndReceivePacketLoopback) {
    int server_fd = -1;
    bool server_running = true;
    std::thread dummy_server_thread;
    
    // Attempt to start dummy server on an OS-chosen ephemeral port
    uint16_t actual_server_port = run_dummy_echo_server(0, server_fd, server_running, dummy_server_thread);
    ASSERT_NE(actual_server_port, static_cast<uint16_t>(-1)) << "Dummy server failed to start.";
    ASSERT_NE(actual_server_port, 0) << "Dummy server got port 0, which is unexpected.";


    Client test_client(LOCALHOST_IP, actual_server_port);
    ASSERT_TRUE(test_client.connect_to_server()) << "Client failed to connect to dummy server port " << actual_server_port; //

    send_buffer_.clear();
    netcode::PacketHeader header_send;
    header_send.type = netcode::MessageType::ECHO_REQUEST;
    header_send.sequenceNumber = 123;
    send_buffer_.write_header(header_send);
    std::string test_payload = "Hello Server from ClientTest!";
    send_buffer_.write_string(test_payload);

    EXPECT_TRUE(test_client.send_packet(send_buffer_)) << "Client should send packet successfully."; //

    // Give server a moment to process and echo
    // The client has a 1s timeout, so this should be well within limits.
    // The dummy server also has a timeout, ensure it's longer or client sends quickly.
    int bytes_received = test_client.receive_packet(recv_buffer_, 1024); //
    EXPECT_GT(bytes_received, 0) << "Client should receive bytes from echo server.";

    if (bytes_received > 0) {
        EXPECT_EQ(static_cast<size_t>(bytes_received), send_buffer_.get_size()) << "Received packet size should match sent packet size.";
        
        netcode::PacketHeader header_recv = recv_buffer_.read_header();
        EXPECT_EQ(header_recv.type, header_send.type);
        EXPECT_EQ(header_recv.sequenceNumber, header_send.sequenceNumber);

        std::string received_payload = recv_buffer_.read_string();
        EXPECT_EQ(received_payload, test_payload);
    }
    
    test_client.disconnect_from_server(); //
    stop_dummy_server(server_fd, server_running, dummy_server_thread);
}


TEST_F(ClientTest, SendPacketContentCheckLoopback) {
    int server_fd = -1;
    bool server_running = true;
    std::thread dummy_server_thread;
    uint16_t actual_server_port = run_dummy_echo_server(0, server_fd, server_running, dummy_server_thread);
    ASSERT_NE(actual_server_port, static_cast<uint16_t>(-1)) << "Dummy server failed to start.";
     ASSERT_NE(actual_server_port, 0) << "Dummy server got port 0, which is unexpected.";

    Client test_client(LOCALHOST_IP, actual_server_port);
    ASSERT_TRUE(test_client.connect_to_server()); //

    send_buffer_.clear();
    uint32_t test_val1 = 12345;
    std::string test_val2 = "Another Test";
    send_buffer_.write_uint32(test_val1);
    send_buffer_.write_string(test_val2);
    
    size_t sent_size = send_buffer_.get_size();

    EXPECT_TRUE(test_client.send_packet(send_buffer_)); //

    int bytes_received = test_client.receive_packet(recv_buffer_, 1024); //
    EXPECT_EQ(static_cast<size_t>(bytes_received), sent_size);

    if (static_cast<size_t>(bytes_received) == sent_size) {
        EXPECT_EQ(recv_buffer_.read_uint32(), test_val1);
        EXPECT_EQ(recv_buffer_.read_string(), test_val2);
    }
    
    test_client.disconnect_from_server(); //
    stop_dummy_server(server_fd, server_running, dummy_server_thread);
}