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

            // Set to arrow keys by default
              KeyboardKey keyUp = KEY_UP, KeyboardKey keyDown = KEY_DOWN, 
              KeyboardKey keyLeft = KEY_LEFT, KeyboardKey keyRight = KEY_RIGHT);
    void render();
    void handleInput();  // New method for input handling

private:
    Rectangle bounds_;
    const char* label_;
    Camera3D camera_;    // 3D camera
    std::unique_ptr<Player> player_;
    KeyboardKey keyUp_;      // Key for moving up
    KeyboardKey keyDown_;    // Key for moving down
    KeyboardKey keyLeft_;    // Key for moving left
    KeyboardKey keyRight_;   // Key for moving right
};

}} // namespace netcode::visualization
