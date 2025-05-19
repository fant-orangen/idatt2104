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

GameScene::GameScene(int viewportWidth, int viewportHeight, float x, float y, const char* label,
                     KeyboardKey keyUp, KeyboardKey keyDown, KeyboardKey keyLeft, KeyboardKey keyRight,
                     Color playerColor)
    : label_(label),
      keyUp_(keyUp), keyDown_(keyDown), keyLeft_(keyLeft), keyRight_(keyRight) {
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
    player_ = std::make_unique<Player>(Vector3{0.0f, 1.0f, 0.0f}, playerColor); // Initialize Player
}

void GameScene::handleInput() {
    // Handle arrow key movement
    Vector3 moveDir = {0.0f, 0.0f, 0.0f};
    if (keyRight_ != KEY_NULL && IsKeyDown(keyRight_)) moveDir.x += 1.0f;
    if (keyLeft_ != KEY_NULL && IsKeyDown(keyLeft_)) moveDir.x -= 1.0f;
    if (keyUp_ != KEY_NULL && IsKeyDown(keyUp_)) moveDir.z -= 1.0f; // Forward in Z
    if (keyDown_ != KEY_NULL && IsKeyDown(keyDown_)) moveDir.z += 1.0f; // Backward in Z

    if (player_ && (moveDir.x != 0.0f || moveDir.y != 0.0f || moveDir.z != 0.0f)) { // Check player_ existence and any movement
        // Normalize if you want consistent speed diagonally, or leave as is for faster diagonal
        // moveDir = Vector3Normalize(moveDir); // Uncomment for normalized movement
        player_->move(moveDir); // TODO: this i the value which must be sent to the server
    }
}

void GameScene::render() {
    // Draw white background for the viewport
    DrawRectangle(0, 0, bounds_.width, bounds_.height, RAYWHITE);
    // Draw viewport label
    DrawText(label_, 10, 5, 20, BLACK);
    camera_.position = {0.0f, 10.0f, 2.0f};
    BeginMode3D(camera_);
    if(player_) player_->draw(); // Draw the player
    DrawGrid(10, 1.0f);
    EndMode3D();
}

}} // namespace netcode::visualization
