#ifndef GAME_DATA_TYPES_HPP
#define GAME_DATA_TYPES_HPP

#include "netcode/math/my_vec3.hpp"
#include <cstdint>

struct PlayerInput {
    uint32_t frame_number;
    MyVec3 movement_input_vector;
    bool jump_action_triggered;
};

struct PlayerFrameState {
    uint32_t frame_number;
    uint32_t player_id;

    MyVec3 position;
    MyVec3 velocity;
    bool is_jumping_state;
    bool states_match(const PlayerFrameState& other) const {
        return frame_number == other.frame_number &&
               player_id == other.player_id &&
               position == other.position &&
               velocity == other.velocity &&
               is_jumping_state == other.is_jumping_state;
};

#endif