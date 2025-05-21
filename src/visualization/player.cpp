#include "netcode/visualization/player.hpp"
#include "raymath.h"
#include <iostream>

namespace netcode {
namespace visualization {

Player::ModelConfig Player::getModelConfig(PlayerType type) {
    switch (type) {
        case PlayerType::RED_PLAYER:
            return {
                "../assets/cat/12221_Cat_v1_l3.obj",
                0.05f
            };
        case PlayerType::BLUE_PLAYER:
            return {
                "../assets/cat/12221_Cat_v1_l3.obj",
                0.05f
            };
        default:
            return {"", 1.0f};
    }
}

Player::Player(uint32_t id, PlayerType type, bool is_local, const Vector3& startPos, const Color& playerColor)
    : id_(id),
    is_local_(is_local),
    position_(startPos),
    velocity_({0.0f, 0.0f, 0.0f}),
    is_jumping_(false),
    visual_position_(startPos),
    color_(playerColor),
    type_(type),
    scale_(1.0f),
    modelLoaded_(false) {
    loadModel(false);
}

Player::~Player() {
    if (modelLoaded_) {
        UnloadModel(model_);
    }
}

void Player::loadModel(bool useCubes) {
    if (useCubes) {
        modelLoaded_ = false;
        printf("Loading cube model\n");
        return;
    }

    ModelConfig config = getModelConfig(type_);
    if (!config.modelPath[0]) {
        printf("Invalid model configuration\n");
        return;
    }

    printf("Loading model: %s\n", config.modelPath);
    
    // Load the model - Raylib will automatically load associated textures through the .mtl file
    model_ = LoadModel(config.modelPath);
    this->scale_ = config.scale;
    
    if (model_.meshCount > 0 && model_.meshes != nullptr) {
        modelLoaded_ = true;
        
        // Apply a very subtle tint to distinguish players while preserving textures
        Color tint = (type_ == PlayerType::RED_PLAYER) ? 
            Color{255, 240, 240, 255} : // Very subtle red tint
            Color{240, 240, 255, 255};  // Very subtle blue tint
            
        // Apply tint to the diffuse color
        model_.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = tint;
        
        printf("Model loaded successfully with %d materials\n", model_.materialCount);
    } else {
        printf("Failed to load model: %s\n", config.modelPath);
    }
}

void Player::update_simulation(const PlayerInput& input, float delta_time) {
    Vector3 horizontal_movement = {0};
    horizontal_movement.x = input.movement_input_vector.x * MOVE_SPEED * delta_time;
    horizontal_movement.z = input.movement_input_vector.y * MOVE_SPEED * delta_time;
    // position_ = Vector3Add(position_, horizontal_movement); // If no collision
    position_.x += horizontal_movement.x;
    position_.z += horizontal_movement.z;

    // Apply gravity and vertical movement
    if (input.jump_action_triggered && !is_jumping_ && position_.y <= GROUND_Y) {
        velocity_.y = JUMP_FORCE;
        is_jumping_ = true;
    }

    // Ground collision and state reset
    if (position_.y <= GROUND_Y && velocity_.y < 0) {
        position_.y = GROUND_Y;
        velocity_.y = 0;
        is_jumping_ = false;
    }

    // If this is the local player, the predicted visual position is the new physical position.
    if (is_local_) {
        visual_position_ = position_;
    }

}

void Player::set_authoritative_state(const PlayerFrameState& state) {
    position_ = state.position;
    velocity_ = state.velocity;
    is_jumping_ = state.is_jumping_state;

     if (is_local_) {
        visual_position_ = position_;
    }
}

PlayerFrameState Player::get_current_frame_state(uint32_t frame_number) const {
    PlayerFrameState current_state;

    current_state.frame_number = frame_number;
    current_state.player_id = id_;
    current_state.position = position_;
    current_state.velocity = velocity_;
    current_state.is_jumping_state = is_jumping_;

    return current_state;
}

void Player::add_server_state_snapshot(const PlayerFrameState& snapshot) {
    if (is_local_) return;

    auto it = std::lower_bound(state_history_for_interpolation_.begin(), state_history_for_interpolation_.end(), snapshot.frame_number,
                               [](const PlayerFrameState& s, uint32_t val) {
                                   return s.frame_number < val;
                               });
    state_history_for_interpolation_.insert(it, snapshot);

    if (state_history_for_interpolation_.size() > MAX_INTERPOLATION_HISTORY) {
        state_history_for_interpolation_.pop_front();
    }
}

void Player::set_interpolated_visual_position(const Vector3& pos) {
    if (!is_local_) {
        visual_position_ = pos;
    }
}

// This method is called by the game loop (e.g. GameScene)
// For remote players, it's where InterpolationSystem would apply its results.
// For local players, prediction already updated visual_position_ if apply_input was called.
void Player::update_visual_state(float delta_time) {
    if (is_local_) return;
    // If it's a local player, visual_position_ is already updated by prediction (apply_input).
    // If it's a remote player, InterpolationSystem will call set_interpolated_visual_position.
    // This function could do other visual-only updates if needed (e.g., animation smoothing not tied to physics).
}


void Player::draw() const {
    if (modelLoaded_) {
        DrawModelEx(model_, position_, (Vector3){0, 1, 0}, 180.0f, (Vector3){scale_, scale_, scale_}, WHITE);
    } else {
        DrawCube(position_, 1.0f, 1.0f, 1.0f, color_);
    }
}

/*void Player::move(const Vector3& direction) {
    position_.x += direction.x * MOVE_SPEED;
    position_.y += direction.y * MOVE_SPEED;
    position_.z += direction.z * MOVE_SPEED;
}

void Player::jump() {
    if (!isJumping_) {
        velocity_.y = JUMP_FORCE;
        isJumping_ = true;
    }
}

void Player::update() {
    if (isJumping_) {
        velocity_.y -= GRAVITY;
        position_.y += velocity_.y;
        
        if (position_.y <= 1.0f) {
            position_.y = 1.0f;
            velocity_.y = 0;
            isJumping_ = false;
        }
    }
}*/

}} // namespace netcode::visualization 