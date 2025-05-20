#pragma once
#include "netcode/utils/logger.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

namespace netcode::utils {


    class NetworkLogger {
    public:
        static void log_packer_sent(const std::string& component, const struct sockaddr_in& dest_addr,
                                    size_t size, uint8_t packet_type, uint32_t seq_num);

        static void log_packer_received(const std::string& component, const struct sockaddr_in& src_addr,
                                        size_t size, uint8_t packet_type, uint32_t seq_num);

        static void log_connection_event(const std::string& component, const std::string& event,
                                        const std::string& ip, int port);

        static void log_network_error(const std::string& component, const std::string& error_msg,
                                     const std::string& operation);

    private:
        static std::string address_to_string(const struct sockaddr_in& addr);
    };

}
