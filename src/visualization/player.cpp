#include "netcode/visualization/player.hpp"

namespace netcode {
namespace visualization {

Player::Player(const Vector3& startPos, const Color& playerColor)
    : position_(startPos), color_(playerColor), velocity_({0.0f, 0.0f, 0.0f}) {}

void Player::move(const Vector3& direction) {
    // For now, we'll directly change position based on direction.
    // More sophisticated movement (like applying velocity) can be added in update().
    position_.x += direction.x * MOVE_SPEED;
    position_.y += direction.y * MOVE_SPEED;
    position_.z += direction.z * MOVE_SPEED;
}

void Player::update() {
    // This method can be used for physics, animations, etc.
    // For example, applying velocity to position:
    // position_.x += velocity_.x;
    // position_.y += velocity_.y;
    // position_.z += velocity_.z;
    // And then perhaps applying friction/drag to velocity_.
}

void Player::draw() const {
    DrawCube(position_, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, color_);
}

}} // namespace netcode::visualization 