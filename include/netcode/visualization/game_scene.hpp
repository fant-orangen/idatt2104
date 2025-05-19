#pragma once

#include "raylib.h"
#include "netcode/visualization/entity_controller.hpp"
#include <memory>
#include <string>

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
    GameScene(int viewportWidth, int viewportHeight, ViewType type, 
              float x, float y, const char* label);
    ~GameScene();

    void update();
    void render();
    void handleInput();
    void updatePlayerState(const Vector3& position);
    
    // Access to controllers for network state synchronization
    EntityController& getEntityController() { return *entityController_; }

private:
    void setupCamera();
    void drawScene();
    void drawGrid() const;

    ViewType viewType_;
    std::unique_ptr<EntityController> entityController_;
    Camera3D camera_;
    Rectangle bounds_;
    const char* label_;
    const float CAMERA_DISTANCE = 10.0f;
};

}} // namespace netcode::visualization
