#pragma once

#include "raylib.h"

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

class GameScene {
public:
    GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label);
    void render();

private:
    Rectangle bounds_;
    const char* label_;
};

}} // namespace netcode::visualization
