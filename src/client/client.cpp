/**
 * @file client.cpp
 * @brief Implementation of the Client class for UDP communication.
 *
 * This file contains the method definitions for the Client class, which
 * provides functionality to connect to a UDP server, send data, and
 * receive data.
 */

#include "netcode/client.hpp" // Corresponding header file
#include <iostream>             // For std::cerr, std::endl, std::cout
#include <cstring>              // For strerror, memset, strlen
#include <cerrno>               // For errno

/**
 * @brief Constructs a new Client object.
 *
 * Initializes the client with the server's IP address and port.
 * The client is not connected upon construction.
 *
 * @param server_ip The IP address of the server to connect to.
 * @param port The port number of the server.
 */
Client::Client(const std::string& server_ip, int port)
    : server_ip_(server_ip), port_(port), connected_(false), socket_fd_(-1) {}

/**
 * @brief Destroys the Client object.
 *
 * Ensures that the client is disconnected and the socket is closed
 * when the object goes out of scope.
 */
Client::~Client() {
    disconnect();
}

/**
 * @brief Establishes a connection to the UDP server.
 *
 * Creates a UDP socket and initializes the server address structure
 * with the IP address and port provided during construction.
 *
 * @return `true` if the socket was created and server address was successfully
 * initialized, `false` otherwise. Note: For UDP, "connect" doesn't
 * establish a persistent connection like TCP; this method primarily
 * sets up the socket and server address for future send/receive operations.
 */
bool Client::connect() {
    // Create UDP socket
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Initialize server address structure
    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_); // Convert port to network byte order

    // Convert IP address from string (presentation format) to binary (network format)
    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr) <= 0) {
        std::cerr << "Invalid address or address family not supported: " << server_ip_ << " (" << strerror(errno) << ")" << std::endl;
        close(socket_fd_); // Clean up the created socket
        socket_fd_ = -1;   // Mark socket as invalid
        return false;
    }

    // Note: For UDP, `connect()` can be used to set a default destination address
    // for `send()` calls, but `sendto()` is used here, which specifies the
    // destination each time. So, this `connected_` flag mainly indicates
    // that the socket is initialized and ready for `sendto`/`recvfrom`.

    connected_ = true;
    std::cout << "Client initialized for server " << server_ip_ << ":" << port_ << std::endl;
    return true;
}

/**
 * @brief Disconnects from the server.
 *
 * Sends an optional "DISCONNECT" message to the server (best-effort for UDP)
 * and closes the client's socket.
 */
void Client::disconnect() {
    if (socket_fd_ >= 0) { // Check if the socket is valid
        // Optionally, send a disconnection message to the server.
        // This is a "best-effort" send for UDP, as there's no guarantee of delivery.
        if (connected_) {
            const char* disconnect_msg = "DISCONNECT";
            sendto(socket_fd_, disconnect_msg, strlen(disconnect_msg), 0,
                 (struct sockaddr*)&server_addr_, sizeof(server_addr_));
        }

        close(socket_fd_);  // Close the socket descriptor
        socket_fd_ = -1;    // Mark socket as invalid
        connected_ = false; // Update connection status
        std::cout << "Client disconnected" << std::endl;
    }
}

/**
 * @brief Checks if the client is currently connected (socket initialized).
 *
 * @return `true` if the client has a valid socket descriptor and considers
 * itself connected, `false` otherwise.
 */
bool Client::is_connected() const {
    return connected_ && (socket_fd_ >= 0);
}

/**
 * @brief Sends data to the connected UDP server.
 *
 * @param data Pointer to the data buffer to send.
 * @param size The number of bytes to send from the data buffer.
 * @return `true` if the data was sent successfully (all bytes were queued for sending),
 * `false` if the client is not connected or an error occurred during sending.
 */
bool Client::send_data(const void* data, size_t size) {
    if (!is_connected()) {
        std::cerr << "Cannot send data: client not connected" << std::endl;
        return false;
    }

    // Send data using sendto, specifying the server address
    ssize_t bytes_sent = sendto(socket_fd_, data, size, 0,
                              (struct sockaddr*)&server_addr_, sizeof(server_addr_));

    if (bytes_sent < 0) {
        std::cerr << "Error sending data: " << strerror(errno) << std::endl;
        return false;
    }

    // Check if all data was sent (for UDP, this usually means it was accepted by the OS network stack)
    return static_cast<size_t>(bytes_sent) == size;
}

/**
 * @brief Receives data from the UDP server.
 *
 * Attempts to receive data into the provided buffer. This method uses `recvfrom`
 * and includes a socket timeout (SO_RCVTIMEO) of 500ms to prevent blocking indefinitely.
 *
 * @param buffer Pointer to the buffer where received data will be stored.
 * @param buffer_size The maximum number of bytes to read into the buffer.
 * @return The number of bytes received, or 0 if a timeout occurred,
 * or -1 if an error occurred (other than timeout).
 */
int Client::receive_data(void* buffer, size_t buffer_size) {
    if (!is_connected()) {
        std::cerr << "Cannot receive data: client not connected" << std::endl;
        return -1;
    }

    // Set up a timeout for the receive operation
    struct timeval tv;
    tv.tv_sec = 0;        // 0 seconds
    tv.tv_usec = 500000;  // 500,000 microseconds = 500 milliseconds

    // Apply the timeout option to the socket
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
        // Continue without timeout or handle as a critical error depending on requirements
    }

    struct sockaddr_in from_addr; // To store the sender's address (though for a client, it's expected to be the server)
    socklen_t from_len = sizeof(from_addr);

    // Attempt to receive data
    ssize_t bytes_received = recvfrom(socket_fd_, buffer, buffer_size, 0,
                                     (struct sockaddr*)&from_addr, &from_len);

    if (bytes_received < 0) {
        // Check for timeout (EAGAIN or EWOULDBLOCK)
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // Timeout occurred, no data received within the timeout period
        }
        // Another error occurred
        std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
        return -1; // Indicate an error
    }

    return static_cast<int>(bytes_received); // Return the number of bytes received
}