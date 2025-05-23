#include "netcode/visualization/player.hpp"
#include <iostream>
#include <cmath>     // Required for atan2f and fabs
#include "raymath.h" // Required for RAD2DEG (usually included with raylib.h)

namespace netcode {
namespace visualization {

// Define statics if they are part of the class (based on typical usage, adjust if definition is elsewhere)
// const float Player::MOVE_SPEED = 5.0f; // Example value, ensure it's defined
// const float Player::JUMP_FORCE = 10.0f; // Example value
// const float Player::GRAVITY = 0.5f;    // Example value

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
            // Return a configuration that indicates an issue or a default non-visible model
            return {"", 1.0f};
    }
}

Player::Player(PlayerType type, const netcode::math::MyVec3& startPos, const Color& playerColor)
    : position_(startPos), color_(playerColor), velocity_({0.0f, 0.0f, 0.0f}), type_(type),
      scale_(1.0f), modelLoaded_(false), isJumping_(false),
      id_(type == PlayerType::RED_PLAYER ? 1 : 2), rotationAngle_(0.0f), facingLeft_(true) {
    // Initialize rotationAngle_ perhaps based on a default facing direction if needed
    // For example, if players should initially face "forward" (positive Z):
    // rotationAngle_ = 0.0f; or if facing positive X: rotationAngle_ = 90.0f;
    // The atan2f in move() will set it correctly upon first movement.
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
        printf("Loading cube model\n"); // Consider using a proper logger
        return;
    }

    ModelConfig config = getModelConfig(type_);
    if (!config.modelPath[0]) { // Check if modelPath is empty
        printf("Invalid model configuration or empty modelPath for player type %d\n", static_cast<int>(type_));
        return;
    }

    printf("Loading model: %s\n", config.modelPath); // Consider using a proper logger

    // Load the model - Raylib will automatically load associated textures through the .mtl file
    model_ = LoadModel(config.modelPath);
    scale_ = config.scale;

    if (model_.meshCount > 0 && model_.meshes != nullptr) {
    modelLoaded_ = true;

    // Apply a corrective rotation if the model is facing down by default.
    // This rotates the model 90 degrees "upwards" around its own X-axis.
    // Adjust (+90 or -90, or even MatrixRotateZ) if the model's specific orientation requires it.
    // model_.transform by default is MatrixIdentity().
    model_.transform = MatrixRotateX(-90.0f * DEG2RAD); // Pitches the model up

    // Apply a very subtle tint to distinguish players while preserving textures
    Color tint = (type_ == PlayerType::RED_PLAYER) ?
        Color{255, 240, 240, 255} : // Very subtle red tint
        Color{240, 240, 255, 255};  // Very subtle blue tint

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
    // It's assumed MOVE_SPEED is defined elsewhere, e.g., as a static const in Player.hpp or a global
    // For example: const float MOVE_SPEED = 5.0f; (units per second or per frame depending on your game loop)

    position_.x += direction.x * MOVE_SPEED;
    position_.y += direction.y * MOVE_SPEED; // This line allows for vertical movement based on 'direction.y'.
                                             // If Y movement is exclusively handled by jump/gravity, this might be undesired.
    position_.z += direction.z * MOVE_SPEED;

    // Only update rotation if there is movement in the XZ plane
    // Use a small epsilon for floating point comparisons to avoid issues with very small movements
    if (std::fabs(direction.x) > 1e-5f || std::fabs(direction.z) > 1e-5f) {
        // Calculate the angle of rotation based on the direction vector in the XZ plane.
        // atan2f(x, z) returns the angle in radians between the positive Z-axis and the point (z, x)
        // (or more precisely, the vector from origin to (x,z) on the cartesian plane).
        // We multiply by RAD2DEG to convert to degrees for Raylib.
        // This assumes your model's "front" is along its local +Z axis when rotationAngle_ is 0.
        rotationAngle_ = atan2f(direction.x, direction.z) * RAD2DEG;

        // Update facingLeft_ based on the X component of the direction vector.
        // This maintains its previous behavior. You might want to re-evaluate if
        // facingLeft_ is used for anything critical or if rotationAngle_ suffices.
        if (direction.x < -1e-5f) { // Moving predominantly left
            facingLeft_ = true;
        } else if (direction.x > 1e-5f) { // Moving predominantly right
            facingLeft_ = false;
        }
        // If direction.x is near zero (e.g., moving purely along Z), facingLeft_ remains unchanged.
    }
}

void Player::jump() {
    // It's assumed JUMP_FORCE is defined elsewhere.
    // For example: const float JUMP_FORCE = 10.0f;
    if (!isJumping_ && position_.y <= 1.01f) { // Added a small tolerance for ground check
        velocity_.y = JUMP_FORCE;
        isJumping_ = true;
    }
}

void Player::update() {
    // It's assumed GRAVITY is defined elsewhere.
    // For example: const float GRAVITY = 0.5f; (acceleration per frame or per second)
    // Also, ground_level should be a defined constant or variable.
    const float ground_level = 1.0f; // Example, ensure this matches your scene's ground

    if (isJumping_) {
        position_.y += velocity_.y; // Apply vertical velocity
        velocity_.y -= GRAVITY;     // Apply gravity

        if (position_.y <= ground_level) {
            position_.y = ground_level; // Land on the ground
            velocity_.y = 0;            // Reset vertical velocity
            isJumping_ = false;         // No longer jumping
        }
    }
    // Ensure player Y is not below ground even if not jumping (e.g. due to external factors)
    if (position_.y < ground_level && !isJumping_){
        position_.y = ground_level;
        velocity_.y = 0;
    }
}

void Player::draw() const {
    if (modelLoaded_) {
        // For planar movement (XZ plane), rotation is typically around the Y-axis.
        Vector3 rotationAxis = {0.0f, 1.0f, 0.0f};

        DrawModelEx(model_,
                   Vector3{position_.x, position_.y, position_.z},
                   rotationAxis,       // Use the consistent Y-axis for rotation
                   rotationAngle_,     // Angle calculated in the move function
                   Vector3{scale_, scale_, scale_},
                   WHITE); // WHITE means no additional tinting here, tint is on material
    } else {
        // Fallback to drawing a cube if the model isn't loaded
        DrawCube(
            Vector3{position_.x, position_.y, position_.z},
            1.0f, 1.0f, 1.0f, // Cube dimensions
            color_); // Player's color
    }
}

// Ensure static const members like MOVE_SPEED, JUMP_FORCE, GRAVITY are defined.
// If they are part of the Player class, they should be defined in the .cpp file like:
// const float Player::MOVE_SPEED = 5.0f; // example value
// const float Player::JUMP_FORCE = 10.0f; // example value
// const float Player::GRAVITY = 0.5f;    // example value
// Or ensure they are accessible global constants if defined outside the class.

}} // namespace netcode::visualization