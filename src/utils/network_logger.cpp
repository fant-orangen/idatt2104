#include "netcode/utils/network_logger.hpp"

namespace netcode::utils {

    // Formats and logs packet sending events with format:
    // "Sent packet to IP:PORT | Type: X | Seq: Y | Size: Z bytes"
    void NetworkLogger::log_packet_sent(const std::string& component, const struct sockaddr_in& dest_addr,
                                        size_t size, uint8_t packet_type, uint32_t seq_num) {
        std::stringstream ss;
        ss << "Sent packet to " << address_to_string(dest_addr)
           << " | Type: " << static_cast<int>(packet_type)
           << " | Seq: " << seq_num
           << " | Size: " << size << " bytes";

        Logger::get_instance().info(ss.str(), component);
    }

    // Formats and logs packet receiving events with format:
    // "Received packet from IP:PORT | Type: X | Seq: Y | Size: Z bytes"
    void NetworkLogger::log_packet_received(const std::string &component, const struct sockaddr_in &src_addr,
                                        size_t size, uint8_t packet_type, uint32_t seq_num) {
        std::stringstream ss;
        ss << "Received packet from " << address_to_string(src_addr)
           << " | Type: " << static_cast<int>(packet_type)
           << " | Seq: " << seq_num
           << " | Size: " << size << " bytes";

        Logger::get_instance().info(ss.str(), component);
    }

    // Formats connection events with format: "EVENT - IP:PORT"
    void NetworkLogger::log_connection_event(const std::string &component, const std::string& event,
                                    const std::string& ip, int port) {
        std::stringstream ss;
        ss << event << " - " << ip << ":" << port;

        Logger::get_instance().info(ss.str(), component);
    }

    // Formats network errors with format: "Error during: OPERATION: ERROR_MESSAGE"
    void NetworkLogger::log_network_error(const std::string &component, const std::string &error_msg,
                                const std::string &operation) {
        std::stringstream ss;
        ss << "Error during: " << operation << ": " << error_msg;

        Logger::get_instance().error(ss.str(), component);
    }

    // Converts sockaddr_in to human-readable "IP:PORT" format
    // Uses inet_ntop for thread-safe IP address conversion
    std::string NetworkLogger::address_to_string(const struct sockaddr_in &addr) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
        return std::string(ip_str) + ":" + std::to_string(ntohs(addr.sin_port));
    }

}
