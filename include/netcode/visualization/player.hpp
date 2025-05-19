#pragma once

#include "raylib.h"

namespace netcode {
namespace visualization {

class Player {
public:
    Player(const Vector3& startPos = {0.0f, 1.0f, 0.0f}, const Color& playerColor = RED);

    void move(const Vector3& direction);
    void update();
    void draw() const;

    Vector3 getPosition() const { return position_; }
    void setPosition(const Vector3& pos) { position_ = pos; }

private:
    Vector3 position_;
    Vector3 velocity_;
    Color color_;
    const float MOVE_SPEED = 0.2f;
    const float CUBE_SIZE = 1.0f;
};

}} // namespace netcode::visualization
