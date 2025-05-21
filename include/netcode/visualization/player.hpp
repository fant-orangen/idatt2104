#pragma once

#include "raylib.h"
#include "netcode/game_data_types.hpp"
#include <deque>
#include <string>
#include <cstdint>

namespace netcode {
namespace visualization {

enum class PlayerType {
    RED_PLAYER,  // Iron Man
    BLUE_PLAYER  // Wolf
};

class Player {
public:
    Player(uint32_t id, PlayerType type, bool is_local, const Vector3& startPos = {0.0f, 1.0f, 0.0f}, const Color& playerColor = RED);
    ~Player();

    //void move(const Vector3& direction);
    //void update();
    void draw() const;
    //void jump();

    void update_simulation(const PlayerInput& input, float delta_time);
    void update_simulation_from_state(const PlayerFrameState& state);
    void update_visual_state(float delta_time);

    PlayerFrameState get_current_frame_state(uint32_t frame_number) const;
    void set_authoritative_state(const PlayerFrameState& state);

    void add_server_state_snapshot(const PlayerFrameState& snapshot);
    void set_interpolated_visual_position(const Vector3& pos);

    Vector3 get_physical_position() const { return position_; }
    Vector3 get_visual_position() const { return visual_position_; }
    uint32_t get_id() const { return id_; }
    bool is_local() const { return is_local_; }

    void loadModel(bool useCubes = false);

    //Vector3 getPosition() const { return position_; }
    //void setPosition(const Vector3& pos) { position_ = pos; }

private:
    struct ModelConfig {
        const char* modelPath;
        float scale;
    };

    static ModelConfig getModelConfig(PlayerType type);

    uint32_t id_;
    bool is_local_;
    
    Vector3 position_;
    Vector3 velocity_;
    bool is_jumping_;

    Vector3 visual_position_;
    //Quaternion visual_rotation_; // if we add orientation later

    std::deque<PlayerFrameState> state_history_for_interpolation_;
    static const size_t MAX_INTERPOLATION_HISTORY = 20;

    Color color_;
    Model model_;
    float scale_;
    PlayerType type_;
    bool modelLoaded_;

    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float JUMP_FORCE = 7.0f;
    static constexpr float GRAVITY = 15.0f;
    static constexpr float GROUND_Y = 1.0f;
    //bool isJumping_ = false;
};

}} // namespace netcode::visualization
