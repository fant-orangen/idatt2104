#pragma once

#include "raylib.h"
#include "netcode/visualization/player.hpp"
#include <memory>

namespace netcode {
namespace visualization {

/**
 * @class GameScene
 * @brief Manages the game scene with multiple viewports
 * 
 * Handles rendering, updating, and input processing for a 3D game scene
 */
struct Viewport {
    Rectangle bounds;
    Camera3D camera;
    const char* label;
};

struct FramebufferRect {
    int x, y, width, height;
};

FramebufferRect toFramebufferRect(const Rectangle& logicalRect);

class GameScene {
public:
    GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label,
              KeyboardKey redUp = KEY_W, KeyboardKey redDown = KEY_S, 
              KeyboardKey redLeft = KEY_A, KeyboardKey redRight = KEY_D,
              KeyboardKey blueUp = KEY_UP, KeyboardKey blueDown = KEY_DOWN, 
              KeyboardKey blueLeft = KEY_LEFT, KeyboardKey blueRight = KEY_RIGHT);
    ~GameScene();
    void render();
    void handleInput();

    // Getters for player instances
    std::shared_ptr<Player> getRedPlayer() const { return redPlayer_; }
    std::shared_ptr<Player> getBluePlayer() const { return bluePlayer_; }

    // Getters for movement directions
    Vector3 getRedMovementDirection() const { return redMoveDir_; }
    Vector3 getBlueMovementDirection() const { return blueMoveDir_; }
    bool getRedJumpRequested() const { return redJumpRequested_; }
    bool getBlueJumpRequested() const { return blueJumpRequested_; }

    // Camera control methods
    void panCamera(float yawDelta, float pitchDelta);
    void moveCameraUp(float amount);
    void moveCameraRight(float amount);
    void zoomCamera(float zoomAmount);

    // Texture control
    void setUseTexture(bool use) { USE_TEXTURE = use; }
    bool getUseTexture() const { return USE_TEXTURE; }

private:
    Rectangle bounds_;
    const char* label_;
    Camera3D camera_;
    std::shared_ptr<Player> redPlayer_;   // Red player (WASD)
    std::shared_ptr<Player> bluePlayer_;  // Blue player (arrow keys)
    
    // Texture control
    bool USE_TEXTURE = false;
    
    // Add texture support
    Texture2D groundTexture_ = LoadTexture("../assets/grass/textures/lambert1_baseColor.png"); 
    Model groundModel_;
    bool groundTextureLoaded_ = false;
    
    // Movement direction vectors
    Vector3 redMoveDir_ = {0.0f, 0.0f, 0.0f};
    Vector3 blueMoveDir_ = {0.0f, 0.0f, 0.0f};
    bool redJumpRequested_ = false;
    bool blueJumpRequested_ = false;
    
    // Red player controls (WASD by default)
    KeyboardKey redUp_;
    KeyboardKey redDown_;
    KeyboardKey redLeft_;
    KeyboardKey redRight_;
    
    // Blue player controls (Arrow keys by default)
    KeyboardKey blueUp_;
    KeyboardKey blueDown_;
    KeyboardKey blueLeft_;
    KeyboardKey blueRight_;
};

}} // namespace netcode::visualization
