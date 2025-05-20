#include "netcode/utils/network_logger.hpp"

namespace netcode::utils {

void NetworkLogger::log_packer_sent(const std::string& component, const struct sockaddr_in& dest_addr,
                                    size_t size, uint8_t packet_type, uint32_t seq_num) {
    std::stringstream ss;
    ss << "Sent packet to " << address_to_string(dest_addr)
       << " | Type: " << static_cast<int>(packet_type)
       << " | Seq: " << seq_num
       << " | Size: " << size << " bytes";

    Logger::get_instance().info(ss.str(), component);
}

void NetworkLogger::log_packer_received(const std::string &component, const struct sockaddr_in &src_addr,
                                        size_t size, uint8_t packet_type, uint32_t seq_num) {
    std::stringstream ss;
    ss << "Received packet from " << address_to_string(src_addr)
       << " | Type: " << static_cast<int>(packet_type)
       << " | Seq: " << seq_num
       << " | Size: " << size << " bytes";

    Logger::get_instance().info(ss.str(), component);
}

void NetworkLogger::log_network_error(const std::string &component, const std::string &error_msg,
                                    const std::string &operation) {
    std::stringstream ss;
    ss << "Error while " << operation << ": " << error_msg;

    Logger::get_instance().error(ss.str(), component);
}

std::string NetworkLogger::address_to_string(const struct sockaddr_in &addr) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    return std::string(ip_str) + ":" + std::to_string(ntohs(addr.sin_port));
}

}
