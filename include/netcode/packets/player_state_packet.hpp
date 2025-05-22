#pragma once
#include <cstdint>
#include <chrono>
#include <netinet/in.h> // For sockaddr_in

namespace netcode::packets {

    /**
     * @struct PlayerStatePacket
     * @brief Represents a player's current state including position and movement
     * @details Used by server to broadcast player states to all clients
     */
    struct PlayerStatePacket {
        uint32_t player_id;    ///< Unique identifier for the player
        float x;               ///< X coordinate position
        float y;               ///< Y coordinate position
        float z;               ///< Z coordinate position
        float velocity_y;      ///< Current vertical velocity (for jumping/falling)
        bool is_jumping;       ///< Whether player is currently jumping
        uint32_t last_processed_input_sequence; ///< Sequence number of the last input that was processed
    };

    /**
     * @struct PlayerMovementRequest
     * @brief Represents a player's movement input request
     * @details Sent by clients to request movement from server
     */
    struct PlayerMovementRequest {
        uint32_t player_id;    ///< Unique identifier for the player
        float movement_x;      ///< Requested movement in X direction
        float movement_y;      ///< Requested movement in Y direction
        float movement_z;      ///< Requested movement in Z direction
        float velocity_y;      ///< Current vertical velocity (for jumping/falling)
        bool is_jumping;       ///< Whether jump is being requested
        uint32_t input_sequence_number; ///< Client-side sequence number for this input
    };

    /**
     * @struct TimestampedPlayerStatePacket
     * @brief Player state packet with timestamp for network delay simulation
     * @details Used in client-side prediction and server reconciliation
     */
    struct TimestampedPlayerStatePacket {
        std::chrono::steady_clock::time_point timestamp; ///< When the state was recorded
        PlayerStatePacket player_state;                  ///< The player state data
    };

    /**
     * @struct TimestampedPlayerMovementRequest
     * @brief Movement request with timestamp for network delay simulation
     * @details Used in client-side prediction and server reconciliation
     */
    struct TimestampedPlayerMovementRequest {
        std::chrono::steady_clock::time_point timestamp; ///< When the request was made
        PlayerMovementRequest player_movement_request;   ///< The movement request data
        sockaddr_in clientAddr;                         ///< Client address that sent this request
    };
}
