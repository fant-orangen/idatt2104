#include "netcode/visualization/game_scene.hpp"
#include "rlgl.h"

namespace netcode {
namespace visualization {

FramebufferRect toFramebufferRect(const Rectangle& logicalRect) {
    float scaleX = (float)GetRenderWidth() / GetScreenWidth();
    float scaleY = (float)GetRenderHeight() / GetScreenHeight();
    return {
        (int)(logicalRect.x * scaleX),
        (int)(logicalRect.y * scaleY),
        (int)(logicalRect.width * scaleX),
        (int)(logicalRect.height * scaleY)
    };
}

GameScene::GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label)
    : label_(label) {
    bounds_ = {x, y, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    float aspect = bounds_.width / bounds_.height;
    // Move camera farther back and increase FOV for a wider view
    camera_ = {
        {0.0f, 15.0f, 25.0f},  // Position farther back and higher
        {0.0f, 0.0f, 0.0f},    // Target
        {0.0f, 1.0f, 0.0f},    // Up
        60.0f,                  // Wider FOV
        CAMERA_PERSPECTIVE
    };
    playerPos_ = {0.0f, 0.0f, 0.0f};
}

void GameScene::handleInput() {
    // Handle arrow key movement
    if (IsKeyDown(KEY_RIGHT)) playerPos_.x += 0.1f;
    if (IsKeyDown(KEY_LEFT)) playerPos_.x -= 0.1f;
    if (IsKeyDown(KEY_UP)) playerPos_.z -= 0.1f;
    if (IsKeyDown(KEY_DOWN)) playerPos_.z += 0.1f;
}

void GameScene::render() {
    FramebufferRect fb = toFramebufferRect(bounds_);
    BeginScissorMode(fb.x, fb.y, fb.width, fb.height);
    rlViewport(fb.x, fb.y, fb.width, fb.height);
    
    // Draw white background for the viewport
    DrawRectangle(bounds_.x, bounds_.y, bounds_.width, bounds_.height, RAYWHITE);
    
    // Draw viewport label
    
    DrawText(label_, bounds_.x + 10, bounds_.y + 5, 20, BLACK);
    
    camera_.position = {0.0f, 10.0f, 2.0f};

    // Draw 3D scene
    BeginMode3D(camera_);
    
    // Draw player (red cube)
    DrawCube(playerPos_, 1.0f, 1.0f, 1.0f, RED);
    
    // Draw grid for reference
    DrawGrid(10, 1.0f);
    
    EndMode3D();
    
    rlViewport(0, 0, GetRenderWidth(), GetRenderHeight());
    EndScissorMode();
}

}} // namespace netcode::visualization
