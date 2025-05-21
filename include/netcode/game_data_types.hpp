#ifndef GAME_DATA_TYPES_HPP
#define GAME_DATA_TYPES_HPP

#include "raylib.h"
#include <cstdint>

struct PlayerInput {
    uint32_t frame_number;
    Vector3 movement_input_vector;
    bool jump_action_triggered;
};

struct PlayerFrameState {
    uint32_t frame_number;
    uint32_t player_id;

    Vector3 position;
    Vector3 velocity;
    bool is_jumping_state;
};

#endif