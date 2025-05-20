#include "netcode/server/player_server.hpp"
#include "netcode/serialization.hpp"
#include "netcode/packet_types.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/serialization/player_state_serializer.hpp"
#include <string>

namespace netcode {
    PlayerServer::PlayerServer(int port) : port_(port) {
        server_ = std::make_unique<Server>(port);
    }

    PlayerServer::~PlayerServer() {
        stop();
    }

    bool PlayerServer::start() {
        return server_->start();
    }

    void PlayerServer::stop() {
        if (server_ && server_->is_running()) {
            server_->stop();
        }
    }

    bool PlayerServer::is_running() const {
        return server_ && server_->is_running();
    }

    void PlayerServer::update() {
        if (!is_running()) return;

        server_->remove_inactive_clients(60);

        Buffer buffer;

        struct sockaddr_in client_addr;

        int bytes_received = server_->receive_packet(buffer, 1024, client_addr);

        if (bytes_received > 0) {
            handle_packet(buffer, client_addr);
        }
    }

    std::map<uint32_t, packets::PlayerStatePacket> PlayerServer::get_plater_states() const {
        std::lock_guard<std::mutex> lock(players_mutex_);
        return player_states_;
    }

    void PlayerServer::set_player_state_callback(PlayerStateCallback callback) {
        player_state_callback_ = callback;
    }

    void PlayerServer::handle_packet(const Buffer& buffer, const struct sockaddr_in& client_addr) {
        Buffer temp_buffer = buffer;

        PacketHeader header;

        if (!try_deserialize(temp_buffer, header)) {
            utils::Logger::get_instance().LOG_WARNING("Could not deserialize packet header", "PlayerServer");
            return;
        }

        switch (header.type) {
            case MessageType::PLAYER_STATE_UPDATE: {
                packets::PlayerStatePacket state;
                if (serialization::deserialize(temp_buffer, state)) {
                    handle_player_state(state, client_addr);
                } else {
                    utils::Logger::get_instance().LOG_WARNING("Could not deserialize player state packet", "PlayerServer");
                }
                break;
            }

            default:
                utils::Logger::get_instance().LOG_DEBUG("Received unhandled packet type: " +
                    std::to_string(static_cast<int>(header.type)), "PlayerServer");
                break;
        }
    }

    void PlayerServer::handle_player_state(const packets::PlayerStatePacket& state, const struct sockaddr_in& client_addr) {
        utils::Logger::get_instance().LOG_DEBUG("Received player state update for player " +
            std::to_string(state.player_id) +
            ", pos=(" + std::to_string(state.x) + "," +
            std::to_string(state.y) + "," +
            std::to_string(state.z) + ")", "PlayerServer");

        {
            std::lock_guard<std::mutex> lock(players_mutex_);
            player_states_[state.player_id] = state;
        }

        if (player_state_callback_) {
            player_state_callback_(state);
        }

        broadcast_player_states(state);
    }

    void PlayerServer::broadcast_player_states(const packets::PlayerStatePacket &state) {
        Buffer buffer;
        PacketHeader header;
        header.type = MessageType::PLAYER_STATE_UPDATE;
        header.sequenceNumber = next_broadcast_seq_++;

        serialize(buffer, header);
        serialization::serialize(buffer, state);

        server_->send_to_all_clients(buffer);

        utils::Logger::get_instance().LOG_DEBUG("Sent condition for player " +
            std::to_string(state.player_id) +
            " to all clients", "PlayerServer");
    }
}