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
    player_ = std::make_unique<Player>(Vector3{0.0f, 1.0f, 0.0f}, RED); // Initialize Player
}

void GameScene::handleInput() {
    // Handle arrow key movement
    Vector3 moveDir = {0.0f, 0.0f, 0.0f};
    if (IsKeyDown(KEY_RIGHT)) moveDir.x += 1.0f;
    if (IsKeyDown(KEY_LEFT)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_UP)) moveDir.z -= 1.0f; // Forward in Z
    if (IsKeyDown(KEY_DOWN)) moveDir.z += 1.0f; // Backward in Z

    if (moveDir.x != 0.0f || moveDir.z != 0.0f) {
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
