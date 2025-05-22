#pragma once
#include "netcode/utils/logger.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

namespace netcode::utils {


    /**
     * @brief A utility class for logging network-related events and operations
     *
     * The NetworkLogger provides static methods to log various network events including
     * packet transmission, reception, connection events and errors. It helps in debugging
     * and monitoring network operations.
     */
    class NetworkLogger {
    public:
        /**
         * @brief Logs information about a packet that was sent
         * @param component The name of the component sending the packet
         * @param dest_addr The destination address structure
         * @param size Size of the packet in bytes
         * @param packet_type Type identifier of the packet
         * @param seq_num Sequence number of the packet
         */
        static void log_packet_sent(const std::string& component, const struct sockaddr_in& dest_addr,
                                    size_t size, uint8_t packet_type, uint32_t seq_num);

        /**
         * @brief Logs information about a packet that was received
         * @param component The name of the component receiving the packet
         * @param src_addr The source address structure
         * @param size Size of the packet in bytes
         * @param packet_type Type identifier of the packet
         * @param seq_num Sequence number of the packet
         */
        static void log_packet_received(const std::string& component, const struct sockaddr_in& src_addr,
                                        size_t size, uint8_t packet_type, uint32_t seq_num);

        /**
         * @brief Logs connection-related events
         * @param component The name of the component reporting the event
         * @param event Description of the connection event
         * @param ip IP address involved in the connection event
         * @param port Port number involved in the connection event
         */
        static void log_connection_event(const std::string& component, const std::string& event,
                                        const std::string& ip, int port);

        /**
         * @brief Logs network-related errors
         * @param component The name of the component where the error occurred
         * @param error_msg Description of the error
         * @param operation The network operation that failed
         */
        static void log_network_error(const std::string& component, const std::string& error_msg,
                                     const std::string& operation);

    private:
        /**
         * @brief Converts a sockaddr_in structure to a readable string
         * @param addr The address structure to convert
         * @return String representation of the address
         */
        static std::string address_to_string(const struct sockaddr_in& addr);
    };

}