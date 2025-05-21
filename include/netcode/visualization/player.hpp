#pragma once

#include "raylib.h"
#include <cstdint>

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
    void jump();

    Vector3 getPosition() const { return position_; }
    void setPosition(const Vector3& pos) { position_ = pos; }
    void loadModel(bool useCubes = false);
    uint32_t getId() const { return id_; }
    float getMoveSpeed() const { return MOVE_SPEED; }

private:
    struct ModelConfig {
        const char* modelPath;
        float scale;
    };

    static ModelConfig getModelConfig(PlayerType type);
    
    Vector3 position_;
    Vector3 velocity_;
    Color color_;
    Model model_;
    float scale_;
    PlayerType type_;
    bool modelLoaded_;
    const float MOVE_SPEED = 0.2f;
    const float JUMP_FORCE = 1.5f;
    const float GRAVITY = 0.2f;
    bool isJumping_ = false;
    uint32_t id_;  // Player ID: 1 for RED_PLAYER, 2 for BLUE_PLAYER
};

}} // namespace netcode::visualization
