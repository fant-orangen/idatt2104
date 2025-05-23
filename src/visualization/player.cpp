#include "netcode/visualization/player.hpp"
#include <iostream>
#include <cmath>
#include "raymath.h"

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

Player::Player(PlayerType type, const netcode::math::MyVec3& startPos, const Color& playerColor)
    : position_(startPos), renderPosition_(startPos), color_(playerColor), velocity_({0.0f, 0.0f, 0.0f}), type_(type),
      scale_(1.0f), modelLoaded_(false), isJumping_(false), isVisuallyBlending_(false),
      id_(type == PlayerType::RED_PLAYER ? 1 : 2), rotationAngle_(0.0f), facingLeft_(true) {
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
        printf("Invalid model configuration or empty modelPath for player type %d\n", static_cast<int>(type_));
        return;
    }

    printf("Loading model: %s\n", config.modelPath);

    model_ = LoadModel(config.modelPath);
    scale_ = config.scale;

    if (model_.meshCount > 0 && model_.meshes != nullptr) {
    modelLoaded_ = true;

      model_.transform = MatrixRotateX(-90.0f * DEG2RAD);

    Color tint = (type_ == PlayerType::RED_PLAYER) ?
        Color{255, 255, 255, 255} :
        Color{101, 67, 33, 255};

    if (model_.materialCount > 0 && model_.materials != nullptr) {
         model_.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = tint;
    } else {
        printf("Model loaded but has no materials to apply tint.\n");
    }
    printf("Model loaded successfully with %d materials\n", model_.materialCount);
} else {
    printf("Failed to load model: %s\n", config.modelPath);
}
}

void Player::move(const netcode::math::MyVec3& direction) {
    position_.x += direction.x * MOVE_SPEED;
    position_.y += direction.y * MOVE_SPEED;
    position_.z += direction.z * MOVE_SPEED;

     if (std::fabs(direction.x) > 1e-5f || std::fabs(direction.z) > 1e-5f) {
          rotationAngle_ = atan2f(direction.x, direction.z) * RAD2DEG;

          if (direction.x < -1e-5f) {
            facingLeft_ = true;
        } else if (direction.x > 1e-5f) {
            facingLeft_ = false;
        }
    }
}

void Player::setPosition(const netcode::math::MyVec3& pos) {
    // Calculate direction based on position change for rotation
    netcode::math::MyVec3 direction = {
        pos.x - position_.x,
        pos.y - position_.y,
        pos.z - position_.z
    };
    
    // Update position first
    position_ = pos;
    
    // Calculate rotation if there's significant horizontal movement
    if (std::fabs(direction.x) > 1e-5f || std::fabs(direction.z) > 1e-5f) {
        rotationAngle_ = atan2f(direction.x, direction.z) * RAD2DEG;

        if (direction.x < -1e-5f) {
            facingLeft_ = true;
        } else if (direction.x > 1e-5f) {
            facingLeft_ = false;
        }
    }
}

void Player::jump() {
    if (!isJumping_ && position_.y <= 1.01f) {
        velocity_.y = JUMP_FORCE;
        isJumping_ = true;
    }
}

void Player::update() {
    const float ground_level = 1.0f;

    if (isJumping_) {
        position_.y += velocity_.y;
        velocity_.y -= GRAVITY;

        if (position_.y <= ground_level) {
            position_.y = ground_level;
            velocity_.y = 0;
            isJumping_ = false;
        }
    }

    if (position_.y < ground_level && !isJumping_){
        position_.y = ground_level;
        velocity_.y = 0;
    }
}

void Player::updateRenderPosition(float deltaTime) {
    if (isVisuallyBlending_) {
        // Update blend progress
        visualBlendProgress_ += deltaTime * VISUAL_BLEND_SPEED;

        if (visualBlendProgress_ >= 1.0f) {
            // Blending complete
            renderPosition_ = position_;
            isVisuallyBlending_ = false;
            visualBlendProgress_ = 0.0f;
        } else {
            // Calculate new render position using interpolation
            float t = visualBlendProgress_;
            netcode::math::MyVec3 startPos = renderPosition_;
            netcode::math::MyVec3 targetPos = position_;

            renderPosition_.x = startPos.x + (targetPos.x - startPos.x) * t;
            renderPosition_.y = startPos.y + (targetPos.y - startPos.y) * t;
            renderPosition_.z = startPos.z + (targetPos.z - startPos.z) * t;
        }
    } else {
        // If not blending, immediately use simulation position
        renderPosition_ = position_;
    }
}

void Player::snapSimulationState(const netcode::math::MyVec3& position, bool isJumping, float velocityY) {
    // Update simulation state to match the server's authoritative state
    position_ = position;
    isJumping_ = isJumping;

    if (velocityY != 0.0f) {
        velocity_.y = velocityY;
    }
}

void Player::initiateVisualBlend() {
    // Start visual blending from current render position to new simulation position
    isVisuallyBlending_ = true;
    visualBlendProgress_ = 0.0f;
}

void Player::draw() const {
    if (modelLoaded_) {
        Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};

        DrawModelEx(model_, 
                   Vector3{renderPosition_.x, renderPosition_.y, renderPosition_.z}, // Use render position for display
                   rotationAxis,
                   rotationAngle_,
                   Vector3{scale_, scale_, scale_}, 
                   WHITE);
    } else {
        DrawCube(
            Vector3{renderPosition_.x, renderPosition_.y, renderPosition_.z}, // Use render position for display
            1.0f, 1.0f, 1.0f, color_);
    }
}
}} // namespace netcode::visualization