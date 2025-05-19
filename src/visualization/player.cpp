#include "netcode/visualization/player.hpp"

namespace netcode {
namespace visualization {

Player::Player(const Vector3& startPos, const Color& playerColor)
    : position_(startPos), velocity_({0, 0, 0}), color_(playerColor) {}

void Player::move(const Vector3& direction) {
    velocity_.x = direction.x * MOVE_SPEED;
    velocity_.z = direction.z * MOVE_SPEED;
}

void Player::update() {
    position_.x += velocity_.x;
    position_.z += velocity_.z;
}

void Player::draw() const {
    // Draw main player cube
    DrawCube(position_, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, color_);
    DrawCubeWires(position_, CUBE_SIZE, CUBE_SIZE, CUBE_SIZE, BLACK);
}

}} // namespace netcode::visualization 