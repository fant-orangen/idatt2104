#include "netcode/visualization/player.hpp"
#include <iostream>

namespace netcode {
namespace visualization {

Player::Player(PlayerType type, const Vector3& startPos, const Color& playerColor)
    : position_(startPos), color_(playerColor), velocity_({0.0f, 0.0f, 0.0f}), type_(type), 
      scale_(1.0f), modelLoaded_(false) {
    loadModel(true);  // Default to cubes
}

Player::~Player() {
    if (modelLoaded_) {
        UnloadModel(model_);
    }
}

void Player::loadModel(bool useCubes) {
    if (useCubes) {
        modelLoaded_ = false;
        return;
    }

    const char* modelPath;
    switch (type_) {
        case PlayerType::RED_PLAYER:
            modelPath = "../assets/wolf/Wolf_obj.obj";
            scale_ = 0.5f;
            break;
        case PlayerType::BLUE_PLAYER:
            modelPath = "../assets/wolf/Wolf_obj.obj";
            scale_ = 0.5f;
            break;
    }
    
    // Initialize model with default values
    model_ = { 0 };
    modelLoaded_ = false;
    
    // Try to load the model
    model_ = LoadModel(modelPath);
    
    // Simple validation - check if model loaded successfully
    if (model_.meshCount > 0 && model_.meshes != nullptr) {
        modelLoaded_ = true;
    }
}

void Player::move(const Vector3& direction) {
    position_.x += direction.x * MOVE_SPEED;
    position_.y += direction.y * MOVE_SPEED;
    position_.z += direction.z * MOVE_SPEED;
}

void Player::update() {
    // This method can be used for physics, animations, etc.
}

void Player::draw() const {
    if (modelLoaded_) {
        DrawModelEx(model_, position_, (Vector3){0, 1, 0}, 0.0f, (Vector3){scale_, scale_, scale_}, color_);
    } else {
        // Fallback to cube if model failed to load
        DrawCube(position_, 1.0f, 1.0f, 1.0f, color_);
    }
}

}} // namespace netcode::visualization 