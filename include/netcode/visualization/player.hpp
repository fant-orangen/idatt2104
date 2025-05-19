#pragma once

#include "raylib.h"

namespace netcode {
namespace visualization {

enum class PlayerType {
    RED_PLAYER,  // Iron Man
    BLUE_PLAYER  // Wolf
};

class Player {
public:
    Player(PlayerType type, const Vector3& startPos = {0.0f, 1.0f, 0.0f}, const Color& playerColor = RED);
    ~Player();

    void move(const Vector3& direction);
    void update();
    void draw() const;

    Vector3 getPosition() const { return position_; }
    void setPosition(const Vector3& pos) { position_ = pos; }
    void loadModel(bool useCubes = true);

private:
    Vector3 position_;
    Vector3 velocity_;
    Color color_;
    Model model_;
    float scale_;
    PlayerType type_;
    bool modelLoaded_;
    const float MOVE_SPEED = 0.2f;

    void loadModel();
};

}} // namespace netcode::visualization
