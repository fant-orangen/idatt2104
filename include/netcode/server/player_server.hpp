#pragma once
#include "netcode/server.hpp"
#include "netcode/packets/player_state_packet.hpp"
#include <memory>
#include <atomic>
#include <map>
#include <mutex>

namespace netcode {

    class PlayerServer {
    public:
        PlayerServer(int port);
        ~PlayerServer();

        bool start();
        void stop();
        bool is_running() const;

        void update();

        std::map<uint32_t, packets::PlayerStatePacket> get_plater_states() const;

        using PlayerStateCallback = std::function<void(const packets::PlayerStatePacket&)>;
        void set_player_state_callback(PlayerStateCallback callback);

    private:
        std::unique_ptr<Server> server_;
        int port_;
        std::atomic<uint32_t> next_broadcast_seq_{0};
        std::mutex players_mutex_;
        std::map<uint32_t, packets::PlayerStatePacket> player_states_;
        PlayerStateCallback player_state_callback_;

        void handle_packet(const Buffer& buffer, const struct sockaddr_in& client_addr);

        void handle_player_state(const packets::PlayerStatePacket& state, const struct sockaddr_in& client_addr);

        void broadcast_player_states(const packets::PlayerStatePacket& state);

    };
}