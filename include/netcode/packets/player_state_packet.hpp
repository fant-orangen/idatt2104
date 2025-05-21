#pragma once
#include <cstdint>

namespace netcode::packets {

    struct PlayerStatePacket {
        uint32_t player_id;
        float x;
        float y;
        float z;
        float velocity_y;
        bool is_jumping;
    };

    struct PlayerMovementRequest {
        uint32_t player_id;
        float movement_x;
        float movement_y;
        float movement_z;
        float velocity_y;
        bool is_jumping;
    };

    struct TimestampedPlayerStatePacket {
        std::chrono::steady_clock::time_point timestamp;
        PlayerStatePacket player_state;
    };

    struct TimestampedPlayerMovementRequest {
        std::chrono::steady_clock::time_point timestamp;
        PlayerMovementRequest player_movement_request;
    };
}
