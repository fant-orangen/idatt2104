#pragma once

#include "raylib.h"
#include <cstdint>
#include "netcode/networked_entity.hpp"
#include "netcode/math/my_vec3.hpp"

namespace netcode {
namespace visualization {

enum class PlayerType {
    RED_PLAYER,  // Iron Man
    BLUE_PLAYER  // Wolf
};

class Player : public netcode::NetworkedEntity {
public:
    Player(PlayerType type, const netcode::math::MyVec3& startPos = {0.0f, 1.0f, 0.0f}, const Color& playerColor = RED);
    ~Player();
    
    /**
     * @brief Move the player in the given direction
     * @param direction The direction to move in
     */
    void move(const netcode::math::MyVec3& direction) override;

    /**
     * @brief Update the player's position and velocity
     */
    void update() override;

    /**
     * @brief Draw the player
     */
    void draw() const;

    /**
     * @brief Make the player jump
     */
    void jump() override;

    /**
     * @brief Get the player's position
     * @return The player's position
     */
    netcode::math::MyVec3 getPosition() const override { return position_; }

    /**
     * @brief Set the player's position
     * @param pos The new position
     */
    void setPosition(const netcode::math::MyVec3& pos) override { position_ = pos; }

    /**
     * @brief Load the player's model
     * @param useCubes Whether to use cubes instead of the default model
     */
    void loadModel(bool useCubes = false);

    /**
     * @brief Get the player's ID
     * @return The player's ID
     */
    uint32_t getId() const override { return id_; }

    /**
     * @brief Get the player's move speed
     * @return The player's move speed
     */
    float getMoveSpeed() const override { return MOVE_SPEED; }

private:
    struct ModelConfig {
        const char* modelPath;
        float scale;
    };

    static ModelConfig getModelConfig(PlayerType type);
    
    netcode::math::MyVec3 position_;
    netcode::math::MyVec3 velocity_;
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
