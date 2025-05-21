#pragma once
#include "netcode/client.hpp"
#include "netcode/packets/player_state_packet.hpp"
#include <string>
#include <memory>
#include <functional>
#include <atomic>

namespace netcode {

    class PlayerClient {

    public:
        PlayerClient(const std::string& server_ip, int port);
        ~PlayerClient();

        bool connect();
        void disconnect();
        bool is_connected() const;

        bool send_player_state(const packets::PlayerStatePacket& state);

        void update();

        using PlayerStateCallback = std::function<void(const packets::PlayerStatePacket&)>;
        void set_player_state_callback(PlayerStateCallback callback);

    private:
        std::unique_ptr<Client> client_;
        std::string server_ip_;
        int port_;
        std::atomic<uint32_t> next_seq_num_{0};
        PlayerStateCallback player_state_callback_;

        void handle_packet(const Buffer& buffer);

    };
}
