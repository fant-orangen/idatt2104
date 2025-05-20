#include "netcode/visualization/player.hpp"
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

Player::Player(PlayerType type, const Vector3& startPos, const Color& playerColor)
    : position_(startPos), color_(playerColor), velocity_({0.0f, 0.0f, 0.0f}), type_(type), 
      scale_(1.0f), modelLoaded_(false), isJumping_(false),
      id_(type == PlayerType::RED_PLAYER ? 1 : 2) {
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
    scale_ = config.scale;
    
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

void Player::move(const Vector3& direction) {
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
}

void Player::draw() const {
    if (modelLoaded_) {
        DrawModelEx(model_, position_, (Vector3){0, 1, 0}, 180.0f, (Vector3){scale_, scale_, scale_}, WHITE);
    } else {
        DrawCube(position_, 1.0f, 1.0f, 1.0f, color_);
    }
}

}} // namespace netcode::visualization 