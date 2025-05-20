#include "netcode/client/player_client.hpp"
#include "netcode/serialization.hpp"
#include "netcode/packet_types.hpp"
#include "netcode/serialization/player_state_serializer.hpp"
#include "netcode/utils/logger.hpp"
#include "netcode/visualization/player.hpp"


namespace netcode {
    PlayerClient::PlayerClient(const std::string &server_ip, int port)
        : server_ip_(server_ip), port_(port) {
            client_ = std::make_unique<Client>(server_ip, port);
    }


    PlayerClient::~PlayerClient() {
        disconnect();
    }

    bool PlayerClient::connect() {
        return client_->connect_to_server();
    }

    void PlayerClient::disconnect() {
        if (client_ && client_->is_connected()) {
            client_->disconnect_from_server();
        }
    }

    bool PlayerClient::is_connected() const {
        return client_ && client_->is_connected();
    }

    bool PlayerClient::send_player_state(const packets::PlayerStatePacket& state) {
        if (!is_connected()) {
            utils::Logger::get_instance().LOG_WARNING("Could not send player condition: not connected", "PlayerClient");
            return false;
        }

        Buffer buffer;
        PacketHeader header;
        header.type = MessageType::PLAYER_STATE_UPDATE;
        header.sequenceNumber = next_seq_num_++;

        serialize(buffer, header);
        serialization::serialize(buffer, state);

        utils::Logger::get_instance().LOG_DEBUG("Sending player state packet: pos=(" +
              std::to_string(state.x) + "," +
              std::to_string(state.y) + "," +
              std::to_string(state.z) + "), jumping=" +
              (state.is_jumping ? "true" : "false"),"PlayerClient");

        return client_->send_packet(buffer);
    }

    void PlayerClient::update() {
        if (!is_connected()) return;

        Buffer buffer;

        int bytes_received = client_->receive_packet(buffer, 1024);

        if (bytes_received > 0) {
            handle_packet(buffer);
        }
    }

    void PlayerClient::set_player_state_callback(PlayerStateCallback callback) {
        player_state_callback_ = callback;
    }

    void PlayerClient::handle_packet(const Buffer& buffer) {
        Buffer temp_buffer = buffer;

        PacketHeader header;

        if (!try_deserialize(temp_buffer, header)) {
            utils::Logger::get_instance().LOG_WARNING("Could not deserialize packet header", "PlayerClient");
            return;
        }

        switch (header.type) {
            case MessageType::PLAYER_STATE_UPDATE: {
                packets::PlayerStatePacket state;
                if (serialization::deserialize(temp_buffer, state)) {
                    utils::Logger::get_instance().LOG_DEBUG("Received condition for player " +
                        std::to_string(state.player_id), "PlayerClient");

                    if (player_state_callback_) {
                        player_state_callback_(state);
                    }
                } else {
                    utils::Logger::get_instance().LOG_WARNING("Could not deserialize player state packet", "PlayerClient");
                }
                break;
            }
            default:
                utils::Logger::get_instance().LOG_DEBUG("Received unhandled packet type: " +
                    std::to_string(static_cast<int>(header.type)), "PlayerClient");
                break;
        }
    }

}